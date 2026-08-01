// D2Debugger.audiocap.cpp -- capture the game's audio at the DirectSound layer
// and mix it ourselves.
//
// GOAL. This is the audio leg of remote play: the frame capture already yields
// clean images and the virtual-input layer already accepts clicks and keys, so
// the missing piece for "play D2 in a browser" is a PCM stream. Capturing at
// the DirectSound buffer level (rather than off the speakers) gives PRE-MIX
// sources, which is what a remote client actually wants -- it can remix for
// headphones, drop positional audio, or re-encode, none of which is possible
// once the mix has been flattened.
//
// WHY NOT THE OBVIOUS ROUTES
//   * GetActiveWindow: D2Sound.dll IMPORTS it and never calls it. Measured --
//     hook attached and firing (self-test), total calls across the whole
//     process: zero. Importing is not calling; this codebase has now been bitten
//     by that twice (see D2Debugger.vinput.cpp on GetCursorPos).
//   * System loopback: captures every sound on the machine, not the game's.
//
// THE SILENCE PROBLEM. DirectSound silences a secondary buffer whenever the
// window passed to SetCooperativeLevel loses focus, unless the buffer was
// created with DSBCAPS_GLOBALFOCUS. That window is the GAME window, so simply
// focusing the debugger panel mutes everything -- and we would faithfully
// capture the silence. CreateSoundBuffer therefore has GLOBALFOCUS added on the
// way through. Whether the OPERATOR hears anything stays our decision, keyed on
// whether any window of this process is foreground.
//
// SCOPE (agreed): per-buffer volume, pan and loop/cursor tracking are honoured.
// SetFrequency resampling and 3D positioning are not -- D2 barely uses them and
// they are where the DSP bug surface lives. Buffer sample RATE is still
// converted, or everything would play at the wrong pitch; that is format
// handling, not pitch shifting.
//
// SAFETY. Every hook body is SEH-guarded and does the minimum on the game's own
// audio thread: record state, memcpy the written PCM, return. All mixing happens
// on our thread. A hook that stalls the game is not a hypothetical here -- one
// already froze it solid earlier today.

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <atomic>
#include <mutex>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include "detours.h"

#pragma comment(lib, "winmm.lib")

namespace
{
	constexpr int kOutRate = 44100;
	constexpr int kOutCh = 2;
	constexpr int kMixFrames = 441;              // 10 ms per tick
	constexpr int kWaveBuffers = 4;

	struct BufState
	{
		WAVEFORMATEX fmt{};
		std::vector<BYTE> pcm;                   // shadow copy of the buffer
		bool   playing = false;
		bool   looping = false;
		LONG   volume = 0;                       // DSBVOLUME_MAX == 0 (dB*100)
		LONG   pan = 0;                          // -10000..10000
		double cursor = 0.0;                     // in SOURCE frames
	};

	std::mutex g_mx;
	std::map<IDirectSoundBuffer*, BufState> g_bufs;

	std::atomic<bool> g_enabled{ true };
	std::atomic<bool> g_playLocal{ true };
	std::atomic<unsigned long> g_locks{ 0 }, g_plays{ 0 }, g_mixed{ 0 };
	std::atomic<int>  g_active{ 0 };
	std::atomic<bool> g_run{ false };

	HANDLE g_thread = nullptr;

	// ---- verification ("shadow mode" for audio) -----------------------------
	//
	// Same discipline the dispatchers use on code: run BOTH, capture BOTH,
	// compare. The reference here is the NATIVE mix -- what DirectSound and
	// Windows actually render -- taken by WASAPI loopback. Ours is what the
	// mixer above produces from the same buffer writes.
	//
	// Our own waveOut playback is part of this process's output and would land
	// in the loopback capture, so a verify run MUTES local playback for its
	// duration. Comparing our mix against a recording of our mix would return a
	// perfect score and mean nothing.
	std::mutex g_capMx;
	std::vector<short> g_capOurs;      // our mixer output, interleaved stereo
	std::vector<short> g_capRef;       // WASAPI loopback of the native mix
	std::atomic<bool> g_capturing{ false };
	std::atomic<bool> g_refRun{ false };
	std::atomic<int>  g_refRate{ 0 };
	std::atomic<int>  g_refCh{ 0 };

