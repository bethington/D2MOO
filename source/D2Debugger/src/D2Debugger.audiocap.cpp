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
#include <audioclientactivationparams.h>
#include "detours.h"

#pragma comment(lib, "winmm.lib")

namespace
{
	constexpr int kOutRate = 44100;
	constexpr int kOutCh = 2;
	constexpr int kMixFrames = 441;              // 10 ms per tick

	struct BufState
	{
		WAVEFORMATEX fmt{};
		std::vector<BYTE> pcm;                   // shadow copy (hook mode only)
		// Buffer length in frames. In hook mode this is pcm.size()/frameBytes;
		// in shim mode there IS no shadow copy -- the shim owns the memory and
		// we read windows of it on demand -- so the length has to be carried.
		size_t srcFrames = 0;
		bool   playing = false;
		bool   looping = false;
		// The volume the GAME asked for. Distinct from what DirectSound is
		// actually set to: while our mixer is driving, we force the real buffer
		// to silence and mix at this value instead, so the game's intent is
		// preserved without hearing both paths at once.
		LONG   volume = 0;                       // DSBVOLUME_MAX == 0 (dB*100)
		LONG   pan = 0;                          // -10000..10000
		double cursor = 0.0;                     // in SOURCE frames
		// Take DirectSound's position on the next tick and integrate from there.
		// Set on Play, and whenever our cursor and its cursor genuinely disagree
		// (a seek or a retrigger) rather than merely jitter apart.
		bool   needResync = true;
		// Ran off the end of a non-looping buffer and contributed nothing since.
		// Kept so the sound stays ended instead of being revived by a drift
		// resync every tick, and so end-of-data is counted once rather than
		// every tick for the rest of the sound's life.
		bool   finished = false;

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

	// ---- dsound-headless capture ABI ------------------------------------
	//
	// Mirrors subprojects/dsound-headless/include/dsound_headless_capture.h.
	// Duplicated rather than #included on purpose: including it would make
	// D2Debugger fail to BUILD when that submodule is not checked out, and this
	// is a runtime-optional capability. The version handshake below is what
	// keeps the copies honest -- a layout change bumps the ABI and we refuse it
	// rather than misread the struct.
	struct D2SndCapBuf
	{
		void*         id;
		unsigned int  rate;         // effective (SetFrequency), not the format's
		unsigned int  channels;
		unsigned int  bits;
		unsigned int  frames;
		unsigned int  playFrame;
		int           playing;
		int           looping;
		long          volume;
		long          pan;
	};
	using CapAbiFn      = unsigned int(__stdcall*)(void);
	using CapSnapshotFn = int(__stdcall*)(D2SndCapBuf*, int);
	using CapReadFn     = int(__stdcall*)(void*, unsigned, unsigned, void*, unsigned);
	using CapPrimaryFn  = int(__stdcall*)(long*);
	CapAbiFn      cap_Abi = nullptr;
	CapSnapshotFn cap_Snapshot = nullptr;
	CapReadFn     cap_Read = nullptr;
	CapPrimaryFn  cap_Primary = nullptr;
	// 0 = Detours hooks (real DirectSound), 1 = shim capture API.
	std::atomic<int> g_captureMode{ 0 };

	std::mutex g_mx;
	std::map<IDirectSoundBuffer*, BufState> g_bufs;

	// THE PRIMARY BUFFER. It carries no PCM, so the capture skips it -- but a
	// DirectSound app conventionally applies MASTER volume there, and anything
	// applied to the primary scales the native output while being invisible to a
	// mixer that only sums secondaries. That is the shape of "the in-game volume
	// sliders do nothing to the audio coming through the debugger".
	//
	// Tracked separately rather than added to g_bufs: it must never be mixed as
	// a source, only used as a final gain.
	IDirectSoundBuffer* g_primary = nullptr;
	std::atomic<long> g_primaryVol{ 0 };            // DSBVOLUME, 0 == unity
	std::atomic<unsigned long> g_primaryVolSets{ 0 };

	std::atomic<bool> g_enabled{ true };
	// OUR MIXER drives the sound. When on, the game's native DirectSound output
	// is silenced and you hear our reproduction instead -- which is the point:
	// you cannot judge a reimplementation you are not listening to. When off,
	// the game's own audio plays and ours is captured for verification only.
	std::atomic<bool> g_playLocal{ false };
	// The game window is hidden, so its own output must not be heard. A SECOND,
	// independent reason to silence the native path: the mixer may well be off.
	//
	// D2 stops its audio when it loses focus, which masked this -- a freshly
	// launched game is hidden but still FOCUSED, so it played out loud until you
	// clicked something else and it went quiet by accident.
	std::atomic<bool> g_suppressNative{ false };
	// 0 = MIXER (hook the DirectSound buffers and rebuild the mix ourselves).
	// 1 = LOOPBACK (stand our interference down and tap what the process renders).
	std::atomic<int>  g_srcMode{ 0 };

	// The one predicate both reasons go through. Two independent causes writing
	// the same DirectSound volume is exactly how they end up fighting: whichever
	// cleared last would win and un-mute the other's reason.
	bool NativeMuted()
	{
		// LOOPBACK: never touch the game's output. It is the thing being
		// captured, and muting it captures silence -- measured at RMS 0.48 of
		// 32768 with the game hidden. This deliberately overrides the
		// hidden-mute; the two requirements cannot both hold.
		if (g_srcMode.load(std::memory_order_relaxed) == 1)
			return false;
		return g_playLocal.load(std::memory_order_relaxed)
		    || g_suppressNative.load(std::memory_order_relaxed);
	}
	// Duplicate-tracking, so a buffer made by DuplicateSoundBuffer shares the
	// source's audio data. D2 duplicates heavily for concurrent SFX, and every
	// duplicate was previously invisible to us -- a prime suspect for the 19 dB
	// deficit the first shadow run measured.
	using DuplicateFn = HRESULT(WINAPI*)(IDirectSound*, IDirectSoundBuffer*,
	                                     IDirectSoundBuffer**);
	DuplicateFn real_Duplicate = nullptr;
	std::atomic<unsigned long> g_locks{ 0 }, g_plays{ 0 }, g_mixed{ 0 };
	// One-shots that ran out of data part-way through a tick. Was silently a
	// replay of the sound's own head; now it is a clean stop, and this counts
	// how often it happens so the fix can be shown to be doing something rather
	// than assumed to be.
	std::atomic<unsigned long> g_oneshotEnd{ 0 };
	// Output samples that hit the +/-32767 clamp. Dense polyphony clipping would
	// ALSO sound like distortion, and the two are indistinguishable by ear --
	// this separates them instead of leaving it to argument.
	std::atomic<unsigned long> g_clipped{ 0 };
	// Times we deferred to DirectSound's position instead of our own. Should be
	// roughly one per sound played; anything near the tick rate means the two
	// clocks are genuinely fighting and this fix did not take.
	std::atomic<unsigned long> g_resyncs{ 0 };
	// Worst gap ever seen between our integrated cursor and DirectSound's, in
	// SOURCE frames. This is the number that says whether per-tick resyncing was
	// smearing the audio: at 22050 Hz, 220 frames is one whole 10 ms tick.
	std::atomic<unsigned long> g_driftMax{ 0 };
	// Buffers we thought had ended, which DirectSound then rewound and kept
	// playing -- so they were looping all along and the flag was wrong. Non-zero
	// means trusting DSBSTATUS_LOOPING alone would have cut ambient sound out.
	std::atomic<unsigned long> g_healedLoops{ 0 };
	std::atomic<int>  g_active{ 0 };
	// Lock-flag census. DSBLOCK_FROMWRITECURSOR makes DirectSound pick the
	// offset and IGNORE the one passed in, so recording the caller's `off` would
	// scatter every streamed chunk to the wrong place -- most likely offset 0,
	// leaving the rest of the ring silent. Counted rather than assumed.
	std::atomic<unsigned long> g_lockPlain{ 0 }, g_lockWriteCur{ 0 }, g_lockEntire{ 0 };
	std::atomic<bool> g_run{ false };

	HANDLE g_thread = nullptr;

	// ---- output ring --------------------------------------------------------
	//
	// The mixer used to BE the playback loop: mix a block, hand it to waveOut,
	// wait for waveOut to give the buffer back. That coupled three things with
	// no business being coupled -- the mix clock, the local speaker path, and
	// any future consumer of the stream -- and it stuttered: four 10 ms buffers
	// polled with Sleep(2) is ~40 ms of cushion, paced by whenever the poll
	// happened to notice a buffer was free, with no way to even COUNT a miss.
	//
	// Now the mixer PRODUCES into this ring on its own clock and local playback
	// is just one consumer. The remote-stream encoder (FLAC over the oracle's
	// WebSocket, next phase) is simply the next consumer, reading the same ring
	// at its own cursor. That split is the point: the canonical stream must not
	// depend on a local audio device existing (a Session-0 container has none --
	// measured, DSERR_NODRIVER), and the speaker path must be free to stutter,
	// lag or vanish without touching what a remote client receives.
	//
	// Single writer, each consumer owns its cursor. The writer publishes
	// g_ringWrite with release; readers load it with acquire and never write
	// anything the producer reads. A reader that falls more than the ring's
	// depth behind is trimmed forward by its own logic, not the producer's.
	constexpr size_t kRingFrames = 65536;      // power of two, ~1.49 s at 44.1k
	std::vector<short> g_ring;                 // kRingFrames * kOutCh shorts
	std::atomic<unsigned long long> g_ringWrite{ 0 };   // absolute frames produced

