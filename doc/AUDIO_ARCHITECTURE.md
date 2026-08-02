# Audio architecture

How sound gets from Diablo II to your speakers, to the debugger's Game panel,
and to a remote client — and why each piece is shaped the way it is.

Every claim here was measured. Where a design looks odd, the reason is usually
that the obvious version was tried first and failed in a specific, recorded way.

## The whole path

```
                D2 game code
                     |  writes PCM into DirectSound secondary buffers
                     v
         +-------------------------+
         |  dsound.dll             |   subprojects/dsound-headless
         |  (deviceless)           |   Buffers are plain memory. The play
         |                         |   cursor is arithmetic over QPC.
         +-------------------------+   NOTHING is rendered to any device.
                     |
                     |  Detours hooks on the buffer vtable
                     v
         +-------------------------+
         |  D2Debugger.audiocap    |   Shadow-copies every buffer write,
         |  capture + mixer        |   tracks volume/pan/cursor/status,
         |                         |   mixes all ~20 sources into ONE stream.
         +-------------------------+
                     |
                     v
              +--------------+
              |   THE RING   |  1.49 s, 44.1 kHz / 16-bit / stereo, interleaved
              +--------------+  single writer, one cursor per consumer
                  /        \
                 /          \
                v            v
   +---------------------+   +----------------------------+
   | RenderThread        |   | D2Debugger.audiostream     |
   | WASAPI, event-driven|   | libFLAC -> WebSocket :8791 |
   | local speakers      |   | remote client              |
   +---------------------+   +----------------------------+
```

The ring is the seam the whole design turns on. Everything upstream of it is
about *obtaining* audio; everything downstream is a consumer that cannot affect
the others.

## 1. The game does not mix

D2 writes each sound into its own DirectSound secondary buffer — about 20
registered, 5–9 sounding at once — and lets the system mix them. There is no
"post-game-mix" buffer to grab, which is why the capture happens at the buffer
level and the mixing is ours.

That also makes byte-exactness against a system mix impossible in principle:
the OS mixes in float and resamples 44.1 kHz sources to the endpoint's rate.
Score correlation and gain, never bytes.

## 2. dsound-headless — a DirectSound that needs no audio device

See [subprojects/dsound-headless](../subprojects/dsound-headless/README.md).

With the Windows audio services stopped, `DirectSoundCreate8` returns
`DSERR_NODRIVER` and only the "Primary Sound Driver" placeholder enumerates. No
device means no `IDirectSound`, no secondary buffers, and therefore no PCM
anywhere for the capture layer to read. D2 additionally needs `-ns` to start at
all. Video survives Session 0 and a missing device; audio does not.

So we don't wrap the audio system, we *are* it. Buffers are heap memory, the
play cursor is arithmetic over `QueryPerformanceCounter`, and nothing touches an
endpoint, a driver or a mixer. Its import table is `KERNEL32` and nothing else.

**Consequence worth internalising: while the shim is installed, our mixer is the
only audio path in the process.** The game's "native" output does not exist.
That is not a bug and not something to debug — it is the design.

## 3. Capture — two sources, hooks are the fallback

`/audio` reports which one is live as `captureMode`.

**`shim` (preferred).** When the game is running dsound-headless, the capture
asks that DLL for the audio instead of patching it: `D2SndCap_Snapshot` returns
every live buffer with format, gain, pan, play state and cursor; `D2SndCap_Read`
copies the window of PCM a tick needs. Nothing is hooked, `detourDevErr` /
`detourBufErr` stay `-1`, and two whole classes of failure disappear —
**the install-ordering race** (a buffer created before a vtable patch is
invisible forever; that is what made menu audio silent while in-world worked)
and **Detours transaction contention** with the launcher. It is also more
accurate: the snapshot reports the *effective* rate, so `SetFrequency`-pitched
sounds resample correctly, which the hook path never handled.

Selected by a version handshake — `D2SndCap_Abi() == 1`. An absent export (stock
Microsoft `dsound.dll`) or an unknown ABI falls through to hooking rather than
guessing at a struct layout.

**`hooks` (fallback).** Everything below. Still required against real
DirectSound, and still the only way to A/B our reconstruction against the
system mixer.

### Hooking the buffer vtable

`source/D2Debugger/src/D2Debugger.audiocap.cpp`.

We create one `IDirectSound` purely to read its vtable, then Detour
`CreateSoundBuffer` (index 3) and `DuplicateSoundBuffer` (5). On the first
buffer we patch the buffer vtable once — all buffers from a device share it, and
they are told apart by `this`:

