// dsound_shim.cpp -- a DirectSound that needs no audio device.
//
// WHY THIS EXISTS. Measured 2026-08-01 with the Windows audio services stopped:
// DirectSoundCreate8 returns DSERR_NODRIVER (0x88780078) and only the "Primary
// Sound Driver" placeholder enumerates. No device means no IDirectSound, which
// means no secondary buffers, which means the capture layer has nothing to hook
// -- audio for remote play is dead in a headless container, and D2 needs -ns to
// start at all. Video survives both Session 0 and a missing device; audio does
// not. This closes that gap.
//
// THE APPROACH. Don't capture the audio system -- BE the audio system. Buffers
// are plain memory and the play cursor is arithmetic over the wall clock, so
// nothing here touches an endpoint, a driver or a mixer. D2 writes PCM into our
// buffers exactly as it always did; D2Debugger's mixer reads them through the
// same hooks it already installs and renders via waveOut when a device exists.
// In a container it simply doesn't render, and the PCM is still there to ship
// to a remote client.
//
// WHY A LOCAL DLL WORKS. dsound.dll is not a KnownDLL, so a copy in the game
// directory is loaded ahead of the system one. cnc-ddraw in that same folder is
// the working precedent for this exact pattern.
//
// COMPATIBILITY CONSTRAINTS, both measured rather than assumed:
//   * D2sound.dll imports dsound.dll BY ORDINAL 1 and 2, not by name, so the
//     .def file pins every ordinal to match the real dsound.dll exactly.
//   * D2Debugger resolves "DirectSoundCreate" BY NAME and then reads the vtable
//     off a device it creates itself, detouring fixed indices. These classes
//     derive from the real COM interfaces, so the vtable layout is the standard
//     one by construction and those indices keep meaning what they mean.
//
// NO TIMER THREAD. The cursor is computed on demand from QueryPerformanceCounter
// the moment anyone asks. A thread ticking buffers forward would be one more
// thing to race against the game's audio thread, for no gain.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmreg.h>
#include <mmsystem.h>
#include <dsound.h>
#include <stdio.h>
#include <math.h>
#include <vector>

// EXPORTS, pinned to the real dsound.dll's ordinals.
//
// This is not cosmetic: D2sound.dll imports BY ORDINAL 1 and 2 (verified with
// dumpbin -- its import table has no names at all for dsound), so an export
// table that merely has the right names in the wrong slots binds the game to
// the wrong functions. Ordinals below are exactly those of the system
// dsound.dll. The internal names carry the x86 __stdcall decoration (_Name@N,
// N = bytes of arguments) so the linker cannot mismatch them silently.
#pragma comment(linker, "/EXPORT:DirectSoundCreate=_Shim_DirectSoundCreate@12,@1")
#pragma comment(linker, "/EXPORT:DirectSoundEnumerateA=_Shim_DirectSoundEnumerateA@8,@2")
#pragma comment(linker, "/EXPORT:DirectSoundEnumerateW=_Shim_DirectSoundEnumerateW@8,@3")
#pragma comment(linker, "/EXPORT:DllCanUnloadNow=_Shim_DllCanUnloadNow@0,@4,PRIVATE")
#pragma comment(linker, "/EXPORT:DllGetClassObject=_Shim_DllGetClassObject@12,@5,PRIVATE")
#pragma comment(linker, "/EXPORT:DirectSoundCaptureCreate=_Shim_DirectSoundCaptureCreate@12,@6")
#pragma comment(linker, "/EXPORT:DirectSoundCaptureEnumerateA=_Shim_DirectSoundCaptureEnumerateA@8,@7")
#pragma comment(linker, "/EXPORT:DirectSoundCaptureEnumerateW=_Shim_DirectSoundCaptureEnumerateW@8,@8")
#pragma comment(linker, "/EXPORT:GetDeviceID=_Shim_GetDeviceID@8,@9")
#pragma comment(linker, "/EXPORT:DirectSoundFullDuplexCreate=_Shim_DirectSoundFullDuplexCreate@40,@10")
#pragma comment(linker, "/EXPORT:DirectSoundCreate8=_Shim_DirectSoundCreate8@12,@11")
#pragma comment(linker, "/EXPORT:DirectSoundCaptureCreate8=_Shim_DirectSoundCaptureCreate8@12,@12")