	// Stutter attribution. "It stutters" arrived as two hypotheses -- gaps
	// (output starving) and repeats (cursor logic re-reading source) -- with no
	// counter to split them. These are the counters. Gaps land in underruns
	// (consumer starved) or late ticks (producer missed its own deadline);
	// repeats land in slews/resyncs. Same discipline as oneshotEndMidTick:
	// per-hypothesis counters, let the numbers falsify.
	std::atomic<unsigned long> g_outUnderruns{ 0 };      // times playback ran dry
	std::atomic<unsigned long> g_outUnderrunFrames{ 0 }; // silence inserted, frames
	std::atomic<unsigned long> g_lateTicks{ 0 };         // producer wakes >15 ms apart
	std::atomic<unsigned long> g_maxTickUs{ 0 };         // worst mix-tick duration
	std::atomic<unsigned long> g_maxGapUs{ 0 };          // worst wake-to-wake gap
	std::atomic<unsigned long> g_slews{ 0 };             // gentle cursor corrections
	std::atomic<unsigned long> g_latencyTrims{ 0 };      // consumer snapped forward
	std::atomic<unsigned long> g_ringFill{ 0 };          // local consumer's backlog
	// Frames the producer has published. Against wall time this is its TRUE
	// output rate -- the measurement that caught the fixed-block deficit, and
	// the one that says whether the clock-derived block size fixed it.
	std::atomic<unsigned long long> g_produced{ 0 };
	std::atomic<unsigned long> g_rateResets{ 0 };        // unrecoverable stalls
	// Sampled from the OS, not remembered from what we set. -1 = unread.
	std::atomic<int> g_sessionMuted{ -1 };
	// Did the GLOBALFOCUS patch actually apply? `passthru` non-zero means the
	// descriptor went through untouched and those buffers WILL be silenced by
	// DirectSound on focus loss -- the exact failure this counter exists to
	// make visible instead of inferable.
	std::atomic<unsigned long> g_descPatched{ 0 }, g_descPassthru{ 0 };
	// How far D2AudioCap_Install got, and what Detours said. `descPatched` being
	// zero while the game demonstrably creates buffers means the hook is not on
	// the function being called -- and every stage below is a different reason.
	//   1 dsound module found   2 DirectSoundCreate resolved   3 device created
	//   4 device detour committed   5 buffer vtable detour committed
	std::atomic<unsigned long> g_installStage{ 0 };
	std::atomic<long> g_detourDevErr{ -1 };
	std::atomic<long> g_detourBufErr{ -1 };
	// The address we detoured, so it can be compared against what the shim
	// actually exports/dispatches.
	std::atomic<unsigned long long> g_vtCreateAddr{ 0 };
	std::atomic<unsigned long> g_descSize{ 0 };
	std::atomic<int>  g_renderDeviceOk{ 0 };             // WASAPI consumer has a device
	HANDLE g_renderThread = nullptr;

	// ---- verification ("shadow mode" for audio) -----------------------------
	//
	// Same discipline the dispatchers use on code: run BOTH, capture BOTH,
	// compare. The reference here is the NATIVE mix -- what DirectSound and
	// Windows actually render -- taken by WASAPI loopback. Ours is what the
	// mixer above produces from the same buffer writes.
	//
	// Our own local playback is part of this process's output and would land
	// in the loopback capture, so a verify run MUTES local playback for its
	// duration. Comparing our mix against a recording of our mix would return a
	// perfect score and mean nothing.
	std::mutex g_capMx;
	std::vector<short> g_capOurs;      // our mixer output, interleaved stereo
	std::vector<short> g_capRef;       // WASAPI loopback of the native mix
	std::atomic<bool> g_capturing{ false };
	std::atomic<bool> g_refRun{ false };
	// Continuous loopback capture, separate from the bounded verify runs so the
	// two can never fight over one run-flag.
	std::atomic<bool> g_liveRun{ false };
	HANDLE            g_liveThread = nullptr;
	// Live loopback accounting. Frames prove the tap is running; the sum of
	// squares gives an RMS, which is what says whether it is capturing AUDIO
	// rather than a perfectly healthy stream of silence.
	std::atomic<unsigned long long> g_loopFrames{ 0 };
	std::atomic<unsigned long long> g_loopSumSq{ 0 };
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
	// Is it still sounding? DirectSound knows; we were guessing.
	//
	// A non-looping buffer simply STOPS at its end -- D2 does not have to call
	// Stop() for that to happen, and mostly does not. Tracking `playing` from
	// Play/Stop alone therefore left one-shots marked playing forever, and once
	// the mixer stopped integrating its own cursor there was nothing left to end
	// them: every finished sound wrapped and replayed for the rest of the
	// session. Asking is both correct and simpler than inferring.
	using GetStatusFn = HRESULT(WINAPI*)(IDirectSoundBuffer*, LPDWORD);
	GetStatusFn real_GetStatus = nullptr;
	bool g_bufferVtableHooked = false;

	// Track what each Lock handed out so Unlock knows where to copy FROM. A
	// buffer can be locked in two segments (DirectSound buffers are circular).
	struct PendingLock { void* p1; DWORD b1; void* p2; DWORD b2; DWORD offset; };
	std::map<IDirectSoundBuffer*, PendingLock> g_pending;

	// Loud, never silent: a loopback that quietly fails to activate is
	// indistinguishable from a game that is simply not making noise.
	void AudLog(const char* m)
	{
		char b[256];
		_snprintf_s(b, sizeof(b), _TRUNCATE, "[audiocap] %s\n", m);
		OutputDebugStringA(b);
		FILE* f = nullptr;
		if (fopen_s(&f, "C:\\Users\\benam\\source\\cpp\\D2MOO\\conformance\\behavioral\\overlay_gl_log.txt", "a") == 0 && f)
		{
			fputs(b, f);
			fclose(f);
		}
	}

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
	// NO CACHE. This used to early-return when the requested state matched the
	// last one it BELIEVED it had applied -- and that belief is only updated on
	// the success path, so a single failed COM call (or anything else changing
	// the session mute) left the cache lying and NOTHING ever re-applied.
	// Measured 2026-08-01: after one `/audio/refnoise {"mute":true}` the session
	// stayed muted for the rest of the session. Every later loopback
	// measurement returned exactly the silence floor (0.48 rms) -- the game, our
	// mixer and the verify reference all "silent" -- which reads as a dead audio
	// path and is really one stale bool. A setter whose cache can disagree with
	// reality and never re-checks is worse than no cache.
	void SetSessionMute(bool mute)
	{
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
		}
		if (vol) vol->Release();
		if (mgr) mgr->Release();
		if (dev) dev->Release();
		if (en) en->Release();
	}

	// Read it back rather than tracking what we think we set -- the whole point
	// of the bug above. Surfaced in /audio as `sessionMuted` so "everything is
	// silent" is one field to check instead of an afternoon of bisecting.
	// -1 = could not read.
	int ReadSessionMute()
	{
		int result = -1;
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
			BOOL m = FALSE;
			if (SUCCEEDED(vol->GetMute(&m)))
				result = m ? 1 : 0;
		}
		if (vol) vol->Release();
		if (mgr) mgr->Release();
		if (dev) dev->Release();
		if (en) en->Release();
		return result;
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
			it->second.needResync = true;
			it->second.finished = false;
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
		if (self && self == g_primary)
		{
			g_primaryVol.store(v, std::memory_order_relaxed);
			g_primaryVolSets.fetch_add(1, std::memory_order_relaxed);
			return;
		}
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

	// Own frame: the caller wraps its body in __try, which MSVC will not allow
	// alongside the lock_guard here (C2712).
	void Rec_Primary(IDirectSoundBuffer* b)
	{
		std::lock_guard<std::mutex> lk(g_mx);
		g_primary = b;
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
			const int fb = (wf.wBitsPerSample / 8) * wf.nChannels;
			st.srcFrames = fb > 0 ? (bytes / (size_t)fb) : 0;
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
		if (NativeMuted())
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
		if (NativeMuted() && self != g_primary)
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
		// Every IDirectSoundBuffer from the same device shares one vtable, so a
		// single patch covers all of them; buffers are told apart by `this`.
		void** vt = *(void***)b;
		real_Lock      = (LockFn)vt[11];
		real_Play      = (PlayFn)vt[12];
		real_SetVolume = (SetVolumeFn)vt[15];
		real_SetPan    = (SetPanFn)vt[16];
		real_GetPos    = (GetPosFn)vt[4];
		real_GetVolume = (GetVolFn)vt[6];
		real_GetStatus = (GetStatusFn)vt[9];
		real_Stop      = (StopFn)vt[18];
		real_Unlock    = (UnlockFn)vt[19];

		// RETRY on contention. Detours transactions are global to the process,
		// and DetourTransactionBegin/Commit return ERROR_INVALID_OPERATION
		// (4317) when another thread holds one. Now that the audio install runs
		// at thread start it overlaps the launcher's own patching, and one
		// failed attempt with no retry left the hooks dead for the whole
		// session (measured: installStage stuck at 3, every menu buffer
		// missed). Bounded and short: this can run on the game's audio thread.
		LONG bufErr = ERROR_INVALID_OPERATION;
		for (int attempt = 0; attempt < 25 && bufErr != NO_ERROR; ++attempt)
		{
			if (attempt)
				Sleep(2);
			if (DetourTransactionBegin() != NO_ERROR)
				continue;
			DetourUpdateThread(GetCurrentThread());
			DetourAttach(&(PVOID&)real_Lock,      (PVOID)H_Lock);
			DetourAttach(&(PVOID&)real_Play,      (PVOID)H_Play);
			DetourAttach(&(PVOID&)real_SetVolume, (PVOID)H_SetVolume);
			DetourAttach(&(PVOID&)real_SetPan,    (PVOID)H_SetPan);
			DetourAttach(&(PVOID&)real_Stop,      (PVOID)H_Stop);
			DetourAttach(&(PVOID&)real_Unlock,    (PVOID)H_Unlock);
			bufErr = DetourTransactionCommit();
		}
		g_detourBufErr.store(bufErr, std::memory_order_relaxed);
		// Only claim the vtable is hooked when it actually is -- the old
		// set-before-trying left a failed attempt looking permanently done.
		g_bufferVtableHooked = (bufErr == NO_ERROR);
		if (bufErr == NO_ERROR)
			g_installStage.store(5, std::memory_order_relaxed);
		else
			AudLog("buffer vtable detour failed after retries");
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
		// NativeMuted(), not g_playLocal: a duplicate born while the game is
		// hidden with the mixer OFF must still be silent. D2 duplicates heavily
		// for concurrent footsteps, so this is a busy path, not a corner.
		if (NativeMuted())
			SafeSetVolume(*out, DSBVOLUME_MIN);
		return hr;
	}