	// ---- original vtable entries -------------------------------------------
	using CreateSoundBufferFn = HRESULT(WINAPI*)(IDirectSound*, LPCDSBUFFERDESC,
	                                             LPDIRECTSOUNDBUFFER*, LPUNKNOWN);
	using LockFn = HRESULT(WINAPI*)(IDirectSoundBuffer*, DWORD, DWORD, LPVOID*,
	                                LPDWORD, LPVOID*, LPDWORD, DWORD);
	using UnlockFn = HRESULT(WINAPI*)(IDirectSoundBuffer*, LPVOID, DWORD, LPVOID, DWORD);
	using PlayFn = HRESULT(WINAPI*)(IDirectSoundBuffer*, DWORD, DWORD, DWORD);
	using StopFn = HRESULT(WINAPI*)(IDirectSoundBuffer*);
	using SetVolumeFn = HRESULT(WINAPI*)(IDirectSoundBuffer*, LONG);
	using SetPanFn = HRESULT(WINAPI*)(IDirectSoundBuffer*, LONG);

	CreateSoundBufferFn real_CreateSoundBuffer = nullptr;
	LockFn      real_Lock = nullptr;
	UnlockFn    real_Unlock = nullptr;
	PlayFn      real_Play = nullptr;
	StopFn      real_Stop = nullptr;
	SetVolumeFn real_SetVolume = nullptr;
	SetPanFn    real_SetPan = nullptr;
	bool g_bufferVtableHooked = false;

	// Track what each Lock handed out so Unlock knows where to copy FROM. A
	// buffer can be locked in two segments (DirectSound buffers are circular).
	struct PendingLock { void* p1; DWORD b1; void* p2; DWORD b2; DWORD offset; };
	std::map<IDirectSoundBuffer*, PendingLock> g_pending;

	bool OurProcessHasFocus()
	{
		HWND fg = GetForegroundWindow();
		if (!fg)
			return false;
		DWORD pid = 0;
		GetWindowThreadProcessId(fg, &pid);
		return pid == GetCurrentProcessId();
	}

	// DirectSound volume is hundredths of a dB, 0 == full, -10000 == silence.
	float GainFromDb(LONG mb)
	{
		if (mb <= DSBVOLUME_MIN)
			return 0.0f;
		if (mb >= 0)
			return 1.0f;
		return powf(10.0f, (float)mb / 2000.0f);
	}

	// ---- hooks (game audio thread) -----------------------------------------
	//
	// Each hook is split in two: a helper doing the C++ work (locks, maps,
	// vectors) and a thin hook wrapping the CALL in SEH. MSVC rejects __try in
	// any function that needs object unwinding (C2712), and every one of these
	// bodies takes a lock_guard. Same split the frame capture needed.

	void Rec_Lock(IDirectSoundBuffer* self, LPVOID* p1, LPDWORD b1,
	              LPVOID* p2, LPDWORD b2, DWORD off, DWORD flags)
	{
		std::lock_guard<std::mutex> lk(g_mx);
		PendingLock pl{};
		pl.p1 = p1 ? *p1 : nullptr;
		pl.b1 = b1 ? *b1 : 0;
		pl.p2 = p2 ? *p2 : nullptr;
		pl.b2 = b2 ? *b2 : 0;
		pl.offset = (flags & DSBLOCK_ENTIREBUFFER) ? 0 : off;
		g_pending[self] = pl;
	}

	void Rec_Unlock(IDirectSoundBuffer* self)
	{
		std::lock_guard<std::mutex> lk(g_mx);
		auto it = g_bufs.find(self);
		auto pit = g_pending.find(self);
		if (it != g_bufs.end() && pit != g_pending.end())
		{
			BufState& st = it->second;
			const PendingLock& pl = pit->second;
			// Segment 2 always wraps to offset 0 -- that is what makes a
			// DirectSound buffer circular.
			if (pl.p1 && pl.b1 && pl.offset + pl.b1 <= st.pcm.size())
				memcpy(st.pcm.data() + pl.offset, pl.p1, pl.b1);
			if (pl.p2 && pl.b2 && pl.b2 <= st.pcm.size())
				memcpy(st.pcm.data(), pl.p2, pl.b2);
			g_locks.fetch_add(1, std::memory_order_relaxed);
		}
		g_pending.erase(self);
	}