// ---------------------------------------------------------------- logging ---
namespace {

CRITICAL_SECTION g_logCs;
bool g_logInit = false;
bool g_logOn = false;

void ShimLog(const char* fmt, ...)
{
    if (!g_logOn) return;
    EnterCriticalSection(&g_logCs);
    FILE* f = nullptr;
    if (fopen_s(&f, "C:\\tmp\\dsound_shim.log", "a") == 0 && f)
    {
        va_list ap; va_start(ap, fmt);
        vfprintf(f, fmt, ap);
        va_end(ap);
        fputc('\n', f);
        fclose(f);
    }
    LeaveCriticalSection(&g_logCs);
}

LONGLONG QpcFreq()
{
    static LONGLONG f = 0;
    if (!f) { LARGE_INTEGER li; QueryPerformanceFrequency(&li); f = li.QuadPart; }
    return f;
}

LONGLONG QpcNow()
{
    LARGE_INTEGER li; QueryPerformanceCounter(&li);
    return li.QuadPart;
}

// ------------------------------------------------------------------ buffer --
class ShimBuffer : public IDirectSoundBuffer
{
public:
    ShimBuffer(const WAVEFORMATEX& fmt, DWORD bytes, DWORD flags, bool primary)
        : m_fmt(fmt), m_bytes(bytes), m_flags(flags), m_primary(primary)
    {
        InitializeCriticalSection(&m_cs);
        // Primary buffers carry no PCM of their own -- they exist to hold the
        // output format and the master volume. Allocating for one would just be
        // memory nobody ever reads.
        if (!primary && bytes)
            m_data.resize(bytes, 0);
    }

    ~ShimBuffer() { DeleteCriticalSection(&m_cs); }

    // Shared PCM for DuplicateSoundBuffer: same audio, independent play state.
    void CopyDataFrom(ShimBuffer* src)
    {
        EnterCriticalSection(&src->m_cs);
        m_data = src->m_data;
        m_volume = src->m_volume;
        m_pan = src->m_pan;
        LeaveCriticalSection(&src->m_cs);
    }

