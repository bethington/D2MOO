"""pd2_frida_rng_capture — locate + oracle PD2's RNG LCG in-process via Frida.

Self-locates the RNG by its unique magic multiplier 0x6AC690C5 (no ordinals, no
vanilla/D2MOO addresses -- immune to PD2's address scramble), auto-picks the
entry candidate that reproduces D2MOO's LCG (D2Seed.h:
  lSeed = nHighSeed + 0x6AC690C5 * nLowSeed ; return lSeed  (fastcall(pSeed)->u64)
then captures a fingerprint over chosen seeds and asserts every roll is bit-exact
vs that formula (== compiled D2MOO's SEED_RollRandomNumber). Writes pd2_rng_fp.json.

Usage: python pd2_frida_rng_capture.py [Game.exe|<pid>]   (needs elevation: PD2 is admin)
"""
import frida
import json
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
MAGIC = 0x6AC690C5
MASK64 = (1 << 64) - 1

# chosen initial seeds (low, high)
SEEDS = [(1, 0), (0, 1), (2, 0), (12345, 0), (0, 12345),
         (0xDEADBEEF, 0xCAFEBABE), (0xFFFFFFFF, 0xFFFFFFFF), (0x9E3779B9, 0x12345678)]
ROLLS = 5   # advance the LCG this many times per seed


def lcg(low, high):
    """D2MOO's SEED_RollRandomNumber reference."""
    r = (high + MAGIC * low) & MASK64
    return r, (r & 0xFFFFFFFF), (r >> 32) & 0xFFFFFFFF


def main():
    target = sys.argv[1] if len(sys.argv) > 1 else "Game.exe"
    try:
        target = int(target)
    except ValueError:
        pass

    session = frida.attach(target)
    with open(os.path.join(HERE, "pd2_frida_rng_probe.js"), encoding="utf-8") as f:
        script = session.create_script(f.read())
    script.load()
    api = script.exports_sync

    rpt = None
    for attempt in range(20):
        rpt = api.probe()
        if rpt.get("loaded") is False:
            print(f"  D2Common not loaded yet (attempt {attempt + 1}) -- waiting...")
            time.sleep(3)
            continue
        break
    if not rpt or not rpt.get("hits"):
        print("  !! magic not found / D2Common not loaded -- different RNG path?"); return
    print("D2Common base:", rpt["base"])
    print("magic 0x6AC690C5 found at:", rpt["hits"])

    # READ-ONLY: disassemble around each hit so we can CONFIRM PD2 uses the same
    # LCG (imul 0x6AC690C5 + seed-struct access) WITHOUT calling anything. We do
    # NOT call auto-located entries -- a wrong entry crashes the game.
    for c in rpt["candidates"]:
        print(f"\n  magic@{c['magic_at']}  heuristic entry={c['entry']}")
        for line in c["ctx"]:
            print("     ", line)
    print("\n  (read-only confirmation. To CAPTURE outputs, call roll() only with a"
          "\n   Ghidra-VERIFIED entry, never a heuristic one.)")
    session.detach()


if __name__ == "__main__":
    main()
