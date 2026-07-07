"""pd2_phase0_spike -- Phase 0 of ../LIVE_DISPATCH_FRAMEWORK_PLAN.md.

Proves (or disproves) that PD2 TOLERATES an inline prologue hook -- the Detours
mechanism -- on a live D2Common function, using Frida's Interceptor (which patches
the prologue exactly as Detours does). If this passes, the live-dispatch framework
is unblocked; if the game detects it or crashes, we learn that here, cheaply,
before building anything on top.

SELF-DRIVING: after installing a hook the driver calls the target itself with
known inputs, so the spike is conclusive from the MAIN MENU with no gameplay. The
proven vectors (PD2S12Conformance.cpp) are the correctness oracle:
    (5, 3)   -> (160, 320)
    (-7, 2)  -> (-720, -200)
    (0, 0)   -> (0, 0)

Usage (from an ELEVATED shell, PD2 running & at least at the main menu):
    python pd2_phase0_spike.py [Game.exe|<pid>] [--replace]
Prefer the one-command launcher: run-phase0-spike.ps1 (self-elevates, provisions
a frida venv, launches the game if needed, runs this).
"""
import json
import os
import sys
import time

import frida

HERE = os.path.dirname(os.path.abspath(__file__))
VERDICT = os.path.join(HERE, "phase0_verdict.json")

# (x, y) -> expected (outX, outY), from the proven PD2-S12 coord vectors.
CASES = [((0, 0), (0, 0)), ((5, 3), (160, 320)), ((-7, 2), (-720, -200)),
         ((1, 0), (80, 40)), ((10, -4), (1120, 240))]


def _load(target):
    session = frida.attach(target)
    with open(os.path.join(HERE, "pd2_phase0_hook_spike.js"), encoding="utf-8") as f:
        script = session.create_script(f.read())
    script.load()
    return session, script.exports_sync


def _check_outputs(api, label):
    """Drive the target through the installed hook. Returns (outputs_correct,
    process_alive). A crash/detach surfaces as an exception from api.drive ->
    alive=False, which is the real signal we care about (integrity intolerance)."""
    ok = True
    try:
        for (x, y), (ex, ey) in CASES:
            ox, oy = api.drive(x, y)
            good = (ox == ex and oy == ey)
            ok = ok and good
            print(f"    {label} ({x:4},{y:3}) -> ({ox:6},{oy:5})  "
                  f"{'OK' if good else f'MISMATCH exp ({ex},{ey})'}")
    except Exception as e:      # noqa: BLE001 -- any failure here == process gone
        print(f"    {label}: EXCEPTION driving target ({e!r}) -- process likely crashed/detached")
        return False, False
    return ok, True


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    do_replace = "--replace" in sys.argv
    target = args[0] if args else "Game.exe"
    try:
        target = int(target)
    except ValueError:
        pass

    print("=== PD2 Phase 0 inline-hook tolerance spike ===")
    session, api = _load(target)
    print("  D2Common base:", api.base(), " target:", api.target())

    verdict = {"base": api.base(), "target": api.target()}

    # --- Probe A: observe-only inline hook (prologue splice + trampoline) -------
    print("\n[A] Interceptor.attach (observe-only prologue hook)")
    print("    installed:", api.install_attach())
    a_ok, alive_a = _check_outputs(api, "attach")
    if alive_a:
        time.sleep(2.0)   # let any in-game rendering calls accumulate too
    st = api.stats() if alive_a else {"attachHits": 0, "replaceHits": 0}
    print(f"    stats: {st}")
    a_fired = st["attachHits"] >= len(CASES)
    print(f"    hook fired: {a_fired}   outputs correct: {a_ok}   process alive: {alive_a}")
    verdict["A_attach"] = {"fired": a_fired, "outputs_correct": a_ok,
                           "alive": alive_a, "hits": st["attachHits"]}

    # --- Probe B (opt-in): full replace, dispatcher REIMPL/SHADOW route ---------
    # Wrapped so a Probe B failure never discards Probe A's (already gating) result.
    if do_replace and alive_a:
        print("\n[B] Interceptor.replace (full replacement -> trampoline -> original)")
        try:
            api.revert()                       # drop probe A first
            print("    installed:", api.install_replace())
            b_ok, alive_b = _check_outputs(api, "replace")
            st = api.stats() if alive_b else {"replaceHits": 0}
            print(f"    stats: {st}")
            b_fired = st.get("replaceHits", 0) >= len(CASES)
            print(f"    replacement fired: {b_fired}   outputs correct: {b_ok}   "
                  f"process alive: {alive_b}")
            verdict["B_replace"] = {"fired": b_fired, "outputs_correct": b_ok,
                                    "alive": alive_b, "hits": st.get("replaceHits", 0)}
            if alive_b:
                api.revert()
        except Exception as e:  # noqa: BLE001 -- harness/RPC error, not a PD2 verdict
            print(f"    Probe B harness error (not a PD2 tolerance result): {e!r}")
            verdict["B_replace"] = {"error": repr(e)}

    # --- Verdict ---------------------------------------------------------------
    passA = a_ok and alive_a and a_fired
    verdict["PASS"] = passA
    print("\n=== VERDICT ===")
    print(f"  A (attach) : {'PASS' if passA else 'FAIL'} "
          "-- inline prologue hook installs, fires, game survives & stays correct")
    if "B_replace" in verdict:
        b = verdict["B_replace"]
        passB = b["outputs_correct"] and b["alive"] and b["fired"]
        print(f"  B (replace): {'PASS' if passB else 'FAIL'} "
              "-- full replacement + trampoline-to-original tolerated")
    print("\n  PASS => PD2 tolerates inline hooking; the Detours dispatch framework"
          "\n          is unblocked. Proceed to Phase 1 (native dispatcher).")
    print("  FAIL/crash => integrity/anti-debug intolerance; resolve before building.")

    with open(VERDICT, "w", encoding="utf-8") as f:
        json.dump(verdict, f, indent=2)
    print("  wrote", VERDICT)

    try:
        session.detach()
    except Exception:  # noqa: BLE001
        pass
    return 0 if passA else 1


if __name__ == "__main__":
    sys.exit(main())