	HRESULT WINAPI H_CreateSoundBuffer(IDirectSound* self, LPCDSBUFFERDESC desc,
	                                   LPDIRECTSOUNDBUFFER* out, LPUNKNOWN unk)
	{
		DSBUFFERDESC local{};
		LPCDSBUFFERDESC use = desc;
		// ACCEPT THE DX7-ERA DESCRIPTOR TOO.
		//
		// This required `dwSize >= sizeof(DSBUFFERDESC)` -- 36 bytes, the modern
		// layout with guid3DAlgorithm. D2 is DirectSound 7 code and passes
		// DSBUFFERDESC1 (20 bytes), so the test failed for EVERY buffer and the
		// descriptor went through untouched: DSBCAPS_GLOBALFOCUS was never
		// actually applied, whatever the comment claimed.
		//
		// Consequence, measured 2026-08-01: DirectSound silences a non-GLOBALFOCUS
		// buffer whenever the cooperative-level window (the GAME window) is not
		// foreground. So the game's own output was dead any time you looked at
		// anything else -- including every /audio/verify run, whose reference is
		// exactly that output. Three separate silence theories (session mute
		// stuck, D2 quieting itself, mix regression) were chased against a
		// reference that could not have carried audio.
		//
		// Copy dwSize bytes and OR the flags in place: dwFlags sits at offset 4
		// in BOTH layouts, and preserving the caller's dwSize keeps DirectSound
		// interpreting the struct the way the caller meant.
		if (desc && desc->dwSize >= 20 && desc->dwSize <= sizeof(DSBUFFERDESC))
		{
			memcpy(&local, desc, desc->dwSize);
			// Keep the stream ALIVE when the game window loses focus, or the
			// capture faithfully records silence. Also request the controls we
			// read, so SetVolume/SetPan cannot fail for want of a flag.
			local.dwFlags |= DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLVOLUME |
			                 DSBCAPS_CTRLPAN | DSBCAPS_GETCURRENTPOSITION2;
			use = &local;
			g_descPatched.fetch_add(1, std::memory_order_relaxed);
		}
		else
		{
			g_descPassthru.fetch_add(1, std::memory_order_relaxed);
		}
		if (desc)
			g_descSize.store(desc->dwSize, std::memory_order_relaxed);

		HRESULT hr = real_CreateSoundBuffer(self, use, out, unk);
		if (FAILED(hr) && use != desc)
			hr = real_CreateSoundBuffer(self, desc, out, unk);   // retry unmodified
		if (FAILED(hr) || !out || !*out)
			return hr;

		// The PRIMARY buffer carries no PCM of its own, so it is never mixed as a
		// source -- but its volume is a master gain over everything, so remember it.
		if (use && (use->dwFlags & DSBCAPS_PRIMARYBUFFER))
		{
			Rec_Primary(*out);
			return hr;
		}
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
				// END OF DATA. What happens next depends on whether the buffer
				// loops, and getting this wrong is audible on every one-shot.
				//
				// A non-looping sound must simply STOP. It used to wrap to zero
				// here regardless, because ending a sound was treated as
				// DirectSound's call via GetStatus -- but GetStatus is only read
				// once per tick, so a one-shot ending part-way through a 10 ms
				// tick got its own opening milliseconds mixed in again to fill
				// the remainder. Music loops and never hit it; footsteps fire
				// several times a second and hit it EVERY time.
				if (!st.looping)
				{
					// Count the transition, not every tick afterwards -- the
					// cursor stays past the end until something resyncs it.
					if (!st.finished)
					{
						st.finished = true;
						g_oneshotEnd.fetch_add(1, std::memory_order_relaxed);
					}
					break;
				}
				// Looping: carry the fractional phase across the seam rather
				// than snapping to 0.0, which would quantise the loop point to a
				// whole frame every lap and buzz at the loop rate.
				st.cursor -= (double)totalFrames;
				if (st.cursor < 0.0)
					st.cursor = 0.0;
				idx = (size_t)st.cursor;
				if (idx >= totalFrames)
				{
					st.cursor = 0.0;
					idx = 0;
				}
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

	// PROCESS loopback -- this process only.
	//
	// The default-endpoint loopback this replaced captured EVERYTHING on the
	// machine. As a reference for "what does the game sound like" that is only
	// correct while nothing else is playing, and as a streaming source it would
	// ship a Windows notification or a browser tab to the remote client with no
	// way to remove it afterwards.
	//
	// Costs a Windows 10 2004 (build 19041) floor, which is free here: the
	// debugger already requires Detours, D3D9 and a modern MSVC build, so the
	// game runs anywhere but this DLL never did. The DIRECT DirectSound capture
	// remains the primary path precisely because it has no such floor.
	class ActivateHandler : public IActivateAudioInterfaceCompletionHandler,
	                        public IAgileObject
	{
	public:
		IAudioClient* client = nullptr;
		HRESULT       result = E_FAIL;
		HANDLE        done = CreateEventA(nullptr, TRUE, FALSE, nullptr);

		~ActivateHandler() { if (done) CloseHandle(done); }

		STDMETHOD(QueryInterface)(REFIID riid, void** ppv) override
		{
			if (!ppv) return E_POINTER;
			if (riid == __uuidof(IUnknown) ||
			    riid == __uuidof(IActivateAudioInterfaceCompletionHandler) ||
			    riid == __uuidof(IAgileObject))
			{
				*ppv = static_cast<IActivateAudioInterfaceCompletionHandler*>(this);
				AddRef();
				return S_OK;
			}
			*ppv = nullptr;
			return E_NOINTERFACE;
		}
		// Stack-allocated and outlived by the wait below, so lifetime is by
		// scope rather than by refcount.
		STDMETHOD_(ULONG, AddRef)() override { return 2; }
		STDMETHOD_(ULONG, Release)() override { return 1; }

		STDMETHOD(ActivateCompleted)(IActivateAudioInterfaceAsyncOperation* op) override
		{
			IUnknown* unk = nullptr;
			HRESULT hrAct = E_FAIL;
			if (op && SUCCEEDED(op->GetActivateResult(&hrAct, &unk)) &&
			    SUCCEEDED(hrAct) && unk)
			{
				unk->QueryInterface(__uuidof(IAudioClient), (void**)&client);
				unk->Release();
			}
			result = hrAct;
			if (done) SetEvent(done);
			return S_OK;
		}
	};

	// param != 0 selects the continuous LIVE capture: stats only, and watching
	// g_liveRun instead of g_refRun so a verify run and the live tap cannot stop
	// each other.
	DWORD WINAPI RefThread(LPVOID param)
	{
		const bool live = param != nullptr;
		CoInitializeEx(nullptr, COINIT_MULTITHREADED);

		// Resolved dynamically so the DLL still LOADS on a Windows older than
		// 2004 -- it simply cannot offer the loopback fallback there, which is
		// the honest degradation given the direct path is primary anyway.
		HMODULE mmd = LoadLibraryA("Mmdevapi.dll");
		using ActivateFn = HRESULT(WINAPI*)(LPCWSTR, REFIID, PROPVARIANT*,
		                                    IActivateAudioInterfaceCompletionHandler*,
		                                    IActivateAudioInterfaceAsyncOperation**);
		auto activate = mmd ? (ActivateFn)GetProcAddress(mmd, "ActivateAudioInterfaceAsync")
		                    : nullptr;
		if (!activate)
		{
			AudLog("process loopback unavailable (needs Windows 10 2004+)");
			CoUninitialize();
			return 1;
		}

		AUDIOCLIENT_ACTIVATION_PARAMS ap{};
		ap.ActivationType = AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK;
		ap.ProcessLoopbackParams.TargetProcessId = GetCurrentProcessId();
		ap.ProcessLoopbackParams.ProcessLoopbackMode =
			PROCESS_LOOPBACK_MODE_INCLUDE_TARGET_PROCESS_TREE;

		PROPVARIANT pv{};
		pv.vt = VT_BLOB;
		pv.blob.cbSize = sizeof(ap);
		pv.blob.pBlobData = (BYTE*)&ap;

		// Process loopback has no mix format to query -- we state the format we
		// want and the engine converts. Matching our mixer's rate keeps the
		// comparison free of an extra resample on the reference side.
		WAVEFORMATEX wf{};
		wf.wFormatTag = WAVE_FORMAT_PCM;
		wf.nChannels = 2;
		wf.nSamplesPerSec = kOutRate;
		wf.wBitsPerSample = 16;
		wf.nBlockAlign = 4;
		wf.nAvgBytesPerSec = kOutRate * 4;

		ActivateHandler handler;
		IActivateAudioInterfaceAsyncOperation* op = nullptr;
		IAudioClient* ac = nullptr;
		IAudioCaptureClient* cc = nullptr;

		HRESULT hr = activate(VIRTUAL_AUDIO_DEVICE_PROCESS_LOOPBACK,
		                      __uuidof(IAudioClient), &pv, &handler, &op);
		if (SUCCEEDED(hr) && handler.done)
			WaitForSingleObject(handler.done, 3000);
		if (op) op->Release();
		ac = handler.client;
		if (!ac)
		{
			AudLog("process loopback activation failed");
			CoUninitialize();
			return 1;
		}

		g_refRate.store(kOutRate, std::memory_order_relaxed);
		g_refCh.store(2, std::memory_order_relaxed);

		hr = ac->Initialize(AUDCLNT_SHAREMODE_SHARED,
		                    AUDCLNT_STREAMFLAGS_LOOPBACK |
		                    AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM,
		                    10000000, 0, &wf, nullptr);
		if (SUCCEEDED(hr)) hr = ac->GetService(__uuidof(IAudioCaptureClient), (void**)&cc);
		if (SUCCEEDED(hr)) hr = ac->Start();

		if (SUCCEEDED(hr))
		{
			while ((live ? g_liveRun : g_refRun).load(std::memory_order_relaxed))
			{
				UINT32 packet = 0;
				if (FAILED(cc->GetNextPacketSize(&packet)) || !packet) { Sleep(5); continue; }
				BYTE* data = nullptr; UINT32 frames = 0; DWORD flags = 0;
				if (FAILED(cc->GetBuffer(&data, &frames, &flags, nullptr, nullptr)))
					continue;
				const bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0;
				const short* v = (const short*)data;
				if (live)
				{
					// Stats only. A verify run is bounded and can afford to keep
					// every sample; this one runs for as long as the mode is on,
					// so accumulating a vector here would grow without limit.
					unsigned long long sq = 0;
					if (!silent)
						for (UINT32 i = 0; i < frames; ++i)
						{
							const long long l = v[i * 2 + 0];
							const long long r = v[i * 2 + 1];
							sq += (unsigned long long)(l * l + r * r);
						}
					g_loopFrames.fetch_add(frames, std::memory_order_relaxed);
					g_loopSumSq.fetch_add(sq, std::memory_order_relaxed);
				}
				else
				{
					std::lock_guard<std::mutex> lk(g_capMx);
					for (UINT32 i = 0; i < frames; ++i)
					{
						g_capRef.push_back(silent ? 0 : v[i * 2 + 0]);
						g_capRef.push_back(silent ? 0 : v[i * 2 + 1]);
					}
				}
				cc->ReleaseBuffer(frames);
			}
			ac->Stop();
		}
		else
		{
			AudLog("process loopback initialize/start failed");
		}

		if (cc) cc->Release();
		if (ac) ac->Release();
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

	// Mix one buffer in SHIM mode.
	//
	// Same arithmetic as MixInto, with one structural difference: there is no
	// shadow copy to index. The shim owns the PCM, so we pull the window this
	// tick actually needs -- a couple of hundred frames -- and mix from that.
	// D2SndCap_Read clamps at the end of the buffer instead of wrapping, so a
	// short read IS the end-of-data signal: exactly what tells a finished
	// one-shot from a loop that should seam.
	void MixShim(std::vector<int>& acc, BufState& st, int frames, void* id)
	{
		const WAVEFORMATEX& f = st.fmt;
		if (!f.nChannels || !f.nSamplesPerSec || !st.srcFrames || !cap_Read)
			return;
		const int bytesPerSample = f.wBitsPerSample / 8;
		const int frameBytes = bytesPerSample * f.nChannels;
		if (frameBytes <= 0)
			return;
		const double step = (double)f.nSamplesPerSec / (double)kOutRate;
		const size_t totalFrames = st.srcFrames;

		// Source span this tick can touch, plus a frame of slack for the
		// fractional cursor landing mid-frame.
		const int want = (int)((double)frames * step) + 2;
		static thread_local std::vector<BYTE> scratch;
		if (scratch.size() < (size_t)want * (size_t)frameBytes)
			scratch.assign((size_t)want * (size_t)frameBytes, 0);

		size_t base = (size_t)st.cursor;         // first source frame we need
		if (base >= totalFrames)
			base = st.looping ? 0 : totalFrames;
		int got = 0;
		if (base < totalFrames)
			got = cap_Read(id, (unsigned)base, (unsigned)want, scratch.data(),
			               (unsigned)scratch.size());
		// Short read on a LOOPING buffer means we hit the end mid-window; take
		// the seam from the top so the loop point is not a hole.
		if (got < want && st.looping && got >= 0)
		{
			const int more = cap_Read(id, 0, (unsigned)(want - got),
			                          scratch.data() + (size_t)got * frameBytes,
			                          (unsigned)(scratch.size() - (size_t)got * frameBytes));
			if (more > 0)
				got += more;
		}
		if (got <= 0)
		{
			if (!st.looping && !st.finished)
			{
				st.finished = true;
				g_oneshotEnd.fetch_add(1, std::memory_order_relaxed);
			}
			return;
		}

		{
			BufState::Ev& e = st.ev[st.evNext % 32];
			e.kind = 'R';
			e.pos = (DWORD)(base * frameBytes);
			e.bytes = (DWORD)(got * frameBytes);
			e.peak = PeakOf(scratch.data(), (size_t)got * frameBytes, f.wBitsPerSample);
			st.evNext++;
		}

		const float gain = GainFromDb(st.volume);
		float gl = 1.0f, gr = 1.0f;
		if (st.pan > 0) gl = GainFromDb(-st.pan);
		else if (st.pan < 0) gr = GainFromDb(st.pan);

		// Offset within the window, carrying the cursor's fractional phase.
		double pos = st.cursor - (double)base;
		for (int i = 0; i < frames; ++i)
		{
			const size_t idx = (size_t)pos;
			if (idx >= (size_t)got)
			{
				// Ran out of window. For a one-shot that is the end of the
				// sound; a looping buffer just continues next tick from the
				// wrapped cursor.
				if (!st.looping && !st.finished)
				{
					st.finished = true;
					g_oneshotEnd.fetch_add(1, std::memory_order_relaxed);
				}
				break;
			}
			const BYTE* src = scratch.data() + idx * frameBytes;
			int l = 0, r = 0;
			if (bytesPerSample == 2)
			{
				const short* v = (const short*)src;
				l = v[0];
				r = (f.nChannels > 1) ? v[1] : v[0];
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
			st.energy += ((double)ol * ol + (double)orr * orr);
			st.framesMixed++;
			pos += step;
		}

		st.cursor = (double)base + pos;
		if (st.cursor >= (double)totalFrames)
		{
			if (st.looping)
			{
				st.cursor -= (double)totalFrames * floor(st.cursor / (double)totalFrames);
				st.finished = false;
			}
			else
			{
				st.cursor = (double)totalFrames;
			}
		}
	}

	// Pull the shim's view of every live buffer into g_bufs. This REPLACES the
	// hook path's phases 1 and 2 -- discovery, format, gain, play state and the
	// authoritative cursor all arrive in one call, from the component that owns
	// them, with no vtable patched and no ordering race to lose.
	int SyncFromShim()
	{
		static D2SndCapBuf snap[64];
		const int n = cap_Snapshot ? cap_Snapshot(snap, 64) : 0;
		if (n <= 0)
			return 0;   // 0 = none; negative = more than 64 live, ignore the tail

		std::lock_guard<std::mutex> lk(g_mx);
		int active = 0;
		for (int i = 0; i < n; ++i)
		{
			const D2SndCapBuf& b = snap[i];
			IDirectSoundBuffer* key = (IDirectSoundBuffer*)b.id;
			BufState& st = g_bufs[key];
			const bool fresh = st.srcFrames == 0;
			st.fmt.wFormatTag = WAVE_FORMAT_PCM;
			st.fmt.nChannels = (WORD)b.channels;
			st.fmt.nSamplesPerSec = b.rate;
			st.fmt.wBitsPerSample = (WORD)b.bits;
			st.fmt.nBlockAlign = (WORD)((b.bits / 8) * b.channels);
			st.fmt.nAvgBytesPerSec = st.fmt.nBlockAlign * b.rate;
			st.srcFrames = b.frames;
			st.volume = b.volume;
			st.pan = b.pan;
			st.looping = b.looping != 0;

			const bool wasPlaying = st.playing;
			st.playing = b.playing != 0;
			if (st.playing && (!wasPlaying || fresh))
			{
				// A fresh Play: take the shim's cursor rather than integrating
				// from wherever we happened to be.
				st.cursor = (double)b.playFrame;
				st.needResync = false;
				st.finished = false;
				st.plays++;
				g_plays.fetch_add(1, std::memory_order_relaxed);
			}
			else if (st.playing)
			{
				// Steady state: same slew-vs-snap rules as the hook path, against
				// the shim's cursor instead of DirectSound's.
				constexpr double kResyncFrames = 2205.0;
				constexpr double kSlewDeadband = 221.0;
				const double dsPos = (double)b.playFrame;
				const double total = (double)st.srcFrames;
				double sd = dsPos - st.cursor;
				if (st.looping && total > 0.0)
				{
					if (sd > total * 0.5)       sd -= total;
					else if (sd < -total * 0.5) sd += total;
				}
				const double drift = sd < 0.0 ? -sd : sd;
				if (drift > (double)g_driftMax.load(std::memory_order_relaxed))
					g_driftMax.store((unsigned long)drift, std::memory_order_relaxed);
				if (drift > kResyncFrames)
				{
					st.cursor = dsPos;
					st.finished = false;
					g_resyncs.fetch_add(1, std::memory_order_relaxed);
				}
				else if (drift > kSlewDeadband)
				{
					st.cursor += sd * 0.05;
					g_slews.fetch_add(1, std::memory_order_relaxed);
				}
			}
			if (st.playing)
				++active;
		}
		return active;
	}

	// The PRODUCER. Mixes one 10 ms block per tick and publishes it to the ring;
	// it never touches an audio device, so it runs identically on a box with no
	// endpoint at all -- the property the remote stream depends on.
	//
	// PACING. The old loop was paced by waveOut handing buffers back, i.e. by
	// the AUDIO DEVICE's clock -- the same clock DirectSound advances on, which
	// is why integrated cursors never drifted far from GetCurrentPosition. This
	// loop is paced by a high-resolution waitable timer, i.e. the CPU clock, so
	// the two clocks now measurably skew (~tens of ppm). That is why phase 3
	// below grew a SLEW band: without it, skew accumulates to the 100 ms hard
	// threshold every few minutes and resyncs with an audible jump -- a
	// plausible mechanism for the observed "repeats" stutter, now counted.
	DWORD WINAPI MixProducerThread(LPVOID)
	{
		// Explicit COM init for SetSessionMute. The old loop leaned on some
		// other thread having created the process's implicit MTA.
		CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

		// Clear any session mute inherited from a previous run of this process
		// (a killed verify/refnoise leaves one behind, and it silences
		// EVERYTHING downstream). Cheap insurance, once, at a point where COM
		// is definitely initialised on this thread.
		SetSessionMute(false);

		timeBeginPeriod(1);
		HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr,
			CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
		if (!timer)
			timer = CreateWaitableTimerW(nullptr, FALSE, nullptr);  // pre-1803 fallback
		if (timer)
		{
			LARGE_INTEGER due; due.QuadPart = -1;
			SetWaitableTimer(timer, &due, 10, nullptr, nullptr, FALSE);
		}

		// FRAME COUNT COMES FROM THE CLOCK, NOT THE TICK COUNT.
		//
		// Emitting a fixed 441 frames per wake assumes the wake was exactly
		// 10 ms. It never is: a periodic timer fires LATE, never early, so a
		// fixed block silently produces slow. Measured on the first deployed
		// build -- 13 underruns and 5325 frames of inserted silence in 16
		// seconds with nothing even playing, a ~0.8% deficit. That is timer
		// granularity, not crystal skew, and it would have starved a remote
		// consumer exactly as it starved the speakers.
		//
		// So: derive how many frames SHOULD exist by now from QPC and emit the
		// difference. A late wake produces a correspondingly bigger block and
		// the deficit never accumulates. What remains against the device is
		// genuine crystal skew (tens of ppm), which the consumer's latency
		// trim absorbs over minutes.
		LARGE_INTEGER qpf{}, epoch{}, lastWake{};
		QueryPerformanceFrequency(&qpf);
		QueryPerformanceCounter(&epoch);
		unsigned long long produced = 0;          // frames emitted since epoch
		unsigned long sessionPoll = 0;            // ticks, for the mute poll below
		// Catch-up ceiling. Past this we declare the lost time UNRECOVERABLE
		// rather than mixing a multi-second block under the lock to "catch up"
		// -- the audio is gone, and pretending otherwise stalls the game's
		// audio thread and overruns the ring in one go.
		constexpr int kMaxBlock = 4410;           // 100 ms

		std::vector<int> acc((size_t)kMaxBlock * kOutCh, 0);
		while (g_run.load(std::memory_order_relaxed))
		{
			if (timer) WaitForSingleObject(timer, 100); else Sleep(10);

			// Wake-to-wake gap: the producer-side "gaps" hypothesis. A tick that
			// arrives late leaves a hole in the ring exactly as long as the delay.
			LARGE_INTEGER wake; QueryPerformanceCounter(&wake);
			if (lastWake.QuadPart && qpf.QuadPart)
			{
				const unsigned long gapUs = (unsigned long)
					((wake.QuadPart - lastWake.QuadPart) * 1000000ll / qpf.QuadPart);
				if (gapUs > g_maxGapUs.load(std::memory_order_relaxed))
					g_maxGapUs.store(gapUs, std::memory_order_relaxed);
				if (gapUs > 15000)
					g_lateTicks.fetch_add(1, std::memory_order_relaxed);
			}
			lastWake = wake;

			// How many frames the clock says should exist by now.
			int block = 0;
			if (qpf.QuadPart)
			{
				const unsigned long long due = (unsigned long long)
					((wake.QuadPart - epoch.QuadPart) * (long long)kOutRate / qpf.QuadPart);
				long long want = (long long)due - (long long)produced;
				if (want <= 0)
					continue;                     // early wake: nothing owed yet
				if (want > kMaxBlock)
				{
					// Long stall (breakpoint, swap, a wedged tick). Restart the
					// clock here and emit one nominal block: the missing audio
					// is not recoverable and chasing it just moves the stall.
					epoch = wake;
					produced = 0;
					want = kMixFrames;
					g_rateResets.fetch_add(1, std::memory_order_relaxed);
				}
				block = (int)want;
			}
			else
			{
				block = kMixFrames;               // no QPC: nominal, as before
			}

			std::fill(acc.begin(), acc.begin() + (size_t)block * kOutCh, 0);
			int active = 0;

			if (g_captureMode.load(std::memory_order_relaxed) == 1)
			{
				// SHIM MODE. One call replaces phases 1-2 below: the component
				// that owns the buffers hands us discovery, format, gain, play
				// state and the cursor together. Nothing is patched, so there is
				// no install race and no Detours transaction to lose.
				active = SyncFromShim();
				std::lock_guard<std::mutex> lk(g_mx);
				for (auto it = g_bufs.begin(); it != g_bufs.end(); ++it)
					if (it->second.playing)
						MixShim(acc, it->second, block, (void*)it->first);
				if (cap_Primary)
				{
					long pv = 0;
					if (cap_Primary(&pv))
						g_primaryVol.store(pv, std::memory_order_relaxed);
				}
			}
			else
			{
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
			struct Live { IDirectSoundBuffer* b; DWORD pos; DWORD status; bool haveStatus; };
			std::vector<Live> live;
			live.reserve(sounding.size());
			for (IDirectSoundBuffer* b : sounding)
			{
				Live e{ b, 0, 0, false };
				if (real_GetPos)
				{
					DWORD play = 0, write = 0;
					if (SUCCEEDED(real_GetPos(b, &play, &write)))
						e.pos = play;
				}
				if (real_GetStatus)
					e.haveStatus = SUCCEEDED(real_GetStatus(b, &e.status));
				live.push_back(e);
			}

			// PHASE 3: mix from where DirectSound is reading.
			{
				std::lock_guard<std::mutex> lk(g_mx);
				for (auto& e : live)
				{
					auto it = g_bufs.find(e.b);
					if (it == g_bufs.end())
						continue;
					BufState& st = it->second;
					const int frameBytes = (st.fmt.wBitsPerSample / 8) * st.fmt.nChannels;
					if (frameBytes > 0)
					{
						// DirectSound's position is authoritative for WHERE a
						// sound is, but not to the sample: it is reported in
						// driver-sized blocks. Overwriting our cursor with it
						// every tick re-mixed or dropped a ~10 ms fragment 100
						// times a second -- inaudible under sustained music,
						// ruinous on a footstep's attack transient.
						//
						// Integrate instead, and defer to DirectSound only on a
						// real discontinuity. Both clocks come off the same audio
						// device, so they should not drift meaningfully apart.
						// kResyncFrames is deliberately far larger than one
						// tick's worth of jitter (220 frames at 22050) so normal
						// block granularity never triggers it, but a seek or a
						// retrigger always does.
						constexpr double kResyncFrames = 2205.0;   // 100 ms
						// Below the hard threshold but above one tick's worth of
						// block jitter: sustained small drift. With the producer
						// now paced by the CPU clock rather than the audio
						// device's (see MixProducerThread), this band is no
						// longer noise -- it is CLOCK SKEW accumulating, and left
						// alone it climbs to kResyncFrames and hard-resyncs with
						// an audible jump. Nudge instead: 5% of the gap per tick
						// converges in under a second and moves the cursor by
						// fractions of a millisecond -- inaudible where the hard
						// snap was a repeat/skip.
						constexpr double kSlewDeadband = 221.0;    // ~10 ms
						const double dsPos = (double)(e.pos / frameBytes);
						const double totalFrames =
							(double)(st.pcm.size() / (size_t)frameBytes);
						// On a LOOPING buffer the two cursors live on a circle:
						// the instant either one wraps, the straight-line gap is
						// ~one whole buffer even though they are adjacent. Take
						// the short way round -- signed, so the slew below knows
						// which direction "toward DirectSound" is. Non-looping
						// buffers stay linear: the heal branch reads a large
						// negative value as "DirectSound rewound this".
						double sd = dsPos - st.cursor;
						if (st.looping && totalFrames > 0.0)
						{
							if (sd > totalFrames * 0.5)       sd -= totalFrames;
							else if (sd < -totalFrames * 0.5) sd += totalFrames;
						}
						const double drift = sd < 0.0 ? -sd : sd;
						if (drift > (double)g_driftMax.load(std::memory_order_relaxed))
							g_driftMax.store((unsigned long)drift, std::memory_order_relaxed);

						if (st.needResync)
						{
							st.cursor = dsPos;
							st.needResync = false;
							st.finished = false;
							g_resyncs.fetch_add(1, std::memory_order_relaxed);
						}
						else if (st.finished)
						{
							// We believe this sound ended. If DirectSound has
							// rewound it and is STILL playing it, it is looping
							// and our flag was wrong -- heal rather than stutter.
							// Only a clear rewind counts; jitter must not revive
							// a sound that genuinely finished.
							if (sd < -kResyncFrames)
							{
								st.looping = true;
								st.finished = false;
								st.cursor = dsPos;
								g_resyncs.fetch_add(1, std::memory_order_relaxed);
								g_healedLoops.fetch_add(1, std::memory_order_relaxed);
							}
						}
						else if (drift > kResyncFrames)
						{
							st.cursor = dsPos;
							g_resyncs.fetch_add(1, std::memory_order_relaxed);
						}
						else if (drift > kSlewDeadband)
						{
							st.cursor += sd * 0.05;
							g_slews.fetch_add(1, std::memory_order_relaxed);
						}
					}
					// DirectSound is authoritative for BOTH position and play
					// state. Without this a finished one-shot never ends.
					if (e.haveStatus)
					{
						st.playing = (e.status & DSBSTATUS_PLAYING) != 0;
						st.looping = (e.status & DSBSTATUS_LOOPING) != 0;
					}
				}
				for (auto& kv : g_bufs)
				{
					if (!kv.second.playing)
						continue;
					++active;
					MixInto(acc, kv.second, block);
				}
			}
			}   // end hook-mode phases

			g_active.store(active, std::memory_order_relaxed);
			if (active)
				g_mixed.fetch_add(1, std::memory_order_relaxed);

			// MASTER GAIN. Applied over the summed mix, exactly where DirectSound
			// applies the primary buffer's volume over its own output -- so the
			// in-game sliders move our audio the same way they move the game's.
			const float master = GainFromDb((LONG)g_primaryVol.load(std::memory_order_relaxed));
			if (master < 0.999f)
				for (size_t i = 0; i < acc.size(); ++i)
					acc[i] = (int)(acc[i] * master);

			// THE PER-TICK SESSION MUTE IS GONE. It used to silence the whole
			// process audio session whenever no window of ours was foreground.
			// The ring redesign makes it redundant: the local consumer already
			// renders silence when unfocused (BUFFERFLAGS_SILENT), and the
			// game's own output is already held at DSBVOLUME_MIN by
			// ApplyNativeMute/H_SetVolume for as long as our mixer drives.
			//
			// It was also actively harmful. A session mute sits DOWNSTREAM of
			// everything this process renders, so while it was on, loopback
			// mode and the /audio/verify reference both captured silence --
			// making a perfectly healthy game look like a dead audio path.
			// Anything that must silence the endpoint (refnoise, verify) still
			// calls SetSessionMute explicitly and restores it.

			// Clamp ONCE into the ring every consumer sees. The ring always
			// carries the true mix -- the audibility gate (source mode, playLocal,
			// focus) belongs to the LOCAL consumer in RenderThread, not here: a
			// defocused or hidden game must keep streaming the real audio to a
			// remote client. Clipping is counted on the mix itself now, not on
			// the audible copy -- the old gate silently stopped counting clips
			// whenever the window happened to be unfocused.
			const unsigned long long w = g_ringWrite.load(std::memory_order_relaxed);
			unsigned long clipped = 0;
			for (int f = 0; f < block; ++f)
			{
				const size_t slot = ((size_t)((w + f) & (kRingFrames - 1))) * kOutCh;
				for (int c = 0; c < kOutCh; ++c)
				{
					int v = acc[(size_t)f * kOutCh + c];
					if (v > 32767)  { v = 32767;  ++clipped; }
					if (v < -32768) { v = -32768; ++clipped; }
					g_ring[slot + c] = (short)v;
				}
			}
			if (clipped)
				g_clipped.fetch_add(clipped, std::memory_order_relaxed);
			g_ringWrite.store(w + block, std::memory_order_release);
			produced += (unsigned long long)block;
			g_produced.fetch_add(block, std::memory_order_relaxed);

			// Tee our mix into the verification buffer -- reading back the block
			// just published, which is pre-gate by construction.
			if (g_capturing.load(std::memory_order_relaxed))
			{
				std::lock_guard<std::mutex> lk(g_capMx);
				for (int f = 0; f < block; ++f)
				{
					const size_t slot = ((size_t)((w + f) & (kRingFrames - 1))) * kOutCh;
					g_capOurs.push_back(g_ring[slot + 0]);
					g_capOurs.push_back(g_ring[slot + 1]);
				}
			}

			// How long the mix work itself took. A tick that COMPUTES longer
			// than 10 ms is a different disease from one that WAKES late, and
			// the pair maxTickUs/maxGapUs separates them.
			// Poll the real session-mute state twice a second. Read from the OS
			// on this thread (COM is initialised here); the HTTP handler just
			// reports the atomic.
			//
			// Counts TICKS, not mixed blocks. Gating this on g_mixed was wrong:
			// that counter only advances while something is audible, so with the
			// game silent it sat at 0, `0 % 200 == 0` held every tick, and this
			// fired a COM round-trip 100x a second on the deadline thread --
			// measured maxTickUs 43730 (4.3x the tick budget) and 29 late ticks
			// with nothing playing at all.
			if ((++sessionPoll % 50) == 0)
				g_sessionMuted.store(ReadSessionMute(), std::memory_order_relaxed);

			LARGE_INTEGER done; QueryPerformanceCounter(&done);
			if (qpf.QuadPart)
			{
				const unsigned long tickUs = (unsigned long)
					((done.QuadPart - wake.QuadPart) * 1000000ll / qpf.QuadPart);
				if (tickUs > g_maxTickUs.load(std::memory_order_relaxed))
					g_maxTickUs.store(tickUs, std::memory_order_relaxed);
			}
		}

		if (timer) { CancelWaitableTimer(timer); CloseHandle(timer); }
		timeEndPeriod(1);
		CoUninitialize();
		return 0;
	}

	// The LOCAL playback consumer: WASAPI shared mode, event driven -- the
	// device tells us when it wants audio instead of us polling for permission
	// to hand it some. That inversion is the underrun fix: the old Sleep(2)
	// waveOut poll had ~40 ms of total cushion and lost it to any scheduler
	// hiccup, with no counter to even prove it happened.
	//
	// Structure: a device-acquire loop around a render loop. A box with no
	// endpoint at all (Session-0 container: measured DSERR_NODRIVER) parks in
	// the outer loop retrying every 5 s while the producer keeps mixing for the
	// ring's other consumers -- local playback is a convenience, never a
	// dependency of the stream.
	DWORD WINAPI RenderThread(LPVOID)
	{
		CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

		// How far behind live this consumer sits. 50 ms of cushion absorbs
		// producer jitter; the latency costs only the local ear, never the
		// remote stream (that consumer picks its own cushion). kMaxLatency is
		// the trim point: CPU-vs-device clock skew creeps the backlog a few
		// frames a minute, and past 200 ms we snap back to target and COUNT it
		// rather than let local playback fall progressively behind the game.
		constexpr unsigned long long kTargetLatency = 2205;   // 50 ms
		constexpr unsigned long long kMaxLatency    = 8820;   // 200 ms

		while (g_run.load(std::memory_order_relaxed))
		{
			IMMDeviceEnumerator* en = nullptr;
			IMMDevice* dev = nullptr;
			IAudioClient* ac = nullptr;
			IAudioRenderClient* rc = nullptr;
			HANDLE evt = CreateEventA(nullptr, FALSE, FALSE, nullptr);

			WAVEFORMATEX wf{};
			wf.wFormatTag = WAVE_FORMAT_PCM;
			wf.nChannels = kOutCh;
			wf.nSamplesPerSec = kOutRate;
			wf.wBitsPerSample = 16;
			wf.nBlockAlign = kOutCh * 2;
			wf.nAvgBytesPerSec = kOutRate * wf.nBlockAlign;

			UINT32 bufFrames = 0;
			bool up = false;
			HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
				CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&en);
			if (SUCCEEDED(hr)) hr = en->GetDefaultAudioEndpoint(eRender, eConsole, &dev);
			if (SUCCEEDED(hr)) hr = dev->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
			                                      nullptr, (void**)&ac);
			// AUTOCONVERTPCM: we speak 16-bit 44.1k whatever the device's mix
			// format is -- the same trick the loopback reference uses, and it
			// keeps this consumer format-identical to the ring.
			if (SUCCEEDED(hr)) hr = ac->Initialize(AUDCLNT_SHAREMODE_SHARED,
				AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
				AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
				AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
				1000000 /* 100 ms device buffer */, 0, &wf, nullptr);
			if (SUCCEEDED(hr) && evt) hr = ac->SetEventHandle(evt);
			if (SUCCEEDED(hr)) hr = ac->GetBufferSize(&bufFrames);
			if (SUCCEEDED(hr)) hr = ac->GetService(__uuidof(IAudioRenderClient), (void**)&rc);
			if (SUCCEEDED(hr) && bufFrames)
			{
				// Prime the device with silence before Start so it never reads
				// frames nobody wrote.
				BYTE* p = nullptr;
				if (SUCCEEDED(rc->GetBuffer(bufFrames, &p)))
					rc->ReleaseBuffer(bufFrames, AUDCLNT_BUFFERFLAGS_SILENT);
				up = SUCCEEDED(ac->Start());
			}

			if (up)
			{
				if (!g_renderDeviceOk.exchange(1, std::memory_order_relaxed))
					AudLog("render device up (WASAPI shared, event-driven)");

				unsigned long long write = g_ringWrite.load(std::memory_order_acquire);
				unsigned long long read = write > kTargetLatency ? write - kTargetLatency : 0;
				// Priming: after a start or an underrun, hold silence until the
				// cushion has rebuilt. Without this an underrun leaves the
				// backlog near zero and EVERY following event underruns again --
				// one audible gap becomes a machine-gun of them.
				bool priming = true;

				while (g_run.load(std::memory_order_relaxed))
				{
					if (WaitForSingleObject(evt, 2000) != WAIT_OBJECT_0)
						break;                        // device wedged: reacquire
					UINT32 pad = 0;
					if (FAILED(ac->GetCurrentPadding(&pad)))
						break;                        // invalidated: reacquire
					const UINT32 need = bufFrames - pad;
					if (!need)
						continue;

					write = g_ringWrite.load(std::memory_order_acquire);
					unsigned long long avail = write - read;
					if (avail > kMaxLatency)
					{
						// Clock skew crept the backlog up, or we stalled and the
						// producer lapped us. Snap back to target and count it.
						read = write - kTargetLatency;
						avail = kTargetLatency;
						g_latencyTrims.fetch_add(1, std::memory_order_relaxed);
					}
					g_ringFill.store((unsigned long)avail, std::memory_order_relaxed);
					if (priming && avail >= kTargetLatency)
						priming = false;

					// The audibility gate lives HERE, on the local consumer
					// only. The ring (and so the verify tee and the future
					// remote stream) always carries the real mix. In loopback
					// mode we must not play -- our output lands in the same
					// session the loopback taps and would feed back into it.
					const bool audible = g_srcMode.load(std::memory_order_relaxed) == 0
					                  && g_playLocal.load(std::memory_order_relaxed)
					                  && OurProcessHasFocus();

					BYTE* p = nullptr;
					if (FAILED(rc->GetBuffer(need, &p)))
						break;
					UINT32 take = priming ? 0
					            : (UINT32)(avail < need ? avail
					                                    : (unsigned long long)need);
					if (!priming && take < need)
					{
						// Ran dry mid-buffer: count it ONCE, fill the shortfall
						// with silence, rebuild the cushion before resuming.
						g_outUnderruns.fetch_add(1, std::memory_order_relaxed);
						g_outUnderrunFrames.fetch_add(need - take, std::memory_order_relaxed);
						priming = true;
					}
					short* out = (short*)p;
					for (UINT32 f = 0; f < take; ++f)
					{
						const size_t slot =
							((size_t)((read + f) & (kRingFrames - 1))) * kOutCh;
						out[f * 2 + 0] = g_ring[slot + 0];
						out[f * 2 + 1] = g_ring[slot + 1];
					}
					for (UINT32 f = take; f < need; ++f)
					{
						out[f * 2 + 0] = 0;
						out[f * 2 + 1] = 0;
					}
					// Consume even when inaudible: this cursor must track live,
					// so re-enabling audio resumes at NOW rather than replaying
					// a minutes-old backlog.
					read += take;
					rc->ReleaseBuffer(need, audible ? 0 : AUDCLNT_BUFFERFLAGS_SILENT);
				}
				ac->Stop();
				g_renderDeviceOk.store(0, std::memory_order_relaxed);
			}

			if (rc) rc->Release();
			if (ac) ac->Release();
			if (dev) dev->Release();
			if (en) en->Release();
			if (evt) CloseHandle(evt);

			if (!up)
			{
				// No endpoint (Session-0, unplugged, audio service down): the
				// producer keeps mixing for the ring's other consumers; we just
				// retry quietly.
				if (g_renderDeviceOk.exchange(0, std::memory_order_relaxed))
					AudLog("render device lost; retrying");
				for (int i = 0; i < 50 && g_run.load(std::memory_order_relaxed); ++i)
					Sleep(100);
			}
		}
		CoUninitialize();
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
	g_installStage.store(1, std::memory_order_relaxed);

	// PREFER THE SHIM'S CAPTURE API OVER HOOKING IT.
	//
	// When the game is running dsound-headless, every fact the hooks below
	// reconstruct -- the PCM, the cursor, volume, pan, play state -- is already
	// owned by that DLL, and asking for it is strictly better than patching it:
	// no Detours transaction to lose to the launcher, no SEH on the game's
	// audio thread, and above all NO INSTALL ORDERING RACE. A buffer created
	// before a vtable patch is invisible forever (that is precisely what made
	// menu audio silent while in-world worked); a buffer created before this
	// call is simply in the registry when we ask.
	//
	// The version handshake is the guard: an unknown ABI is refused rather than
	// guessed at, and we fall through to hooking, which is also what happens on
	// a stock Microsoft dsound.dll where these exports do not exist.
	cap_Abi = (CapAbiFn)GetProcAddress(dsdll, "D2SndCap_Abi");
	if (cap_Abi && cap_Abi() == 1u)
	{
		cap_Snapshot = (CapSnapshotFn)GetProcAddress(dsdll, "D2SndCap_Snapshot");
		cap_Read     = (CapReadFn)GetProcAddress(dsdll, "D2SndCap_Read");
		cap_Primary  = (CapPrimaryFn)GetProcAddress(dsdll, "D2SndCap_Primary");
		if (cap_Snapshot && cap_Read)
		{
			g_captureMode.store(1, std::memory_order_relaxed);
			g_installStage.store(5, std::memory_order_relaxed);
			g_ring.assign((size_t)kRingFrames * kOutCh, 0);
			g_run.store(true, std::memory_order_relaxed);
			g_thread = CreateThread(nullptr, 0, MixProducerThread, nullptr, 0, nullptr);
			g_renderThread = CreateThread(nullptr, 0, RenderThread, nullptr, 0, nullptr);
			AudLog("capture via dsound-headless API (no hooks installed)");
			return;
		}
		// Partial resolve means a DLL that claims our ABI but does not
		// implement it -- louder than silently hooking instead.
		AudLog("shim ABI 1 present but exports missing; falling back to hooks");
		cap_Snapshot = nullptr; cap_Read = nullptr; cap_Primary = nullptr;
	}
	else if (cap_Abi)
	{
		AudLog("dsound exports an UNKNOWN capture ABI; falling back to hooks");
		cap_Abi = nullptr;
	}

	using DSCreateFn = HRESULT(WINAPI*)(LPCGUID, IDirectSound**, LPUNKNOWN);
	auto dsCreate = (DSCreateFn)GetProcAddress(dsdll, "DirectSoundCreate");
	if (!dsCreate)
		return;
	g_installStage.store(2, std::memory_order_relaxed);

	IDirectSound* ds = nullptr;
	if (FAILED(dsCreate(nullptr, &ds, nullptr)) || !ds)
		return;
	g_installStage.store(3, std::memory_order_relaxed);
	void** vt = *(void***)ds;
	// IDirectSound vtable, verified against the SDK header rather than counted
	// by eye: 3 CreateSoundBuffer, 4 GetCaps, 5 DuplicateSoundBuffer.
	//
	// This read vt[4] for Duplicate, which is GetCaps. Two consequences, both
	// silent: DuplicateSoundBuffer was never hooked at all, so duplicated
	// buffers -- which is how D2 plays the same sound concurrently, e.g.
	// footsteps -- were never registered and never captured; and had the game
	// ever called GetCaps, H_Duplicate would have run with three declared
	// __stdcall parameters against two pushed and corrupted the stack on return.
	real_CreateSoundBuffer = (CreateSoundBufferFn)vt[3];
	real_Duplicate = (DuplicateFn)vt[5];
	g_vtCreateAddr.store((unsigned long long)(uintptr_t)real_CreateSoundBuffer,
	                     std::memory_order_relaxed);
	// RETRY on contention (see HookBufferVtable). This install now runs at
	// standalone-thread start, which overlaps the Detours launcher's own patch
	// transactions; one ERROR_INVALID_OPERATION with no retry cost the whole
	// session's capture. A longer, patient loop is fine here -- this is our own
	// thread, nothing waits on it, and losing the race means no audio at all.
	LONG devErr = ERROR_INVALID_OPERATION;
	for (int attempt = 0; attempt < 200 && devErr != NO_ERROR; ++attempt)
	{
		if (attempt)
			Sleep(10);
		if (DetourTransactionBegin() != NO_ERROR)
			continue;
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)real_CreateSoundBuffer, (PVOID)H_CreateSoundBuffer);
		DetourAttach(&(PVOID&)real_Duplicate, (PVOID)H_Duplicate);
		devErr = DetourTransactionCommit();
	}
	g_detourDevErr.store(devErr, std::memory_order_relaxed);
	if (devErr == NO_ERROR)
		g_installStage.store(4, std::memory_order_relaxed);
	else
		AudLog("device detour failed after retries");
	ds->Release();