	void Rec_Play(IDirectSoundBuffer* self, DWORD flags)
	{
		std::lock_guard<std::mutex> lk(g_mx);
		auto it = g_bufs.find(self);
		if (it != g_bufs.end())
		{
			it->second.playing = true;
			it->second.looping = (flags & DSBPLAY_LOOPING) != 0;
			it->second.cursor = 0.0;
			g_plays.fetch_add(1, std::memory_order_relaxed);
		}
	}

	void Rec_Stop(IDirectSoundBuffer* self)
	{
		std::lock_guard<std::mutex> lk(g_mx);
		auto it = g_bufs.find(self);
		if (it != g_bufs.end())
			it->second.playing = false;
	}

	void Rec_Volume(IDirectSoundBuffer* self, LONG v)
	{
		std::lock_guard<std::mutex> lk(g_mx);
		auto it = g_bufs.find(self);
		if (it != g_bufs.end())
			it->second.volume = v;
	}

	void Rec_Pan(IDirectSoundBuffer* self, LONG pan)
	{
		std::lock_guard<std::mutex> lk(g_mx);
		auto it = g_bufs.find(self);
		if (it != g_bufs.end())
			it->second.pan = pan;
	}

	void Rec_NewBuffer(IDirectSoundBuffer* b, DWORD bytes)
	{
		WAVEFORMATEX wf{};
		DWORD got = 0;
		if (FAILED(b->GetFormat(&wf, sizeof(wf), &got)) || !wf.nChannels)
			return;
		std::lock_guard<std::mutex> lk(g_mx);
		BufState st;
		st.fmt = wf;
		st.pcm.assign(bytes, 0);
		g_bufs[b] = std::move(st);
	}


	HRESULT WINAPI H_Lock(IDirectSoundBuffer* self, DWORD off, DWORD bytes,
	                      LPVOID* p1, LPDWORD b1, LPVOID* p2, LPDWORD b2, DWORD flags)
	{
		HRESULT hr = real_Lock(self, off, bytes, p1, b1, p2, b2, flags);
		if (FAILED(hr) || !g_enabled.load(std::memory_order_relaxed))
			return hr;
		__try { Rec_Lock(self, p1, b1, p2, b2, off, flags); }
		__except (EXCEPTION_EXECUTE_HANDLER) {}
		return hr;
	}

	HRESULT WINAPI H_Unlock(IDirectSoundBuffer* self, LPVOID p1, DWORD b1,
	                        LPVOID p2, DWORD b2)
	{
		if (g_enabled.load(std::memory_order_relaxed))
		{
			__try { Rec_Unlock(self); }
			__except (EXCEPTION_EXECUTE_HANDLER) {}
		}
		return real_Unlock(self, p1, b1, p2, b2);
	}

	HRESULT WINAPI H_Play(IDirectSoundBuffer* self, DWORD r1, DWORD prio, DWORD flags)
	{
		if (g_enabled.load(std::memory_order_relaxed))
		{
			__try { Rec_Play(self, flags); }
			__except (EXCEPTION_EXECUTE_HANDLER) {}
		}
		return real_Play(self, r1, prio, flags);
	}

	HRESULT WINAPI H_Stop(IDirectSoundBuffer* self)
	{
		__try { Rec_Stop(self); }
		__except (EXCEPTION_EXECUTE_HANDLER) {}
		return real_Stop(self);
	}

	HRESULT WINAPI H_SetVolume(IDirectSoundBuffer* self, LONG v)
	{
		__try { Rec_Volume(self, v); }
		__except (EXCEPTION_EXECUTE_HANDLER) {}
		return real_SetVolume(self, v);
	}

	HRESULT WINAPI H_SetPan(IDirectSoundBuffer* self, LONG p)
	{
		__try { Rec_Pan(self, p); }
		__except (EXCEPTION_EXECUTE_HANDLER) {}
		return real_SetPan(self, p);
	}