| Hooked | Index | Why |
| --- | --- | --- |
| `Lock` / `Unlock` | 11 / 19 | shadow-copy the PCM the game writes |
| `Play` / `Stop` | 12 / 18 | play state, loop flag, cursor reset |
| `SetVolume` / `SetPan` | 15 / 16 | the gain the game *intends* |

| Called, not hooked | Index | Why |
| --- | --- | --- |
| `GetCurrentPosition` | 4 | where DirectSound actually is |
| `GetVolume` | 6 | cross-check our recorded gain |
| `GetStatus` | 9 | still sounding? |

`SetPan` is **16, not 17** — 17 is `SetFrequency`, and reading it as pan
silences a channel outright.

Hook bodies are SEH-guarded and do the minimum on the game's audio thread:
record state, memcpy, return. Each splits into a `Rec_*` helper doing the
locking work and a thin wrapper holding the `__try`, because MSVC rejects
`__try` in any function needing object unwinding (C2712).

### Traps already paid for

- **`DSBLOCK_FROMWRITECURSOR` ignores the offset you passed.** Trusting the
  caller's `off` files every streamed chunk at the wrong place. Ask
  `GetCurrentPosition` for the real write cursor.
- **Back-fill on registration.** D2 preloads sounds before our hook exists, so a
  new buffer is read once with `DSBLOCK_ENTIREBUFFER` or it stays silent forever.
- **Duplicates share audio, not state.** D2 duplicates heavily for concurrent
  SFX; a duplicate needs the source's PCM but its own cursor, volume and pan.
- **`GLOBALFOCUS` must actually apply.** The flag injection was gated on
  `dwSize >= sizeof(DSBUFFERDESC)` (36, the modern layout). **D2 is DirectSound 7
  and passes `DSBUFFERDESC1` — 20 bytes** — so the test failed for every buffer
  and the flag was never set, for years, while the comment claimed otherwise.
  Accept `20 <= dwSize <= sizeof(DSBUFFERDESC)`, `memcpy` `dwSize` bytes and OR
  the flags in place (`dwFlags` is at offset 4 in both layouts). Verify with
  `descPatched` / `descPassthru` / `descSize` in `/audio`.

## 4. The mixer — producer into the ring

One block per 10 ms tick on a high-resolution waitable timer. Per source:
gain from `SetVolume`, pan, nearest-neighbour rate conversion (22050 → 44100, or
everything plays an octave out), summed, then the primary buffer's volume as a
master gain — which is what makes the in-game sliders move our audio the way
they move the game's.

**The block size comes from the clock, not the tick count.** Emitting a fixed
441 frames per wake assumes the wake was exactly 10 ms; a periodic timer fires
late, never early. Measured: a fixed block produced ~0.8 % slow — 13 underruns
and 5325 frames of inserted silence in 16 s *with nothing playing*. Frames owed
are `elapsed_QPC × rate − already_produced`, with a 100 ms catch-up ceiling that
restarts the epoch rather than mixing a multi-second block under the lock.

**Cursor tracking follows DirectSound, but does not obey it every tick.** The
reported position is in driver-sized blocks, not sample-exact; resyncing every
tick re-mixes or drops a ~10 ms fragment 100 times a second. Sustained music
absorbs that, a footstep's attack transient does not. So:

| Drift | Action | Counter |
| --- | --- | --- |
| < 10 ms | ignore | — |
| 10–100 ms | slew 5 % of the gap per tick (inaudible) | `slews` |
| > 100 ms, or `Play`/seek/retrigger | snap | `resyncs` |

The slew band exists because the producer is now CPU-clocked while DirectSound
is device-clocked, so small drift is real skew rather than noise. Drift on a
**looping** buffer is measured the short way round the circle — a linear
difference jumps to a whole buffer length the instant either side wraps.

Non-looping buffers must **stop** at the end of data, not wrap. `GetStatus` is
authoritative for whether a sound is still going: a one-shot ends without D2
calling `Stop()`, and inferring from `Play`/`Stop` alone left every one-shot
looping forever. Fixing that alone took correlation 0.60 → 0.96.

## 5. The ring

1.49 s, power-of-two, single writer publishing with release; each consumer owns
its cursor and only ever reads. Adding a consumer costs the producer nothing.

The ring always carries the **true** mix. Audibility — focus, `playLocal`,
source mode — is applied by the *local* consumer, never at the producer. A
defocused, hidden or muted game must keep streaming real audio to a remote
client; that is the entire point of remote play.

## 6. Local playback — WASAPI

