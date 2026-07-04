"""pd2_frida_rng_vectors — behavioral golden vectors for PD2-S12 SEED_GetRandomNumber.

Calls the Ghidra-VERIFIED entry (D2Common+0x510B0) via a trampoline (custom ABI:
pSeed in ECX, max in EAX) with chosen (seed, max), captures (ret, new-seed), and
verifies every result bit-exact against D2MOO's SEED_RollLimitedRandomNumber
(D2Seed.h). SAFETY: verifies the FIRST call vs the formula and ABORTS on mismatch
(never hammers a broken call). Writes pd2_rng_vectors.json.

Usage: python pd2_frida_rng_vectors.py [Game.exe|<pid>]   (PD2 is elevated)
"""
import frida
import json
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
MAGIC = 0x6AC690C5
MASK64 = (1 << 64) - 1

# (low, high, max); each rolled ROLLS times (advancing the seed) with that max.
VECTORS = [
    (1, 0, 6), (0, 1, 6), (3, 7, 1000), (12345, 0, 256), (0, 12345, 1024),
    (0xDEADBEEF, 0xCAFEBABE, 100), (0x9E3779B9, 0x12345678, 65536),
    (0xFFFFFFFF, 0xFFFFFFFF, 3), (5, 3, 2), (7, 11, 1),
]
ROLLS = 5


def ref(low, high, mx):
    """D2MOO SEED_RollLimitedRandomNumber reference (D2Seed.h + PD2 decompile)."""
    if mx < 1:                       # (int)max < 1 -> return 0, seed unchanged
        return 0, low, high
    r = ((low * MAGIC) + high) & MASK64
    nlow, nhigh = r & 0xFFFFFFFF, (r >> 32) & 0xFFFFFFFF
    low32 = r & 0xFFFFFFFF
    if (mx & (mx - 1)) == 0:         # power of 2
        ret = low32 & (mx - 1)
    else:
        ret = low32 % mx
    return ret, nlow, nhigh


def main():
    target = sys.argv[1] if len(sys.argv) > 1 else "Game.exe"
    try:
        target = int(target)
    except ValueError:
        pass

    session = frida.attach(target)
    with open(os.path.join(HERE, "pd2_frida_rng_trampoline.js"), encoding="utf-8") as f:
        script = session.create_script(f.read())
    script.load()
    api = script.exports_sync

    info = None
    for attempt in range(20):
        info = api.ready()
        if info:
            break
        print(f"  D2Common not loaded yet (attempt {attempt + 1})...")
        time.sleep(3)
    if not info:
        print("  !! D2Common never loaded"); return
    print(f"D2Common base {info['base']}  SEED_GetRandomNumber entry {info['entry']}")

    # SAFETY GATE: verify the first call before capturing anything.
    try:
        r = api.call(3, 7, 1000)
    except Exception as e:
        print(f"  !! FIRST CALL FAULTED ({e}) -- aborting. Frida trapped it; game is safe.")
        return
    er, elo, ehi = ref(3, 7, 1000)
    if not (r["ret"] == er and r["newLow"] == elo and r["newHigh"] == ehi):
        print(f"  !! FIRST CALL MISMATCH -- aborting (not hammering).")
        print(f"     got ret={r['ret']} new=({r['newLow']:#x},{r['newHigh']:#x})")
        print(f"     exp ret={er} new=({elo:#x},{ehi:#x})")
        return
    print(f"  first-call check OK (3,7,1000) -> ret={r['ret']}  [safe to capture]")

    # capture + verify
    out = {}
    all_ok = True
    for (low, high, mx) in VECTORS:
        seq = []
        L, H = low, high
        for _ in range(ROLLS):
            g = api.call(L, H, mx)
            er, elo, ehi = ref(L, H, mx)
            ok = (g["ret"] == er and g["newLow"] == elo and g["newHigh"] == ehi)
            all_ok = all_ok and ok
            seq.append({"in": [L, H, mx], "ret": g["ret"],
                        "new": [g["newLow"], g["newHigh"]], "ok": ok})
            L, H = g["newLow"], g["newHigh"]
        out[f"{low:#x},{high:#x},max={mx}"] = seq
    with open(os.path.join(HERE, "pd2_rng_vectors.json"), "w", encoding="utf-8") as f:
        json.dump(out, f, indent=0)
    session.detach()

    n = sum(len(v) for v in out.values())
    print(f"\n  {n} calls across {len(VECTORS)} (seed,max) vectors; "
          + ("ALL BIT-EXACT vs D2MOO LCG" if all_ok else "DIVERGENCE!"))
    print("  wrote pd2_rng_vectors.json")


if __name__ == "__main__":
    main()