	g_run.store(true, std::memory_order_relaxed);
	// Ring before threads: both of them index it unconditionally.
	g_ring.assign((size_t)kRingFrames * kOutCh, 0);
	g_thread = CreateThread(nullptr, 0, MixProducerThread, nullptr, 0, nullptr);
	g_renderThread = CreateThread(nullptr, 0, RenderThread, nullptr, 0, nullptr);
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
	//
	// NativeMuted(), not `want`: turning the mixer off must not un-mute a game
	// that is silent for the OTHER reason (hidden).
	ApplyNativeMute(NativeMuted());
}

// The game window's visibility changed. Hidden => the native path is silenced
// whether or not our mixer is running.
extern "C" void D2AudioCap_SetSuppressNative(int on)
{
	const bool want = on != 0;
	if (g_suppressNative.exchange(want, std::memory_order_relaxed) == want)
		return;
	// Sweep the buffers that already exist. Newborn ones are handled at
	// registration and every later SetVolume is rewritten in the hook, so this
	// only has to cover what is already playing at the moment of the switch.
	ApplyNativeMute(NativeMuted());
}

// 0 = mixer, 1 = process loopback. Switching restores or reapplies the native
// mute, because the two modes disagree about whether the game may be heard.
extern "C" void D2AudioCap_SetSourceMode(int mode)
{
	const int want = (mode == 1) ? 1 : 0;
	if (g_srcMode.exchange(want, std::memory_order_relaxed) == want)
		return;

	if (want == 1)
	{
		g_loopFrames.store(0, std::memory_order_relaxed);
		g_loopSumSq.store(0, std::memory_order_relaxed);
		// Hands off the game BEFORE the tap starts, or the first second of
		// capture is the silence we were still imposing.
		ApplyNativeMute(false);
		SetSessionMute(false);
		if (!g_liveThread)
		{
			g_liveRun.store(true, std::memory_order_relaxed);
			g_liveThread = CreateThread(nullptr, 0, RefThread, (LPVOID)1, 0, nullptr);
		}
	}
	else
	{
		if (g_liveThread)
		{
			g_liveRun.store(false, std::memory_order_relaxed);
			WaitForSingleObject(g_liveThread, 3000);
			CloseHandle(g_liveThread);
			g_liveThread = nullptr;
		}
		// Back under mixer rules: whatever they now say about muting applies.
		ApplyNativeMute(NativeMuted());
	}
}