	void HookBufferVtable(IDirectSoundBuffer* b)
	{
		if (g_bufferVtableHooked || !b)
			return;
		g_bufferVtableHooked = true;
		// Every IDirectSoundBuffer from the same device shares one vtable, so a
		// single patch covers all of them; buffers are told apart by `this`.
		void** vt = *(void***)b;
		real_Lock      = (LockFn)vt[11];
		real_Play      = (PlayFn)vt[12];
		real_SetVolume = (SetVolumeFn)vt[15];
		real_SetPan    = (SetPanFn)vt[17];
		real_Stop      = (StopFn)vt[18];
		real_Unlock    = (UnlockFn)vt[19];

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)real_Lock,      (PVOID)H_Lock);
		DetourAttach(&(PVOID&)real_Play,      (PVOID)H_Play);
		DetourAttach(&(PVOID&)real_SetVolume, (PVOID)H_SetVolume);
		DetourAttach(&(PVOID&)real_SetPan,    (PVOID)H_SetPan);
		DetourAttach(&(PVOID&)real_Stop,      (PVOID)H_Stop);
		DetourAttach(&(PVOID&)real_Unlock,    (PVOID)H_Unlock);
		DetourTransactionCommit();
	}

	HRESULT WINAPI H_CreateSoundBuffer(IDirectSound* self, LPCDSBUFFERDESC desc,
	                                   LPDIRECTSOUNDBUFFER* out, LPUNKNOWN unk)
	{
		DSBUFFERDESC local{};
		LPCDSBUFFERDESC use = desc;
		if (desc && desc->dwSize >= sizeof(DSBUFFERDESC))
		{
			local = *desc;
			// Keep the stream ALIVE when the game window loses focus, or the
			// capture faithfully records silence. Also request the controls we
			// read, so SetVolume/SetPan cannot fail for want of a flag.
			local.dwFlags |= DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLVOLUME |
			                 DSBCAPS_CTRLPAN | DSBCAPS_GETCURRENTPOSITION2;
			use = &local;
		}

		HRESULT hr = real_CreateSoundBuffer(self, use, out, unk);
		if (FAILED(hr) && use != desc)
			hr = real_CreateSoundBuffer(self, desc, out, unk);   // retry unmodified
		if (FAILED(hr) || !out || !*out)
			return hr;

		// The PRIMARY buffer carries no PCM of its own; only secondaries matter.
		if (use && (use->dwFlags & DSBCAPS_PRIMARYBUFFER))
			return hr;
		__try
		{
			HookBufferVtable(*out);
			Rec_NewBuffer(*out, use ? use->dwBufferBytes : 0);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {}
		return hr;
	}

	// ---- mixer (our thread) -------------------------------------------------

	// Nearest-neighbour rate conversion. Not pitch shifting: without it a
	// 22050 Hz source played through a 44100 Hz device is an octave out.
	void MixInto(std::vector<int>& acc, BufState& st, int frames)
	{
		const WAVEFORMATEX& f = st.fmt;
		if (!f.nChannels || !f.nSamplesPerSec || st.pcm.empty())
			return;
		const int bytesPerSample = f.wBitsPerSample / 8;
		const int frameBytes = bytesPerSample * f.nChannels;
		if (frameBytes <= 0)
			return;
		const double step = (double)f.nSamplesPerSec / (double)kOutRate;
		const size_t totalFrames = st.pcm.size() / frameBytes;
		if (!totalFrames)
			return;

		const float gain = GainFromDb(st.volume);
		// DSBPAN is hundredths of a dB of ATTENUATION applied to one side.
		float gl = 1.0f, gr = 1.0f;
		if (st.pan > 0) gl = GainFromDb(-st.pan);
		else if (st.pan < 0) gr = GainFromDb(st.pan);

		for (int i = 0; i < frames; ++i)
		{
			size_t idx = (size_t)st.cursor;
			if (idx >= totalFrames)
			{
				if (!st.looping) { st.playing = false; return; }
				st.cursor = 0.0;
				idx = 0;
			}
			const BYTE* src = st.pcm.data() + idx * frameBytes;
			int l = 0, r = 0;
			if (bytesPerSample == 2)
			{
				const short* s = (const short*)src;
				l = s[0];
				r = (f.nChannels > 1) ? s[1] : s[0];
			}
			else if (bytesPerSample == 1)
			{
				l = ((int)src[0] - 128) << 8;
				r = (f.nChannels > 1) ? (((int)src[1] - 128) << 8) : l;
			}
			acc[i * 2 + 0] += (int)(l * gain * gl);
			acc[i * 2 + 1] += (int)(r * gain * gr);
			st.cursor += step;
		}
	}

	// WASAPI loopback on the default render endpoint. Captures the native mix
	// that DirectSound actually produced. Runs only during a verify window.
	DWORD WINAPI RefThread(LPVOID)
	{
		CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		IMMDeviceEnumerator* en = nullptr;
		IMMDevice* dev = nullptr;
		IAudioClient* ac = nullptr;
		IAudioCaptureClient* cc = nullptr;
		WAVEFORMATEX* wf = nullptr;

		HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
		                              __uuidof(IMMDeviceEnumerator), (void**)&en);
		if (SUCCEEDED(hr)) hr = en->GetDefaultAudioEndpoint(eRender, eConsole, &dev);
		if (SUCCEEDED(hr)) hr = dev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&ac);
		if (SUCCEEDED(hr)) hr = ac->GetMixFormat(&wf);
		if (SUCCEEDED(hr))
		{
			g_refRate.store((int)wf->nSamplesPerSec, std::memory_order_relaxed);
			g_refCh.store((int)wf->nChannels, std::memory_order_relaxed);
			hr = ac->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK,
			                    10000000, 0, wf, nullptr);
		}
		if (SUCCEEDED(hr)) hr = ac->GetService(__uuidof(IAudioCaptureClient), (void**)&cc);
		if (SUCCEEDED(hr)) hr = ac->Start();

		if (SUCCEEDED(hr))
		{
			const bool isFloat = wf->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
				(wf->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
				 ((WAVEFORMATEXTENSIBLE*)wf)->SubFormat.Data1 == 3);
			const int ch = wf->nChannels;
			while (g_refRun.load(std::memory_order_relaxed))
			{
				UINT32 packet = 0;
				if (FAILED(cc->GetNextPacketSize(&packet)) || !packet) { Sleep(5); continue; }
				BYTE* data = nullptr; UINT32 frames = 0; DWORD flags = 0;
				if (FAILED(cc->GetBuffer(&data, &frames, &flags, nullptr, nullptr)))
					continue;
				{
					std::lock_guard<std::mutex> lk(g_capMx);
					for (UINT32 i = 0; i < frames; ++i)
					{
						// Down-mix to stereo and to int16 so both sides of the
						// comparison are in the same units.
						float l = 0.f, r = 0.f;
						if (flags & AUDCLNT_BUFFERFLAGS_SILENT) { l = r = 0.f; }
						else if (isFloat)
						{
							const float* f = (const float*)(data + (size_t)i * ch * 4);
							l = f[0];
							r = (ch > 1) ? f[1] : f[0];
						}
						else
						{
							const short* v = (const short*)(data + (size_t)i * ch * 2);
							l = v[0] / 32768.0f;
							r = (ch > 1) ? v[1] / 32768.0f : v[0] / 32768.0f;
						}
						auto clamp16 = [](float x) {
							int v = (int)(x * 32767.0f);
							if (v > 32767) v = 32767;
							if (v < -32768) v = -32768;
							return (short)v;
						};
						g_capRef.push_back(clamp16(l));
						g_capRef.push_back(clamp16(r));
					}
				}
				cc->ReleaseBuffer(frames);
			}
			ac->Stop();
		}

		if (wf) CoTaskMemFree(wf);
		if (cc) cc->Release();
		if (ac) ac->Release();
		if (dev) dev->Release();
		if (en) en->Release();
		CoUninitialize();
		return 0;
	}

	bool WriteWav(const char* path, const std::vector<short>& pcm, int rate, int ch)
	{
		HANDLE h = CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
		                       FILE_ATTRIBUTE_NORMAL, nullptr);
		if (h == INVALID_HANDLE_VALUE)
			return false;
		const DWORD dataBytes = (DWORD)(pcm.size() * sizeof(short));
		const DWORD byteRate = (DWORD)(rate * ch * 2);
		BYTE hdr[44];
		memcpy(hdr, "RIFF", 4);
		*(DWORD*)(hdr + 4) = 36 + dataBytes;
		memcpy(hdr + 8, "WAVEfmt ", 8);
		*(DWORD*)(hdr + 16) = 16;
		*(WORD*)(hdr + 20) = 1;
		*(WORD*)(hdr + 22) = (WORD)ch;
		*(DWORD*)(hdr + 24) = (DWORD)rate;
		*(DWORD*)(hdr + 28) = byteRate;
		*(WORD*)(hdr + 32) = (WORD)(ch * 2);
		*(WORD*)(hdr + 34) = 16;
		memcpy(hdr + 36, "data", 4);
		*(DWORD*)(hdr + 40) = dataBytes;
		DWORD wrote = 0;
		WriteFile(h, hdr, sizeof(hdr), &wrote, nullptr);
		if (dataBytes)
			WriteFile(h, pcm.data(), dataBytes, &wrote, nullptr);
		CloseHandle(h);
		return true;
	}

	DWORD WINAPI MixThread(LPVOID)
	{
		HWAVEOUT hwo = nullptr;
		WAVEFORMATEX wf{};
		wf.wFormatTag = WAVE_FORMAT_PCM;
		wf.nChannels = kOutCh;
		wf.nSamplesPerSec = kOutRate;
		wf.wBitsPerSample = 16;
		wf.nBlockAlign = kOutCh * 2;
		wf.nAvgBytesPerSec = kOutRate * wf.nBlockAlign;
		if (waveOutOpen(&hwo, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
			return 1;

		std::vector<short> bufs[kWaveBuffers];
		WAVEHDR hdr[kWaveBuffers]{};
		for (int i = 0; i < kWaveBuffers; ++i)
		{
			bufs[i].assign((size_t)kMixFrames * kOutCh, 0);
			hdr[i].lpData = (LPSTR)bufs[i].data();
			hdr[i].dwBufferLength = (DWORD)(bufs[i].size() * sizeof(short));
			waveOutPrepareHeader(hwo, &hdr[i], sizeof(WAVEHDR));
			hdr[i].dwFlags |= WHDR_DONE;
		}

		std::vector<int> acc((size_t)kMixFrames * kOutCh, 0);
		int next = 0;
		while (g_run.load(std::memory_order_relaxed))
		{
			if (!(hdr[next].dwFlags & WHDR_DONE)) { Sleep(2); continue; }
			waveOutUnprepareHeader(hwo, &hdr[next], sizeof(WAVEHDR));

			std::fill(acc.begin(), acc.end(), 0);
			int active = 0;
			{
				std::lock_guard<std::mutex> lk(g_mx);
				for (auto& kv : g_bufs)
				{
					if (!kv.second.playing)
						continue;
					++active;
					MixInto(acc, kv.second, kMixFrames);
				}
			}
			g_active.store(active, std::memory_order_relaxed);
			if (active)
				g_mixed.fetch_add(1, std::memory_order_relaxed);

			// Audible only while one of OUR windows is foreground -- the point of
			// the exercise is to hear the game while looking at the panel, not to
			// have it play over whatever else you are doing.
			const bool audible = g_playLocal.load(std::memory_order_relaxed)
			                  && OurProcessHasFocus();
			short* dst = bufs[next].data();
			for (size_t i = 0; i < acc.size(); ++i)
			{
				int v = audible ? acc[i] : 0;
				if (v > 32767) v = 32767;
				if (v < -32768) v = -32768;
				dst[i] = (short)v;
			}

			// Tee our mix into the verification buffer BEFORE the audibility
			// gate, so a verify run measures what the mixer produced rather than
			// what happened to be audible at the time.
			if (g_capturing.load(std::memory_order_relaxed))
			{
				std::lock_guard<std::mutex> lk(g_capMx);
				for (size_t i = 0; i < acc.size(); ++i)
				{
					int v = acc[i];
					if (v > 32767) v = 32767;
					if (v < -32768) v = -32768;
					g_capOurs.push_back((short)v);
				}
			}

			hdr[next].dwFlags = 0;
			hdr[next].dwBufferLength = (DWORD)(bufs[next].size() * sizeof(short));
			waveOutPrepareHeader(hwo, &hdr[next], sizeof(WAVEHDR));
			waveOutWrite(hwo, &hdr[next], sizeof(WAVEHDR));
			next = (next + 1) % kWaveBuffers;
		}

		waveOutReset(hwo);
		for (int i = 0; i < kWaveBuffers; ++i)
			waveOutUnprepareHeader(hwo, &hdr[i], sizeof(WAVEHDR));
		waveOutClose(hwo);
		return 0;
	}
}

// ---------------------------------------------------------------- public ----

extern "C" void D2AudioCap_Install()
{
	static bool done = false;
	if (done)
		return;
	done = true;

	// D2Sound creates the device via DirectSoundCreate. Rather than detour that
	// (and every possible creation path), make one ourselves purely to read the
	// vtable -- IDirectSound's layout is identical for every instance.
	// Resolved dynamically rather than linked: dsound.dll is already loaded in
	// this process (the game uses it), and a link-time dependency on dsound.lib
	// would make the whole debugger fail to build wherever that lib is absent.
	HMODULE dsdll = GetModuleHandleA("dsound.dll");
	if (!dsdll)
		dsdll = LoadLibraryA("dsound.dll");
	if (!dsdll)
		return;
	using DSCreateFn = HRESULT(WINAPI*)(LPCGUID, IDirectSound**, LPUNKNOWN);
	auto dsCreate = (DSCreateFn)GetProcAddress(dsdll, "DirectSoundCreate");
	if (!dsCreate)
		return;

	IDirectSound* ds = nullptr;
	if (FAILED(dsCreate(nullptr, &ds, nullptr)) || !ds)
		return;
	void** vt = *(void***)ds;
	real_CreateSoundBuffer = (CreateSoundBufferFn)vt[3];
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)real_CreateSoundBuffer, (PVOID)H_CreateSoundBuffer);
	DetourTransactionCommit();
	ds->Release();

	g_run.store(true, std::memory_order_relaxed);
	g_thread = CreateThread(nullptr, 0, MixThread, nullptr, 0, nullptr);
}