    // ---- IUnknown ----------------------------------------------------------
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv) override
    {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IDirectSoundBuffer
            || riid == IID_IDirectSoundBuffer8)
        {
            *ppv = static_cast<IDirectSoundBuffer*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHOD_(ULONG, AddRef)() override { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHOD_(ULONG, Release)() override
    {
        LONG r = InterlockedDecrement(&m_ref);
        if (r == 0) delete this;
        return (ULONG)r;
    }

    // ---- IDirectSoundBuffer ------------------------------------------------
    STDMETHOD(GetCaps)(LPDSBCAPS c) override
    {
        if (!c || c->dwSize < sizeof(DSBCAPS)) return DSERR_INVALIDPARAM;
        c->dwFlags = m_flags;
        c->dwBufferBytes = m_bytes;
        c->dwUnlockTransferRate = 0;
        c->dwPlayCpuOverhead = 0;
        return DS_OK;
    }

    STDMETHOD(GetCurrentPosition)(LPDWORD play, LPDWORD write) override
    {
        EnterCriticalSection(&m_cs);
        Advance();
        DWORD p = (DWORD)m_cursor;
        LeaveCriticalSection(&m_cs);
        if (play) *play = p;
        // The write cursor leads the play cursor. Real hardware keeps it about
        // one mix period ahead; 15 ms is in that range and, crucially, is
        // CONSTANT -- callers that compute a safe write offset from the gap need
        // it not to jitter.
        if (write)
        {
            DWORD lead = m_fmt.nAvgBytesPerSec / 66;
            if (m_fmt.nBlockAlign) lead -= lead % m_fmt.nBlockAlign;
            *write = m_bytes ? (p + lead) % m_bytes : 0;
        }
        return DS_OK;
    }

    STDMETHOD(GetFormat)(LPWAVEFORMATEX fmt, DWORD size, LPDWORD written) override
    {
        DWORD need = sizeof(WAVEFORMATEX) + m_fmt.cbSize;
        if (fmt)
        {
            if (size < sizeof(WAVEFORMATEX)) return DSERR_INVALIDPARAM;
            memcpy(fmt, &m_fmt, sizeof(WAVEFORMATEX));
        }
        if (written) *written = need;
        return DS_OK;
    }

    STDMETHOD(GetVolume)(LPLONG v) override
    { if (!v) return DSERR_INVALIDPARAM; *v = m_volume; return DS_OK; }
    STDMETHOD(GetPan)(LPLONG p) override
    { if (!p) return DSERR_INVALIDPARAM; *p = m_pan; return DS_OK; }
    STDMETHOD(GetFrequency)(LPDWORD f) override
    { if (!f) return DSERR_INVALIDPARAM; *f = m_freq; return DS_OK; }

    STDMETHOD(GetStatus)(LPDWORD s) override
    {
        if (!s) return DSERR_INVALIDPARAM;
        EnterCriticalSection(&m_cs);
        Advance();                       // may end a one-shot that has run out
        DWORD st = 0;
        if (m_playing) st |= DSBSTATUS_PLAYING;
        if (m_looping) st |= DSBSTATUS_LOOPING;
        LeaveCriticalSection(&m_cs);
        *s = st;
        return DS_OK;
    }

    STDMETHOD(Initialize)(LPDIRECTSOUND, LPCDSBUFFERDESC) override { return DS_OK; }

    STDMETHOD(Lock)(DWORD off, DWORD bytes, LPVOID* p1, LPDWORD b1,
                    LPVOID* p2, LPDWORD b2, DWORD flags) override
    {
        if (!p1 || !b1) return DSERR_INVALIDPARAM;
        EnterCriticalSection(&m_cs);
        if (flags & DSBLOCK_ENTIREBUFFER) { off = 0; bytes = m_bytes; }
        if (flags & DSBLOCK_FROMWRITECURSOR)
        {
            Advance();
            off = (DWORD)m_cursor;
        }
        if (m_bytes == 0 || m_data.empty())
        {
            // A primary buffer has no PCM to hand out, but refusing the lock
            // outright makes callers treat the device as broken. Give back an
            // empty region and success.
            *p1 = nullptr; *b1 = 0;
            if (p2) *p2 = nullptr;
            if (b2) *b2 = 0;
            LeaveCriticalSection(&m_cs);
            return DS_OK;
        }
        off %= m_bytes;
        if (bytes > m_bytes) bytes = m_bytes;

        // A lock that runs off the end WRAPS into a second region -- that is the
        // whole reason Lock has two out-pointers, and a caller writing a
        // streaming buffer relies on it.
        DWORD first = bytes;
        if (off + first > m_bytes) first = m_bytes - off;
        *p1 = m_data.data() + off;
        *b1 = first;
        DWORD second = bytes - first;
        if (p2) *p2 = second ? m_data.data() : nullptr;
        if (b2) *b2 = second;
        m_locks++;
        LeaveCriticalSection(&m_cs);
        return DS_OK;
    }

    STDMETHOD(Play)(DWORD, DWORD, DWORD flags) override
    {
        EnterCriticalSection(&m_cs);
        m_looping = (flags & DSBPLAY_LOOPING) != 0;
        // Play() RESUMES from the current position; it does not rewind. D2 calls
        // SetCurrentPosition(0) itself when it wants a sound from the top.
        m_playing = true;
        m_tick = QpcNow();
        LeaveCriticalSection(&m_cs);
        return DS_OK;
    }

    STDMETHOD(SetCurrentPosition)(DWORD pos) override
    {
        EnterCriticalSection(&m_cs);
        m_cursor = m_bytes ? (double)(pos % m_bytes) : 0.0;
        m_tick = QpcNow();
        LeaveCriticalSection(&m_cs);
        return DS_OK;
    }

    STDMETHOD(SetFormat)(LPCWAVEFORMATEX fmt) override
    {
        if (!fmt) return DSERR_INVALIDPARAM;
        // Only the primary buffer's format is settable, which is exactly what
        // an app does to pick the output format.
        if (!m_primary) return DSERR_INVALIDCALL;
        EnterCriticalSection(&m_cs);
        memcpy(&m_fmt, fmt, sizeof(WAVEFORMATEX));
        LeaveCriticalSection(&m_cs);
        return DS_OK;
    }

    STDMETHOD(SetVolume)(LONG v) override
    {
        if (v > DSBVOLUME_MAX || v < DSBVOLUME_MIN) return DSERR_INVALIDPARAM;
        m_volume = v;
        return DS_OK;
    }
    STDMETHOD(SetPan)(LONG p) override
    {
        if (p > DSBPAN_RIGHT || p < DSBPAN_LEFT) return DSERR_INVALIDPARAM;
        m_pan = p;
        return DS_OK;
    }
    STDMETHOD(SetFrequency)(DWORD f) override
    {
        // 0 means "back to the format's own rate".
        EnterCriticalSection(&m_cs);
        m_freq = f ? f : m_fmt.nSamplesPerSec;
        LeaveCriticalSection(&m_cs);
        return DS_OK;
    }

    STDMETHOD(Stop)() override
    {
        EnterCriticalSection(&m_cs);
        Advance();
        m_playing = false;
        LeaveCriticalSection(&m_cs);
        return DS_OK;
    }

    STDMETHOD(Unlock)(LPVOID, DWORD, LPVOID, DWORD) override { return DS_OK; }
    STDMETHOD(Restore)() override { return DS_OK; }

private:
    // Move the cursor to where wall-clock time says it should be. Called under
    // m_cs by anything that reads position or status, so the cursor is only
    // ever as stale as the last question asked about it.
    void Advance()
    {
        if (!m_playing || m_bytes == 0) { m_tick = QpcNow(); return; }
        const LONGLONG now = QpcNow();
        const LONGLONG dt = now - m_tick;
        m_tick = now;
        if (dt <= 0) return;

        // Bytes per second follows the CURRENT frequency, not the format's --
        // SetFrequency is how D2 pitches sounds, and a cursor that ignored it
        // would drift against the audio the game thinks it is playing.
        const double bytesPerSec =
            (double)m_fmt.nBlockAlign * (double)(m_freq ? m_freq : m_fmt.nSamplesPerSec);
        m_cursor += bytesPerSec * (double)dt / (double)QpcFreq();

        if (m_cursor >= (double)m_bytes)
        {
            if (m_looping)
            {
                // Keep the fractional phase across the seam rather than snapping
                // to 0 -- the same lesson the mixer learned the hard way.
                m_cursor = m_cursor - (double)m_bytes * floor(m_cursor / (double)m_bytes);
            }
            else
            {
                // A one-shot STOPS at its end. DirectSound does this without the
                // app calling Stop(), and code that infers "finished" from
                // GetStatus depends on it.
                m_cursor = 0.0;
                m_playing = false;
            }
        }
    }

    LONG  m_ref = 1;
    CRITICAL_SECTION m_cs;
    WAVEFORMATEX m_fmt{};
    std::vector<BYTE> m_data;
    DWORD m_bytes = 0;
    DWORD m_flags = 0;
    bool  m_primary = false;
    bool  m_playing = false;
    bool  m_looping = false;
    LONG  m_volume = DSBVOLUME_MAX;
    LONG  m_pan = DSBPAN_CENTER;
    DWORD m_freq = 0;
    double m_cursor = 0.0;
    LONGLONG m_tick = 0;
    unsigned m_locks = 0;
};

// ------------------------------------------------------------------ device --
class ShimDevice : public IDirectSound
{
public:
    ShimDevice() {}

    STDMETHOD(QueryInterface)(REFIID riid, void** ppv) override
    {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IDirectSound || riid == IID_IDirectSound8)
        {
            *ppv = static_cast<IDirectSound*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHOD_(ULONG, AddRef)() override { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHOD_(ULONG, Release)() override
    {
        LONG r = InterlockedDecrement(&m_ref);
        if (r == 0) delete this;
        return (ULONG)r;
    }

    STDMETHOD(CreateSoundBuffer)(LPCDSBUFFERDESC d, LPDIRECTSOUNDBUFFER* out,
                                 LPUNKNOWN) override
    {
        if (!d || !out) return DSERR_INVALIDPARAM;
        *out = nullptr;
        const bool primary = (d->dwFlags & DSBCAPS_PRIMARYBUFFER) != 0;

        WAVEFORMATEX fmt{};
        if (d->lpwfxFormat)
            memcpy(&fmt, d->lpwfxFormat, sizeof(WAVEFORMATEX));
        else
        {
            // A primary buffer is created with no format; give it a sane default
            // until SetFormat says otherwise.
            fmt.wFormatTag = WAVE_FORMAT_PCM;
            fmt.nChannels = 2;
            fmt.nSamplesPerSec = 22050;
            fmt.wBitsPerSample = 16;
            fmt.nBlockAlign = 4;
            fmt.nAvgBytesPerSec = 22050 * 4;
        }
        if (!primary && (!fmt.nBlockAlign || !fmt.nSamplesPerSec))
            return DSERR_INVALIDPARAM;

        ShimBuffer* b = new ShimBuffer(fmt, primary ? 0 : d->dwBufferBytes,
                                       d->dwFlags, primary);
        b->SetFrequency(0);
        *out = b;
        ShimLog("CreateSoundBuffer %s bytes=%lu %uHz %uch %ubit",
                primary ? "PRIMARY" : "secondary", d->dwBufferBytes,
                fmt.nSamplesPerSec, fmt.nChannels, fmt.wBitsPerSample);
        return DS_OK;
    }

    STDMETHOD(GetCaps)(LPDSCAPS c) override
    {
        if (!c || c->dwSize < sizeof(DSCAPS)) return DSERR_INVALIDPARAM;
        // Claim a capable software device. Reporting zero free buffers is a
        // classic way to make an app decide it cannot play anything.
        c->dwFlags = DSCAPS_CONTINUOUSRATE | DSCAPS_PRIMARY16BIT
                   | DSCAPS_PRIMARYSTEREO | DSCAPS_SECONDARY16BIT
                   | DSCAPS_SECONDARYSTEREO;
        c->dwMinSecondarySampleRate = DSBFREQUENCY_MIN;
        c->dwMaxSecondarySampleRate = DSBFREQUENCY_MAX;
        c->dwPrimaryBuffers = 1;
        c->dwMaxHwMixingAllBuffers = 0;
        c->dwMaxHwMixingStaticBuffers = 0;
        c->dwMaxHwMixingStreamingBuffers = 0;
        c->dwFreeHwMixingAllBuffers = 0;
        c->dwFreeHwMixingStaticBuffers = 0;
        c->dwFreeHwMixingStreamingBuffers = 0;
        c->dwTotalHwMemBytes = 0;
        c->dwFreeHwMemBytes = 0;
        c->dwMaxContigFreeHwMemBytes = 0;
        c->dwUnlockTransferRateHwBuffers = 0;
        c->dwPlayCpuOverheadSwBuffers = 0;
        return DS_OK;
    }

    STDMETHOD(DuplicateSoundBuffer)(LPDIRECTSOUNDBUFFER orig,
                                    LPDIRECTSOUNDBUFFER* dup) override
    {
        if (!orig || !dup) return DSERR_INVALIDPARAM;
        *dup = nullptr;
        ShimBuffer* src = static_cast<ShimBuffer*>(orig);
        WAVEFORMATEX fmt{};
        DWORD written = 0;
        src->GetFormat(&fmt, sizeof(fmt), &written);
        DSBCAPS caps{}; caps.dwSize = sizeof(caps);
        src->GetCaps(&caps);
        ShimBuffer* b = new ShimBuffer(fmt, caps.dwBufferBytes, caps.dwFlags, false);
        b->CopyDataFrom(src);       // same audio, its own cursor and play state
        *dup = b;
        ShimLog("DuplicateSoundBuffer bytes=%lu", caps.dwBufferBytes);
        return DS_OK;
    }

    STDMETHOD(SetCooperativeLevel)(HWND, DWORD) override { return DS_OK; }
    STDMETHOD(Compact)() override { return DS_OK; }
    STDMETHOD(GetSpeakerConfig)(LPDWORD cfg) override
    { if (cfg) *cfg = DSSPEAKER_STEREO; return DS_OK; }
    STDMETHOD(SetSpeakerConfig)(DWORD) override { return DS_OK; }
    STDMETHOD(Initialize)(LPCGUID) override { return DS_OK; }

private:
    LONG m_ref = 1;
};

}  // namespace

// ------------------------------------------------------------------ exports -
extern "C" {

HRESULT WINAPI Shim_DirectSoundCreate(LPCGUID, LPDIRECTSOUND* out, LPUNKNOWN outer)
{
    if (!out) return DSERR_INVALIDPARAM;
    if (outer) return DSERR_NOAGGREGATION;
    *out = new ShimDevice();
    ShimLog("DirectSoundCreate -> shim device");
    return DS_OK;
}

HRESULT WINAPI Shim_DirectSoundCreate8(LPCGUID g, LPDIRECTSOUND8* out, LPUNKNOWN outer)
{
    return Shim_DirectSoundCreate(g, (LPDIRECTSOUND*)out, outer);
}

// One device, always present. The description matters: an app that filters on
// the name would otherwise not find anything it recognises.
HRESULT WINAPI Shim_DirectSoundEnumerateA(LPDSENUMCALLBACKA cb, LPVOID ctx)
{
    if (!cb) return DSERR_INVALIDPARAM;
    cb(nullptr, "Primary Sound Driver", "", ctx);
    return DS_OK;
}

HRESULT WINAPI Shim_DirectSoundEnumerateW(LPDSENUMCALLBACKW cb, LPVOID ctx)
{
    if (!cb) return DSERR_INVALIDPARAM;
    cb(nullptr, L"Primary Sound Driver", L"", ctx);
    return DS_OK;
}

// Capture is genuinely not implemented -- D2 never records. Failing honestly
// beats handing back a device that produces nothing but silence.
HRESULT WINAPI Shim_DirectSoundCaptureCreate(LPCGUID, void**, LPUNKNOWN)
{ return DSERR_NODRIVER; }
HRESULT WINAPI Shim_DirectSoundCaptureCreate8(LPCGUID, void**, LPUNKNOWN)
{ return DSERR_NODRIVER; }
HRESULT WINAPI Shim_DirectSoundCaptureEnumerateA(LPDSENUMCALLBACKA, LPVOID)
{ return DS_OK; }
HRESULT WINAPI Shim_DirectSoundCaptureEnumerateW(LPDSENUMCALLBACKW, LPVOID)
{ return DS_OK; }
HRESULT WINAPI Shim_DirectSoundFullDuplexCreate(LPCGUID, LPCGUID, LPCDSCBUFFERDESC,
                                                LPCDSBUFFERDESC, HWND, DWORD,
                                                void**, void**, void**, LPUNKNOWN)
{ return DSERR_NODRIVER; }
HRESULT WINAPI Shim_GetDeviceID(LPCGUID src, LPGUID dst)
{
    if (!dst) return DSERR_INVALIDPARAM;
    if (src) *dst = *src; else ZeroMemory(dst, sizeof(GUID));
    return DS_OK;
}
HRESULT WINAPI Shim_DllCanUnloadNow() { return S_FALSE; }
HRESULT WINAPI Shim_DllGetClassObject(REFCLSID, REFIID, void** ppv)
{
    if (ppv) *ppv = nullptr;
    return CLASS_E_CLASSNOTAVAILABLE;
}

}  // extern "C"

BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!g_logInit) { InitializeCriticalSection(&g_logCs); g_logInit = true; }
        // Opt-in logging: writing a file on every buffer create is not something
        // to do by default in a shipped game directory.
        g_logOn = GetEnvironmentVariableA("D2SHIM_LOG", nullptr, 0) != 0;
        ShimLog("---- dsound shim attached ----");
    }
    return TRUE;
}