extern "C" int D2AudioCap_SourceMode()
{
	return g_srcMode.load(std::memory_order_relaxed);
}

// frames captured and the RMS of them, so "is the tap alive and is it carrying
// audio" is answerable without listening.
extern "C" void D2AudioCap_LoopStats(unsigned long* frames, double* rms)
{
	const unsigned long long f = g_loopFrames.load(std::memory_order_relaxed);
	const unsigned long long sq = g_loopSumSq.load(std::memory_order_relaxed);
	if (frames) *frames = (unsigned long)f;
	if (rms)    *rms = f ? sqrt((double)sq / ((double)f * 2.0)) : 0.0;
}

extern "C" int D2AudioCap_SuppressNative()
{
	return g_suppressNative.load(std::memory_order_relaxed) ? 1 : 0;
}
extern "C" void D2AudioCap_Quality(unsigned long* oneshotEnd,
                                   unsigned long* clipped,
                                   unsigned long* resyncs,
                                   unsigned long* driftMax,
                                   unsigned long* healedLoops)
{
	if (oneshotEnd) *oneshotEnd = g_oneshotEnd.load(std::memory_order_relaxed);
	if (clipped)    *clipped    = g_clipped.load(std::memory_order_relaxed);
	if (resyncs)    *resyncs    = g_resyncs.load(std::memory_order_relaxed);
	if (driftMax)   *driftMax   = g_driftMax.load(std::memory_order_relaxed);
	if (healedLoops) *healedLoops = g_healedLoops.load(std::memory_order_relaxed);
}