extern "C" void D2AudioCap_SetEnabled(int on) { g_enabled.store(on != 0, std::memory_order_relaxed); }
extern "C" int  D2AudioCap_IsEnabled() { return g_enabled.load(std::memory_order_relaxed) ? 1 : 0; }
extern "C" void D2AudioCap_SetPlayLocal(int on) { g_playLocal.store(on != 0, std::memory_order_relaxed); }
extern "C" int  D2AudioCap_PlayLocal() { return g_playLocal.load(std::memory_order_relaxed) ? 1 : 0; }

// Run BOTH paths for `seconds` and write two WAVs: ours (the mixer) and the
// reference (WASAPI loopback of the native mix). Returns frames captured on
// each side, or -1 on failure.
//
// Mutes local playback for the duration -- our own waveOut output is part of
// this process's audio and would otherwise be recorded as the "native" signal,
// which would compare our mix against itself and always look perfect.
extern "C" int D2AudioCap_Verify(const char* dir, int seconds,
                                 int* oursFrames, int* refFrames)
{
	if (!dir || !*dir)
		return -1;
	if (seconds <= 0 || seconds > 60)
		seconds = 5;

	const bool prevLocal = g_playLocal.load(std::memory_order_relaxed);
	g_playLocal.store(false, std::memory_order_relaxed);
	{
		std::lock_guard<std::mutex> lk(g_capMx);
		g_capOurs.clear();
		g_capRef.clear();
	}
	g_refRun.store(true, std::memory_order_relaxed);
	HANDLE rt = CreateThread(nullptr, 0, RefThread, nullptr, 0, nullptr);
	g_capturing.store(true, std::memory_order_relaxed);

	Sleep((DWORD)seconds * 1000);

	g_capturing.store(false, std::memory_order_relaxed);
	g_refRun.store(false, std::memory_order_relaxed);
	if (rt) { WaitForSingleObject(rt, 3000); CloseHandle(rt); }
	g_playLocal.store(prevLocal, std::memory_order_relaxed);

	char pOurs[MAX_PATH], pRef[MAX_PATH];
	wsprintfA(pOurs, "%s\\audio_ours.wav", dir);
	wsprintfA(pRef, "%s\\audio_native.wav", dir);

	std::lock_guard<std::mutex> lk(g_capMx);
	WriteWav(pOurs, g_capOurs, kOutRate, kOutCh);
	WriteWav(pRef, g_capRef, g_refRate.load(std::memory_order_relaxed) ?
	         g_refRate.load(std::memory_order_relaxed) : kOutRate, 2);
	if (oursFrames) *oursFrames = (int)(g_capOurs.size() / kOutCh);
	if (refFrames)  *refFrames = (int)(g_capRef.size() / 2);
	return 1;
}

extern "C" int D2AudioCap_RefRate() { return g_refRate.load(std::memory_order_relaxed); }

extern "C" void D2AudioCap_Counters(unsigned long* writes, unsigned long* plays,
                                    unsigned long* mixTicks, int* buffers, int* active)
{
	if (writes)   *writes = g_locks.load(std::memory_order_relaxed);
	if (plays)    *plays = g_plays.load(std::memory_order_relaxed);
	if (mixTicks) *mixTicks = g_mixed.load(std::memory_order_relaxed);
	if (active)   *active = g_active.load(std::memory_order_relaxed);
	if (buffers)
	{
		std::lock_guard<std::mutex> lk(g_mx);
		*buffers = (int)g_bufs.size();
	}
}