Shared mode, event-driven: the device asks for audio instead of us polling for
permission to hand it some. 50 ms cushion, 200 ms trim point, and a priming
state so a single underrun rebuilds the cushion instead of machine-gunning.

The device-acquire loop retries every 5 s, so a machine with no endpoint at all
parks harmlessly while the producer keeps filling the ring for other consumers.

This replaced `waveOut` with 4 × 10 ms buffers polled on `Sleep(2)` — ~40 ms of
cushion, paced by the device handing buffers back, with no way to count a miss.
That was the stutter.

## 7. Remote stream — FLAC over WebSocket

`source/D2Debugger/src/D2Debugger.audiostream.cpp`, listening on
**`ws://127.0.0.1:8791`**.

- **One encoder, many clients.** Encoding per-client would multiply CPU inside
  the game process for identical bytes. The STREAMINFO header is cached and
  replayed to each joiner; FLAC audio frames are independently decodable against
  it, so a mid-stream join is clean.
- **1024-sample blocks** (23 ms) — trading a little ratio for latency, which is
  the right way round for interactive play.
- **Idle costs nothing.** With no client connected the encoder is torn down and
  the ring is not read. A streaming feature nobody is using must not cost frames.
- **Loopback only.** This ships the game's audio; it does not go on the LAN until
  there is an authentication story. Tunnel it deliberately.
- Lossless was a deliberate choice over Opus: at 1.4 Mbit/s raw, audio is noise
  next to the video stream, so compression buys bandwidth rather than
  feasibility.

Verify end to end with `conformance/tools/audio_stream_client.py`, which speaks
the handshake and framing with raw sockets on purpose — a library that tolerates
a sloppy handshake would hide the bug worth finding.

## 8. Panel controls

The Game panel's **Audio** checkbox drives `playLocal`. Right-Shift-click
selects loopback source mode instead of the mixer. State persists in `imgui.ini`
as `Audio=` / `AudioLoop=`, reapplied on the first frame with **source mode set
before `playLocal`**, so a loopback boot never flashes the mixer's mute.

Read the modifier with `D2VInput_RealKeyDown(VK_RSHIFT)`, **not**
`ImGui::GetIO().KeyShift` — in virtual-input mode our own `GetKeyState` hook
feeds ImGui synthetic state and every shift-click option silently stops working.

Note that loopback mode is meaningless while dsound-headless is installed:
there is no native rendering to tap.

## 9. Diagnostics

`GET /audio` is the health surface. The counters exist so a fault is
*attributable* rather than arguable — add one per hypothesis and let it falsify
you.

| Field | Reads as |
| --- | --- |
| `outUnderruns`, `outUnderrunFrames` | gaps: the local consumer starved |
| `lateTicks`, `maxGapUs` | gaps: the producer woke late |
| `maxTickUs` | the mix work itself overran (different disease) |
| `slews` vs `resyncs` | repeats: gentle corrections vs audible snaps |
| `resyncs ÷ plays` | ≈ 1.0 is healthy; ≫ 1 means the clocks are fighting |
| `driftMaxFrames` | worst cursor disagreement seen |
| `producedFrames` | ÷ wall time = the producer's true rate |
| `sessionMuted` | read from the OS, not remembered — a stale cache here once cost an afternoon |
| `descPatched` / `descPassthru` | did the GLOBALFOCUS injection apply? |
| `renderDevice` | false + healthy everything else = the headless case |

Other endpoints: `/audio/buffers` (per-buffer `rms` vs `pcmPeak` separates
"never captured it" from "captured it, never played it"), `/audio/trace/<n>`
(write/read events with peaks), `/audio/stream` (client count, encoder state,
bytes), `/audio/refnoise` (what is the endpoint rendering *right now*).

`/audio/verify` compares our mix against the system's native mix. **It cannot
work while dsound-headless is installed** — the reference is silence by
construction. Restore the system `dsound.dll` first. Its `dir` parameter needs
forward slashes (the JSON parser drops `\\` escapes and you will analyse stale
WAVs), and any correlation must be scored with sample-accurate alignment.

## Rules of thumb

1. **The ring is the product.** Local playback is a convenience; a consumer that
   breaks must not be able to affect the stream.
2. **Never gate the producer on local state.** Focus, mute and device presence
   belong to consumers.
3. **Check the import table before theorising about a silent audio path.** It
   would have answered "why is there no native audio" in ten seconds.
4. **Verify the harness before trusting it.** A system-wide loopback reference
   once produced a phantom +22.7 dB deficit that five hypotheses were chased
   against; process loopback moved it to −2.9 dB instantly.
5. **Add a counter per hypothesis.** Every real fix here was found by a number,
   not by an argument.
