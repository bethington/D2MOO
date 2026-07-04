"""pd2_frida_rng_family — behavioral golden vectors for the PD2-S12 SEED_* family.

Calls SEED_GetRandomNumber (max in EAX), SEED_GetRandomNumberAlt (max in ESI) and
SEED_GetRandomInRange (nMax in EAX, nMin in EDI) via Ghidra-verified trampolines,
and checks every result bit-exact vs the D2MOO/PD2 reference. Proves the trampoline
technique across three different Blizzard register conventions. Per-function safety
gate: verify the first call (or catch a Frida-trapped fault) before capturing.

Usage: python pd2_frida_rng_family.py [Game.exe|<pid>]   (PD2 is elevated)
"""
import frida
import json
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
MAGIC = 0x6AC690C5
MASK64 = (1 << 64) - 1

LIMITED = [(1, 0, 6), (0, 1, 6), (3, 7, 1000), (12345, 0, 256), (0, 12345, 1024),
           (0xDEADBEEF, 0xCAFEBABE, 100), (0x9E3779B9, 0x12345678, 65536),
           (0xFFFFFFFF, 0xFFFFFFFF, 3), (5, 3, 2), (7, 11, 1)]
INRANGE = [(1, 0, 5, 10), (3, 7, -3, 3), (12345, 0, 0, 255),
           (0xDEADBEEF, 0xCAFEBABE, 100, 200), (5, 3, 1, 6), (0, 1, -100, 100),
           (0x9E3779B9, 0x12345678, 0, 999999)]
ROLLS = 5


def adv(low, high):
    r = ((low * MAGIC) + high) & MASK64
    return r, r & 0xFFFFFFFF, (r >> 32) & 0xFFFFFFFF


def ref_limited(low, high, mx):
    if mx < 1:
        return 0, low, high
    r, nlo, nhi = adv(low, high)
    lo32 = r & 0xFFFFFFFF
    ret = (lo32 & (mx - 1)) if (mx & (mx - 1)) == 0 else (lo32 % mx)
    return ret, nlo, nhi


def ref_inrange(low, high, nmin, nmax):
    if nmax <= nmin:
        return nmin, low, high
    rng_m1 = nmax - nmin
    rng = rng_m1 + 1
    if rng < 1:
        return nmin, low, high
    r, nlo, nhi = adv(low, high)
    lo32 = r & 0xFFFFFFFF
    ret = (lo32 & rng_m1) if (rng & rng_m1) == 0 else (lo32 % rng)
    return ret + nmin, nlo, nhi


def gate(name, thunk):
    """run one call; return (result, None) or (None, reason)"""
    try:
        return thunk(), None
    except Exception as e:
        return None, f"faulted ({e})"


def main():
    target = sys.argv[1] if len(sys.argv) > 1 else "Game.exe"
    try:
        target = int(target)
    except ValueError:
        pass
    session = frida.attach(target)
    with open(os.path.join(HERE, "pd2_frida_rng_family.js"), encoding="utf-8") as f:
        script = session.create_script(f.read())
    script.load()
    api = script.exports_sync

    base = None
    for _ in range(20):
        base = api.ready()
        if base:
            break
        print("  D2Common not loaded yet..."); time.sleep(3)
    if not base:
        print("  !! D2Common never loaded"); return
    print("D2Common base", base)

    # per-function safety gate
    checks = [
        ("SEED_GetRandomNumber", lambda: api.grn(3, 7, 1000), ref_limited(3, 7, 1000)),
        ("SEED_GetRandomNumberAlt", lambda: api.alt(3, 7, 1000), ref_limited(3, 7, 1000)),
        ("SEED_GetRandomInRange", lambda: api.inr(3, 7, -3, 3), ref_inrange(3, 7, -3, 3)),
    ]
    for name, thunk, (er, elo, ehi) in checks:
        g, err = gate(name, thunk)
        if err:
            print(f"  !! {name} first call {err} -- aborting."); return
        if not (g["ret"] == er and g["newLow"] == elo and g["newHigh"] == ehi):
            print(f"  !! {name} first-call MISMATCH got {g} exp ret={er} -- aborting."); return
        print(f"  gate OK  {name}")

    # capture
    out, all_ok, total = {}, True, 0
    for name, roller, vectors, ref in [
        ("SEED_GetRandomNumber", api.grn, LIMITED, ref_limited),
        ("SEED_GetRandomNumberAlt", api.alt, LIMITED, ref_limited),
    ]:
        fn_out = {}
        for v in vectors:
            low, high, mx = v
            seq, L, H = [], low, high
            for _ in range(ROLLS):
                g = roller(L, H, mx)
                er, elo, ehi = ref(L, H, mx)
                ok = (g["ret"] == er and g["newLow"] == elo and g["newHigh"] == ehi)
                all_ok = all_ok and ok; total += 1
                seq.append({"in": [L, H, mx], "ret": g["ret"], "new": [g["newLow"], g["newHigh"]], "ok": ok})
                L, H = g["newLow"], g["newHigh"]
            fn_out[f"{low:#x},{high:#x},max={mx}"] = seq
        out[name] = fn_out

    inr_out = {}
    for (low, high, nmin, nmax) in INRANGE:
        seq, L, H = [], low, high
        for _ in range(ROLLS):
            g = api.inr(L, H, nmin, nmax)
            er, elo, ehi = ref_inrange(L, H, nmin, nmax)
            ok = (g["ret"] == er and g["newLow"] == elo and g["newHigh"] == ehi)
            all_ok = all_ok and ok; total += 1
            seq.append({"in": [L, H, nmin, nmax], "ret": g["ret"], "new": [g["newLow"], g["newHigh"]], "ok": ok})
            L, H = g["newLow"], g["newHigh"]
        inr_out[f"{low:#x},{high:#x},[{nmin},{nmax}]"] = seq
    out["SEED_GetRandomInRange"] = inr_out

    with open(os.path.join(HERE, "pd2_rng_family.json"), "w", encoding="utf-8") as f:
        json.dump(out, f, indent=0)
    session.detach()
    print(f"\n  {total} calls across 3 SEED_* functions (EAX / ESI / EAX+EDI ABIs); "
          + ("ALL BIT-EXACT vs D2MOO LCG" if all_ok else "DIVERGENCE!"))
    print("  wrote pd2_rng_family.json")


if __name__ == "__main__":
    main()