extern "C" int  D2AudioCap_PlayLocal() { return g_playLocal.load(std::memory_order_relaxed) ? 1 : 0; }

// ---- ring consumer API --------------------------------------------------
//
// The ring is THE stream: one interleaved 44.1k/16-bit stereo mix, already
// summed, panned, master-gained and clamped. Local playback is one consumer of
// it (RenderThread above); the FLAC/WebSocket streamer is another
// (D2Debugger.audiostream.cpp). Consumers own their cursor and never write, so
// adding one costs the producer nothing and cannot perturb the others.
//
// Deliberately NOT gated on playLocal/focus/srcMode: those gate AUDIBILITY on
// this machine. A remote listener must keep hearing the game while the operator
// is looking at another window -- that is the entire point of remote play.

extern "C" void D2AudioCap_StreamFormat(int* rate, int* channels)
{
	if (rate)     *rate = kOutRate;
	if (channels) *channels = kOutCh;
}

// Copy up to maxFrames from `*cursor` onward. Pass a cursor of 0 to start:
// it is positioned `backlogFrames` behind live rather than at the ring's tail,
// so a new consumer begins at roughly now instead of replaying 1.5 s of stale
// audio. Returns frames copied (0 when caught up -- that is normal, not an
// error; sleep and ask again).
//
// A consumer that falls further behind than the ring is silently overwritten,
// so trim it forward and say so via `dropped` instead of shipping torn audio.
extern "C" int D2AudioCap_ReadRing(unsigned long long* cursor, short* out,
                                   int maxFrames, int backlogFrames,
                                   unsigned long* dropped)
{
	if (!cursor || !out || maxFrames <= 0 || g_ring.empty())
		return 0;
	const unsigned long long w = g_ringWrite.load(std::memory_order_acquire);
	if (*cursor == 0)
	{
		const unsigned long long back = (unsigned long long)
			(backlogFrames > 0 ? backlogFrames : 0);
		*cursor = (w > back) ? (w - back) : 0;
	}
	unsigned long long avail = (w > *cursor) ? (w - *cursor) : 0;
	// Lapped: the producer has overwritten data this consumer never read.
	if (avail > kRingFrames)
	{
		const unsigned long long lost = avail - (kRingFrames / 2);
		*cursor += lost;
		avail = kRingFrames / 2;
		if (dropped) *dropped = (unsigned long)lost;
	}
	int n = (int)((avail < (unsigned long long)maxFrames) ? avail : (unsigned long long)maxFrames);
	for (int f = 0; f < n; ++f)
	{
		const size_t slot = ((size_t)((*cursor + f) & (kRingFrames - 1))) * kOutCh;
		out[f * 2 + 0] = g_ring[slot + 0];
		out[f * 2 + 1] = g_ring[slot + 1];
	}
	*cursor += (unsigned long long)n;
	return n;
}

