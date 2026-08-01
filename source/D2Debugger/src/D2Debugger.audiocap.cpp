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
#include <audiopolicy.h>
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
		// The volume the GAME asked for. Distinct from what DirectSound is
		// actually set to: while our mixer is driving, we force the real buffer
		// to silence and mix at this value instead, so the game's intent is
		// preserved without hearing both paths at once.
		LONG   volume = 0;                       // DSBVOLUME_MAX == 0 (dB*100)
		LONG   pan = 0;                          // -10000..10000
		double cursor = 0.0;                     // in SOURCE frames

		// Per-buffer accounting, so a divergence can be attributed to a
		// SPECIFIC buffer instead of guessed at. The two failure modes look
		// identical in the aggregate and completely different here:
		//   pcm silent + plays > 0  -> we never captured this buffer's audio
		//   pcm loud   + plays == 0 -> we have the audio and never played it
		// Recent WRITE and READ events, so "the audio is not where we look for
		// it" can be seen rather than argued. Three hypotheses about these
		// streaming buffers have already died; this records the two facts that
		// actually matter -- where the game put the audio, and where we read.
		struct Ev { char kind; DWORD pos; DWORD bytes; int peak; };
		Ev  ev[32]{};
		int evNext = 0;
		unsigned long writes = 0;                // Lock/Unlock pairs seen
		unsigned long plays = 0;                 // Play() calls seen
		unsigned long framesMixed = 0;           // output frames we contributed
		double energy = 0.0;                     // sum of squares we contributed
	};

	std::mutex g_mx;
	std::map<IDirectSoundBuffer*, BufState> g_bufs;

	std::atomic<bool> g_enabled{ true };
	// OUR MIXER drives the sound. When on, the game's native DirectSound output
	// is silenced and you hear our reproduction instead -- which is the point:
	// you cannot judge a reimplementation you are not listening to. When off,
	// the game's own audio plays and ours is captured for verification only.
	std::atomic<bool> g_playLocal{ false };
	// Duplicate-tracking, so a buffer made by DuplicateSoundBuffer shares the
	// source's audio data. D2 duplicates heavily for concurrent SFX, and every
	// duplicate was previously invisible to us -- a prime suspect for the 19 dB
	// deficit the first shadow run measured.
	using DuplicateFn = HRESULT(WINAPI*)(IDirectSound*, IDirectSoundBuffer*,
	                                     IDirectSoundBuffer**);
	DuplicateFn real_Duplicate = nullptr;
	std::atomic<unsigned long> g_locks{ 0 }, g_plays{ 0 }, g_mixed{ 0 };
	std::atomic<int>  g_active{ 0 };
	// Lock-flag census. DSBLOCK_FROMWRITECURSOR makes DirectSound pick the
	// offset and IGNORE the one passed in, so recording the caller's `off` would
	// scatter every streamed chunk to the wrong place -- most likely offset 0,
	// leaving the rest of the ring silent. Counted rather than assumed.
	std::atomic<unsigned long> g_lockPlain{ 0 }, g_lockWriteCur{ 0 }, g_lockEntire{ 0 };
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
	// NOT hooked -- called. This is DirectSound telling us where it actually is
	// in the buffer, which is the whole point of following it rather than
	// integrating our own clock.
	using GetPosFn = HRESULT(WINAPI*)(IDirectSoundBuffer*, LPDWORD, LPDWORD);
	GetPosFn    real_GetPos = nullptr;
	// DirectSound's OWN view of the volume. The mix we produce is only right if
	// the gain we apply is the gain DirectSound applies -- and our capture is
	// taken pre-output while the reference is taken post-DirectSound, so any
	// disagreement here lands directly in the measured dB gap.
	using GetVolFn = HRESULT(WINAPI*)(IDirectSoundBuffer*, LPLONG);
	GetVolFn    real_GetVolume = nullptr;
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

	// Mute/unmute THIS PROCESS's audio session. Used for the go-quiet-when-you-
	// tab-away behaviour: it covers the game's native output and our own
	// playback in one place, because both belong to the same session.
	void SetSessionMute(bool mute)
	{
		static bool last = false;
		static bool init = false;
		if (init && last == mute)
			return;
		IMMDeviceEnumerator* en = nullptr;
		IMMDevice* dev = nullptr;
		IAudioSessionManager* mgr = nullptr;
		ISimpleAudioVolume* vol = nullptr;
		if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
		                               __uuidof(IMMDeviceEnumerator), (void**)&en)) &&
		    SUCCEEDED(en->GetDefaultAudioEndpoint(eRender, eConsole, &dev)) &&
		    SUCCEEDED(dev->Activate(__uuidof(IAudioSessionManager), CLSCTX_ALL, nullptr,
		                            (void**)&mgr)) &&
		    SUCCEEDED(mgr->GetSimpleAudioVolume(nullptr, FALSE, &vol)))
		{
			vol->SetMute(mute ? TRUE : FALSE, nullptr);
			last = mute; init = true;
		}
		if (vol) vol->Release();
		if (mgr) mgr->Release();
		if (dev) dev->Release();
		if (en) en->Release();
	}

	// Peak |sample| over a byte range, sub-sampled. Diagnostics only, so it runs
	// on a stride rather than every sample -- a chunk can be tens of KB and this
	// is called from the game's audio thread.
	int PeakOf(const BYTE* p, size_t bytes, int bits)
	{
		int peak = 0;
		if (!p || !bytes)
			return 0;
		if (bits == 16)
		{
			const short* v = (const short*)p;
			const size_t n = bytes / 2;
			for (size_t i = 0; i < n; i += 8)
			{
				int a = v[i] < 0 ? -v[i] : v[i];
				if (a > peak) peak = a;
			}
		}
		else
		{
			for (size_t i = 0; i < bytes; i += 8)
			{
				int a = ((int)p[i] - 128) * 256;
				if (a < 0) a = -a;
				if (a > peak) peak = a;
			}
		}
		return peak;
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
		pl.offset = off;          // already resolved by the caller
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
			// Clip rather than skip: a chunk that runs past the end is normal for
			// a ring, and dropping it entirely leaves a silent hole.
			if (pl.p1 && pl.b1 && pl.offset < st.pcm.size())
			{
				const size_t n = (pl.offset + pl.b1 <= st.pcm.size())
				               ? pl.b1 : (st.pcm.size() - pl.offset);
				memcpy(st.pcm.data() + pl.offset, pl.p1, n);
			}
			if (pl.p2 && pl.b2 && pl.b2 <= st.pcm.size())
				memcpy(st.pcm.data(), pl.p2, pl.b2);
			st.writes++;
			if (pl.p1 && pl.b1)
			{
				BufState::Ev& e = st.ev[st.evNext % 32];
				e.kind = 'W';
				e.pos = pl.offset;
				e.bytes = pl.b1;
				e.peak = PeakOf((const BYTE*)pl.p1, pl.b1, st.fmt.wBitsPerSample);
				st.evNext++;
			}
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
			it->second.plays++;
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

	// Its own frame: __try cannot coexist with object unwinding (C2712), and the
	// callers below all hold locks or vectors.
	void SafeSetVolume(IDirectSoundBuffer* b, LONG v)
	{
		__try { if (real_SetVolume && b) real_SetVolume(b, v); }
		__except (EXCEPTION_EXECUTE_HANDLER) {}
	}

	// Silence (or restore) every registered buffer's REAL DirectSound volume.
	// Called when the mixer is switched on or off. The game's requested volume
	// is untouched in BufState, so our mix still honours it.
	void ApplyNativeMute(bool mute)
	{
		std::vector<std::pair<IDirectSoundBuffer*, LONG>> work;
		{
			std::lock_guard<std::mutex> lk(g_mx);
			for (auto& kv : g_bufs)
				work.emplace_back(kv.first, mute ? DSBVOLUME_MIN : kv.second.volume);
		}
		// Outside the lock: SetVolume re-enters our own hook.
		for (auto& w : work)
			SafeSetVolume(w.first, w.second);
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
		{
			std::lock_guard<std::mutex> lk(g_mx);
			BufState st;
			st.fmt = wf;
			st.pcm.assign(bytes, 0);
			g_bufs[b] = std::move(st);
		}
		// BACK-FILL. D2 preloads sounds at startup, so a buffer may already be
		// full before our hook ever saw a Lock on it -- those played natively
		// and were silent in our mix. Read what is already there.
		void* p1 = nullptr; DWORD b1 = 0; void* p2 = nullptr; DWORD b2 = 0;
		if (real_Lock && SUCCEEDED(real_Lock(b, 0, 0, &p1, &b1, &p2, &b2,
		                                     DSBLOCK_ENTIREBUFFER)))
		{
			{
				std::lock_guard<std::mutex> lk(g_mx);
				auto it = g_bufs.find(b);
				if (it != g_bufs.end() && p1 && b1 && b1 <= it->second.pcm.size())
					memcpy(it->second.pcm.data(), p1, b1);
			}
			if (real_Unlock)
				real_Unlock(b, p1, b1, p2, b2);
		}
		if (g_playLocal.load(std::memory_order_relaxed))
			SafeSetVolume(b, DSBVOLUME_MIN);      // born silent while we drive
	}


	HRESULT WINAPI H_Lock(IDirectSoundBuffer* self, DWORD off, DWORD bytes,
	                      LPVOID* p1, LPDWORD b1, LPVOID* p2, LPDWORD b2, DWORD flags)
	{
		HRESULT hr = real_Lock(self, off, bytes, p1, b1, p2, b2, flags);
		if (FAILED(hr) || !g_enabled.load(std::memory_order_relaxed))
			return hr;

		// WHERE did this lock actually land? With DSBLOCK_FROMWRITECURSOR the
		// answer is "wherever DirectSound's write cursor is", and `off` is
		// ignored -- so trusting `off` files every streamed chunk at the wrong
		// offset. Ask for the real position instead. Queried here, on the game
		// thread, with no lock of ours held.
		DWORD offset = off;
		if (flags & DSBLOCK_ENTIREBUFFER)
		{
			offset = 0;
			g_lockEntire.fetch_add(1, std::memory_order_relaxed);
		}
		else if (flags & DSBLOCK_FROMWRITECURSOR)
		{
			DWORD play = 0, write = 0;
			if (real_GetPos && SUCCEEDED(real_GetPos(self, &play, &write)))
				offset = write;
			g_lockWriteCur.fetch_add(1, std::memory_order_relaxed);
		}
		else
		{
			g_lockPlain.fetch_add(1, std::memory_order_relaxed);
		}

		__try { Rec_Lock(self, p1, b1, p2, b2, offset, flags); }
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
		// While OUR mixer drives, the native buffer stays silent -- enforced
		// HERE rather than by a one-off sweep.
		//
		// D2 re-sets volume before nearly every Play (that is how it does
		// distance attenuation), so ApplyNativeMute's DSBVOLUME_MIN was being
		// overwritten within milliseconds and the game kept sounding: measured
		// at rms 1916 with the "mute" supposedly applied, and audible. Rewriting
		// the value in the hook makes it stick for as long as we are driving,
		// while Rec_Volume above still records what the game ASKED for so our
		// mix reproduces its intent.
		if (g_playLocal.load(std::memory_order_relaxed))
			return real_SetVolume(self, DSBVOLUME_MIN);
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
		real_SetPan    = (SetPanFn)vt[16];
		real_GetPos    = (GetPosFn)vt[4];
		real_GetVolume = (GetVolFn)vt[6];
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

	void Rec_Duplicate(IDirectSoundBuffer* src, IDirectSoundBuffer* dst)
	{
		std::lock_guard<std::mutex> lk(g_mx);
		auto it = g_bufs.find(src);
		if (it == g_bufs.end())
			return;
		BufState st;
		st.fmt = it->second.fmt;
		st.pcm = it->second.pcm;          // same audio, independent play state
		st.volume = it->second.volume;
		g_bufs[dst] = std::move(st);
	}

	// A duplicate SHARES the original's audio data, so it needs the same shadow
	// PCM -- but its own play cursor, volume and pan.
	HRESULT WINAPI H_Duplicate(IDirectSound* self, IDirectSoundBuffer* src,
	                           IDirectSoundBuffer** out)
	{
		HRESULT hr = real_Duplicate(self, src, out);
		if (FAILED(hr) || !out || !*out)
			return hr;
		__try { Rec_Duplicate(src, *out); }
		__except (EXCEPTION_EXECUTE_HANDLER) {}
		if (g_playLocal.load(std::memory_order_relaxed))
			SafeSetVolume(*out, DSBVOLUME_MIN);
		return hr;
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

		// One READ event per tick: where we started and how loud the data there
		// actually is. Paired against the W events, this is the whole diagnosis.
		{
			const size_t startByte = (size_t)st.cursor * frameBytes;
			const size_t span = (size_t)(frames * step) * frameBytes;
			BufState::Ev& e = st.ev[st.evNext % 32];
			e.kind = 'R';
			e.pos = (DWORD)startByte;
			e.bytes = (DWORD)span;
			e.peak = (startByte < st.pcm.size())
			       ? PeakOf(st.pcm.data() + startByte,
			                (std::min)(span, st.pcm.size() - startByte),
			                st.fmt.wBitsPerSample)
			       : -1;                       // read position past the end
			st.evNext++;
		}

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
				// Wrap rather than stop. DirectSound owns "is it still playing"
				// and tells us via Stop(); deciding it ourselves from a cursor we
				// no longer integrate would cut sounds off early, and a buffer we
				// wrongly marked finished never sounds again.
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
			const int ol = (int)(l * gain * gl);
			const int orr = (int)(r * gain * gr);
			acc[i * 2 + 0] += ol;
			acc[i * 2 + 1] += orr;
			// Energy AFTER gain/pan -- what this buffer actually contributed to
			// the mix, not how loud its source data happens to be.
			st.energy += ((double)ol * ol + (double)orr * orr);
			st.framesMixed++;
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

			// PHASE 1: which buffers are sounding. Lock held only to copy
			// pointers out.
			std::vector<IDirectSoundBuffer*> sounding;
			{
				std::lock_guard<std::mutex> lk(g_mx);
				for (auto& kv : g_bufs)
					if (kv.second.playing)
						sounding.push_back(kv.first);
			}

			// PHASE 2: ask DirectSound where it actually is -- OUTSIDE our lock.
			//
			// The game's audio thread takes g_mx in the hook helpers, and
			// calling into DirectSound while holding that same lock is the
			// shape that deadlocks. It costs one small vector per tick to avoid
			// the question entirely, and a deadlock in the game's audio path is
			// exactly the class of bug that froze the game earlier today.
			//
			// Silencing a buffer does not pause it: volume and playback are
			// independent, so the cursor keeps advancing while our mixer drives.
			std::vector<std::pair<IDirectSoundBuffer*, DWORD>> cursors;
			cursors.reserve(sounding.size());
			if (real_GetPos)
			{
				for (IDirectSoundBuffer* b : sounding)
				{
					DWORD play = 0, write = 0;
					if (SUCCEEDED(real_GetPos(b, &play, &write)))
						cursors.emplace_back(b, play);
				}
			}

			// PHASE 3: mix from where DirectSound is reading.
			{
				std::lock_guard<std::mutex> lk(g_mx);
				for (auto& c : cursors)
				{
					auto it = g_bufs.find(c.first);
					if (it == g_bufs.end())
						continue;
					const BufState& st = it->second;
					const int frameBytes = (st.fmt.wBitsPerSample / 8) * st.fmt.nChannels;
					if (frameBytes > 0)
						it->second.cursor = (double)(c.second / frameBytes);
				}
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
			// Audible only while one of OUR windows is foreground -- true for the
			// game's native output and for our mix alike, which is why the mute
			// is applied to the whole process session rather than per-path.
			const bool focused = OurProcessHasFocus();
			if (!g_capturing.load(std::memory_order_relaxed))
				SetSessionMute(!focused);
			const bool audible = g_playLocal.load(std::memory_order_relaxed) && focused;
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
	real_Duplicate = (DuplicateFn)vt[4];
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)real_CreateSoundBuffer, (PVOID)H_CreateSoundBuffer);
	DetourAttach(&(PVOID&)real_Duplicate, (PVOID)H_Duplicate);
	DetourTransactionCommit();
	ds->Release();

	g_run.store(true, std::memory_order_relaxed);
	g_thread = CreateThread(nullptr, 0, MixThread, nullptr, 0, nullptr);
}

extern "C" void D2AudioCap_SetEnabled(int on) { g_enabled.store(on != 0, std::memory_order_relaxed); }
extern "C" int  D2AudioCap_IsEnabled() { return g_enabled.load(std::memory_order_relaxed) ? 1 : 0; }
extern "C" void D2AudioCap_SetPlayLocal(int on)
{
	const bool want = on != 0;
	if (g_playLocal.exchange(want, std::memory_order_relaxed) == want)
		return;
	// Hearing both paths at once is worse than either: they are ~94 ms apart and
	// comb-filter against each other. Whichever is driving, the other is silent.
	ApplyNativeMute(want);
}
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

	// The reference must be the NATIVE mix, so restore the game's real volumes
	// and stop our own playback for the duration. Capturing while our mixer
	// drives would record our mix as the "native" signal and score a perfect,
	// meaningless match.
	const bool prevLocal = g_playLocal.load(std::memory_order_relaxed);
	g_playLocal.store(false, std::memory_order_relaxed);
	ApplyNativeMute(false);
	SetSessionMute(false);          // the endpoint must actually be rendering
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
	ApplyNativeMute(prevLocal);     // back to whichever path was driving

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

// Per-buffer breakdown as JSON. Written into `out` (caller-sized).
//
// `pcmPeak` is the loudest sample in our SHADOW copy: zero means we hold no
// audio for that buffer at all, whatever the game did with it. `rms` is what
// the buffer contributed to our mix. Comparing the two columns is the whole
// point -- it separates "never captured the data" from "captured it, never
// played it", which the aggregate 18 dB deficit cannot distinguish.
extern "C" int D2AudioCap_BuffersJson(char* out, int cap)
{
	if (!out || cap < 64)
		return 0;
	struct Row
	{
		const void* id; int rate; int ch; int bits; size_t bytes;
		unsigned long writes, plays, frames; double rms; int peak; bool playing, looping;
		LONG vol, pan; float gain; LONG dsVol;
	};
	std::vector<Row> rows;
	{
		std::lock_guard<std::mutex> lk(g_mx);
		rows.reserve(g_bufs.size());
		for (auto& kv : g_bufs)
		{
			const BufState& st = kv.second;
			int peak = 0;
			if (st.fmt.wBitsPerSample == 16)
			{
				const short* v = (const short*)st.pcm.data();
				const size_t n = st.pcm.size() / 2;
				for (size_t i = 0; i < n; ++i)
				{
					int a = v[i] < 0 ? -v[i] : v[i];
					if (a > peak) peak = a;
				}
			}
			else
			{
				for (size_t i = 0; i < st.pcm.size(); ++i)
				{
					int a = (int)st.pcm[i] - 128;
					if (a < 0) a = -a;
					if (a * 256 > peak) peak = a * 256;
				}
			}
			Row r{};
			r.id = kv.first;
			r.rate = st.fmt.nSamplesPerSec;
			r.ch = st.fmt.nChannels;
			r.bits = st.fmt.wBitsPerSample;
			r.bytes = st.pcm.size();
			r.writes = st.writes;
			r.plays = st.plays;
			r.frames = st.framesMixed;
			r.rms = st.framesMixed ? sqrt(st.energy / (double)(st.framesMixed * 2)) : 0.0;
			r.peak = peak;
			r.playing = st.playing;
			r.looping = st.looping;
			// The last untested link. Read peaks prove real audio is being read,
			// so anything lost after that is lost HERE.
			r.vol = st.volume;
			r.pan = st.pan;
			r.gain = GainFromDb(st.volume);
			r.dsVol = 1;                 // sentinel: "not read" (valid vols are <= 0)
			rows.push_back(r);
		}
	}
	// Ask DirectSound what IT thinks each volume is -- outside our lock, since
	// the game's audio thread takes the same one in the hook helpers.
	if (real_GetVolume)
	{
		for (Row& r : rows)
		{
			LONG v = 0;
			if (SUCCEEDED(real_GetVolume((IDirectSoundBuffer*)r.id, &v)))
				r.dsVol = v;
		}
	}

	// Loudest contributors first -- and the silent-but-played ones are exactly
	// the tail you want to read.
	std::sort(rows.begin(), rows.end(),
	          [](const Row& a, const Row& b) { return a.rms > b.rms; });

	int n = _snprintf_s(out, cap, _TRUNCATE, "{\"ok\":true,\"buffers\":[");
	bool first = true;
	for (const Row& r : rows)
	{
		if (n > cap - 220)
			break;
		n += _snprintf_s(out + n, cap - n, _TRUNCATE,
			"%s{\"id\":\"0x%p\",\"rate\":%d,\"ch\":%d,\"bits\":%d,\"bytes\":%zu,"
			"\"writes\":%lu,\"plays\":%lu,\"framesMixed\":%lu,"
			"\"rms\":%.3f,\"pcmPeak\":%d,\"vol\":%ld,\"dsVol\":%ld,\"pan\":%ld,\"gain\":%.5f,"
			"\"playing\":%s,\"looping\":%s}",
			first ? "" : ",", r.id, r.rate, r.ch, r.bits, r.bytes,
			r.writes, r.plays, r.frames, r.rms, r.peak, r.vol, r.dsVol, r.pan, r.gain,
			r.playing ? "true" : "false", r.looping ? "true" : "false");
		first = false;
	}
	n += _snprintf_s(out + n, cap - n, _TRUNCATE, "],\"count\":%d}", (int)rows.size());
	return n;
}

extern "C" void D2AudioCap_LockCensus(unsigned long* plain, unsigned long* writeCur,
                                      unsigned long* entire)
{
	if (plain)    *plain = g_lockPlain.load(std::memory_order_relaxed);
	if (writeCur) *writeCur = g_lockWriteCur.load(std::memory_order_relaxed);
	if (entire)   *entire = g_lockEntire.load(std::memory_order_relaxed);
}

// Event trace for the Nth buffer in the /audio/buffers ordering (loudest
// first). W = the game wrote here, R = we read here, with the peak level found
// at that position in our shadow copy.
extern "C" int D2AudioCap_TraceJson(int index, char* out, int cap)
{
	if (!out || cap < 64)
		return 0;
	struct Row { const void* id; int ch; size_t bytes; BufState::Ev ev[32]; int next; int bits; double rms; };
	std::vector<Row> rows;
	{
		std::lock_guard<std::mutex> lk(g_mx);
		for (auto& kv : g_bufs)
		{
			Row r{};
			r.id = kv.first;
			r.ch = kv.second.fmt.nChannels;
			r.bits = kv.second.fmt.wBitsPerSample;
			r.bytes = kv.second.pcm.size();
			memcpy(r.ev, kv.second.ev, sizeof(r.ev));
			r.next = kv.second.evNext;
			r.rms = kv.second.framesMixed
			      ? sqrt(kv.second.energy / (double)(kv.second.framesMixed * 2)) : 0.0;
			rows.push_back(r);
		}
	}
	// MUST match /audio/buffers' ordering or the index means something different
	// in each endpoint -- which is exactly how the first trace came back empty:
	// index 9 there was a buffer that had never been written.
	std::sort(rows.begin(), rows.end(),
	          [](const Row& a, const Row& b) { return a.rms > b.rms; });
	if (index < 0 || index >= (int)rows.size())
		return _snprintf_s(out, cap, _TRUNCATE,
			"{\"ok\":false,\"error\":\"index out of range\",\"count\":%d}", (int)rows.size());

	const Row& r = rows[index];
	int n = _snprintf_s(out, cap, _TRUNCATE,
		"{\"ok\":true,\"id\":\"0x%p\",\"ch\":%d,\"bits\":%d,\"bufferBytes\":%zu,\"events\":[",
		r.id, r.ch, r.bits, r.bytes);
	bool first = true;
	// Oldest first, so the write/read interleaving reads in time order.
	for (int i = 0; i < 32 && n < cap - 120; ++i)
	{
		const BufState::Ev& e = r.ev[(r.next + i) % 32];
		if (!e.kind)
			continue;
		n += _snprintf_s(out + n, cap - n, _TRUNCATE,
			"%s{\"k\":\"%c\",\"pos\":%lu,\"bytes\":%lu,\"peak\":%d}",
			first ? "" : ",", e.kind, e.pos, e.bytes, e.peak);
		first = false;
	}
	n += _snprintf_s(out + n, cap - n, _TRUNCATE, "]}");
	return n;
}

// Silence BOTH paths and record the loopback anyway. Whatever it captures is
// not the game -- it is everything else on the machine.
//
// The reference capture uses the DEFAULT ENDPOINT with AUDCLNT_STREAMFLAGS_
// LOOPBACK, which is system-wide. That was listed as the disqualifying flaw of
// the system-loopback option when the capture design was chosen, and then built
// into the reference side anyway. A contaminated reference inflates its RMS and
// depresses correlation, which is exactly the signature being chased -- so
// measure the contamination before drawing one more conclusion from it.
extern "C" int D2AudioCap_RefNoise(int seconds, int mute, double* rmsOut, int* frames)
{
	if (seconds <= 0 || seconds > 30)
		seconds = 3;
	const bool prevLocal = g_playLocal.load(std::memory_order_relaxed);

	// MUTE THE WHOLE PROCESS SESSION. Per-buffer SetVolume is not sufficient:
	// D2 re-sets volume before each Play (distance attenuation), so our
	// DSBVOLUME_MIN is overwritten within milliseconds and the game keeps
	// sounding. The session mute is applied by the OS downstream of everything
	// this process renders, and loopback captures the endpoint AFTER it -- so
	// whatever survives is provably not us.
	// mute=0 is a PASSIVE monitor: change nothing, just measure what the
	// endpoint is currently rendering. That is what makes "did the native path
	// actually go quiet?" answerable without disturbing the thing being asked
	// about.
	if (mute)
	{
		ApplyNativeMute(true);
		g_playLocal.store(false, std::memory_order_relaxed);
		SetSessionMute(true);
	}
	{
		std::lock_guard<std::mutex> lk(g_capMx);
		g_capRef.clear();
	}
	g_capturing.store(true, std::memory_order_relaxed);
	g_refRun.store(true, std::memory_order_relaxed);
	HANDLE rt = CreateThread(nullptr, 0, RefThread, nullptr, 0, nullptr);

	Sleep((DWORD)seconds * 1000);

	g_refRun.store(false, std::memory_order_relaxed);
	g_capturing.store(false, std::memory_order_relaxed);
	if (rt) { WaitForSingleObject(rt, 3000); CloseHandle(rt); }

	if (mute)
	{
		SetSessionMute(false);
		g_playLocal.store(prevLocal, std::memory_order_relaxed);
		ApplyNativeMute(prevLocal);
	}

	std::lock_guard<std::mutex> lk(g_capMx);
	double acc = 0.0;
	for (short v : g_capRef)
		acc += (double)v * v;
	if (frames) *frames = (int)(g_capRef.size() / 2);
	if (rmsOut) *rmsOut = g_capRef.empty() ? 0.0 : sqrt(acc / (double)g_capRef.size());
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
