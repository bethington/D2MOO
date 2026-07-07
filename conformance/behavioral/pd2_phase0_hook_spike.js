'use strict';
// pd2_phase0_hook_spike -- Phase 0 of ../LIVE_DISPATCH_FRAMEWORK_PLAN.md.
//
// THE QUESTION: does PD2 TOLERATE an inline prologue hook (the Detours mechanism)
// on a live D2Common function -- without tripping its anti-debug / integrity
// checks or crashing? Everything in the dispatch framework is gated on this.
//
// WHY THIS IS NEW: the existing behavioral/ oracle only CALLED functions
// (NativeFunction) -- it never modified a prologue. Frida's Interceptor does
// exactly what Microsoft Detours does (what D2.Detours uses, DetoursPatch.cpp:113
// `DetourAttach`): overwrite the target's prologue with a jump and build a
// trampoline back to the real body. So an Interceptor hook that installs, fires,
// and leaves the game running/correct is direct evidence that Detours will too.
//
// TARGET: DUNGEON_GameTileToClientCoords @ D2Common+0x4dac0 (Ghidra 0x6FD9DAC0,
// real ordinal @10375). void __stdcall(int* pX, int* pY):
//   *pX = (x - y) * 80 ; *pY = ((x + y) * 80) >> 1   -- proven bit-exact
//   (PD2S12Conformance.cpp). It also fires constantly during in-game rendering,
//   so `attachHits` climbs on its own once you are in a level -- bonus signal.
//
// The driver (pd2_phase0_spike.py) is SELF-DRIVING: after installing a hook it
// calls the target itself with known inputs, so the spike is conclusive from the
// MAIN MENU with no gameplay needed.

const OFF = 0x4dac0;
const mod = Process.getModuleByName('D2Common.dll');
const target = mod.base.add(OFF);

let attachHits = 0;
let replaceHits = 0;
let mode = 'none';

// A NativeFunction bound to the raw address so the driver can drive the target
// THROUGH whatever hook is installed (a call to `target` hits the attach hook /
// the replacement, exactly like the game's own calls do).
const callTarget = new NativeFunction(target, 'void', ['pointer', 'pointer'],
                                      { abi: 'stdcall' });

// Original impl captured BEFORE any replace, for the replacement to call the real
// body. (For attach this is unused -- attach doesn't divert the return path.)
const origImpl = new NativeFunction(target, 'void', ['pointer', 'pointer'],
                                    { abi: 'stdcall' });

// --- Probe A: observe-only inline hook (prologue splice + trampoline) ----------
// This alone answers the Phase 0 tolerance question: Interceptor.attach patches
// the prologue just like Detours. Zero divert of behaviour -> safest first probe.
function installAttach() {
  Interceptor.attach(target, { onEnter(_args) { attachHits++; } });
  Interceptor.flush();
  mode = 'attach';
}

// --- Probe B (opt-in): full replace, mirroring the dispatcher REIMPL/SHADOW route
// Our stub runs, then calls the real body via the pre-captured trampoline so the
// game still observes the correct result. A hard re-entry guard prevents an
// accidental infinite recursion from CRASHING the game (which would masquerade as
// a false "PD2 detected the hook").
let inRepl = false;
let replacement = null;
function installReplace() {
  replacement = new NativeCallback(function (pX, pY) {
    replaceHits++;
    if (inRepl) { return; }             // guard: never recurse into ourselves
    inRepl = true;
    try { origImpl(pX, pY); } finally { inRepl = false; }
  }, 'void', ['pointer', 'pointer'], 'stdcall');
  Interceptor.replace(target, replacement);
  Interceptor.flush();
  // NOTE: the module-level `replacement` var above retains the NativeCallback so
  // Frida won't GC it -- do NOT reference `global`, which is undefined in Frida's
  // runtime (use module scope or globalThis).
  mode = 'replace';
}

// Call the target with (x, y) through whatever hook is installed; return [ox, oy].
function drive(x, y) {
  const px = Memory.alloc(4), py = Memory.alloc(4);
  px.writeS32(x); py.writeS32(y);
  callTarget(px, py);
  return [px.readS32(), py.readS32()];
}

rpc.exports = {
  base: () => mod.base.toString(),
  target: () => target.toString(),
  installAttach: () => { installAttach(); return mode; },
  installReplace: () => { installReplace(); return mode; },
  drive: (x, y) => drive(x, y),
  stats: () => ({ mode, attachHits, replaceHits }),
  revert: () => {
    Interceptor.revert(target); Interceptor.flush();
    replacement = null; mode = 'reverted'; return mode;
  },
};