// The output-chain health block: everything needed to attribute a stutter
// without listening to it. Gaps show up in underruns (local consumer starved)
// or lateTicks/maxGapUs (producer woke late); a tick that COMPUTES too long is
// maxTickUs; repeats show up in slews (gentle, inaudible by design) and the
// existing resyncs counter (hard snaps, each one potentially audible).
extern "C" void D2AudioCap_Stream(unsigned long* underruns,
                                  unsigned long* underrunFrames,
                                  unsigned long* lateTicks,
                                  unsigned long* maxTickUs,
                                  unsigned long* maxGapUs,
                                  unsigned long* slews,
                                  unsigned long* trims,
                                  unsigned long* ringFill,
                                  int* deviceOk,
                                  unsigned long* producedFrames,
                                  unsigned long* rateResets,
                                  int* sessionMuted,
                                  unsigned long* descPatched,
                                  unsigned long* descPassthru,
                                  unsigned long* descSize,
                                  unsigned long* installStage,
                                  long* detourDevErr, long* detourBufErr,
                                  unsigned long long* vtCreateAddr,
                                  int* captureMode)
{
	if (installStage) *installStage = g_installStage.load(std::memory_order_relaxed);
	if (captureMode)  *captureMode = g_captureMode.load(std::memory_order_relaxed);
	if (detourDevErr) *detourDevErr = g_detourDevErr.load(std::memory_order_relaxed);
	if (detourBufErr) *detourBufErr = g_detourBufErr.load(std::memory_order_relaxed);
	if (vtCreateAddr) *vtCreateAddr = g_vtCreateAddr.load(std::memory_order_relaxed);
	if (descPatched)  *descPatched = g_descPatched.load(std::memory_order_relaxed);
	if (descPassthru) *descPassthru = g_descPassthru.load(std::memory_order_relaxed);
	if (descSize)     *descSize = g_descSize.load(std::memory_order_relaxed);
	if (producedFrames) *producedFrames = (unsigned long)g_produced.load(std::memory_order_relaxed);
	if (rateResets)     *rateResets = g_rateResets.load(std::memory_order_relaxed);
	if (sessionMuted)   *sessionMuted = g_sessionMuted.load(std::memory_order_relaxed);
	if (underruns)      *underruns = g_outUnderruns.load(std::memory_order_relaxed);
	if (underrunFrames) *underrunFrames = g_outUnderrunFrames.load(std::memory_order_relaxed);
	if (lateTicks)      *lateTicks = g_lateTicks.load(std::memory_order_relaxed);
	if (maxTickUs)      *maxTickUs = g_maxTickUs.load(std::memory_order_relaxed);
	if (maxGapUs)       *maxGapUs = g_maxGapUs.load(std::memory_order_relaxed);
	if (slews)          *slews = g_slews.load(std::memory_order_relaxed);
	if (trims)          *trims = g_latencyTrims.load(std::memory_order_relaxed);
	if (ringFill)       *ringFill = g_ringFill.load(std::memory_order_relaxed);
	if (deviceOk)       *deviceOk = g_renderDeviceOk.load(std::memory_order_relaxed);
}

// Run BOTH paths for `seconds` and write two WAVs: ours (the mixer) and the
// reference (WASAPI loopback of the native mix). Returns frames captured on
// each side, or -1 on failure.
//
// Mutes local playback for the duration -- our own render output is part of
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
	WriteWav(pRef, g_capRef, kOutRate, 2);
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

extern "C" void D2AudioCap_Primary(long* vol, unsigned long* sets, float* gain, int* known)
{
	// Read DirectSound's own value where possible -- the same cross-check that
	// showed the per-buffer volumes were faithful.
	LONG live = g_primaryVol.load(std::memory_order_relaxed);
	int have = 0;
	{
		std::lock_guard<std::mutex> lk(g_mx);
		if (g_primary && real_GetVolume)
		{
			LONG v = 0;
			if (SUCCEEDED(real_GetVolume(g_primary, &v))) { live = v; }
			have = 1;
		}
		else if (g_primary)
		{
			have = 1;
		}
	}
	if (vol)   *vol = live;
	if (sets)  *sets = g_primaryVolSets.load(std::memory_order_relaxed);
	if (gain)  *gain = GainFromDb(live);
	if (known) *known = have;
}

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
