import json, bisect
SC=r"C:\Users\benam\AppData\Local\Temp\claude\c--Users-benam-source-cpp-D2MOO\4663b38e-1006-4771-b1b8-d7812923eb3c\scratchpad"
rows=json.load(open(SC+r"\text_patches.json"))
mods=json.load(open(SC+r"\mods.json"))
mods=[m for m in mods if m["name"].lower()!="game.exe" or True]
mstarts=sorted([(m["base"],m["base"]+m["size"],m["name"]) for m in mods])
starts=[m[0] for m in mstarts]
def mod_of(addr):
    i=bisect.bisect_right(starts,addr)-1
    if 0<=i<len(mstarts) and mstarts[i][0]<=addr<mstarts[i][1]:
        return mstarts[i][2]
    # unnamed region
    if 0xA0000000<=addr<0xB0000000: return "(PD2 alloc heap 0xA0..)"
    return f"(unmapped 0x{addr>>24:02X}..)"

from collections import Counter, defaultdict
by_mod=defaultdict(list)
for r in rows:
    if r["target"]:
        m=mod_of(int(r["target"],16))
    else:
        # operand rewrite: destination = the new 32-bit value (little-endian last 4 bytes of mem for len>=4)
        mb=bytes.fromhex(r["mem"])
        if len(mb)>=4 and r["type"]=="inline-edit":
            import struct
            # find a 4-byte window that looks like a pointer 0x10.. / 0xA0.. / 0x6F..
            val=None
            for off in range(0,len(mb)-3):
                v=struct.unpack_from("<I",mb,off)[0]
                if (v>>24) in (0x10,0xA0,0x6F,0x6C,0x03): val=v; break
            m=mod_of(val) if val else "(inline/other)"
        else:
            m="(nop/logic edit)"
    r["patcher"]=m
    by_mod[m].append(r)

print("# ===== patches grouped by patching module =====")
for m in sorted(by_mod, key=lambda k:-len(by_mod[k])):
    rs=by_mod[m]
    exords=sorted(set(r["export_ord"] for r in rs if r["export_ord"]))
    tc=Counter(r["type"] for r in rs)
    print(f"\n## {m}: {len(rs)} sites across {len(exords)} exports  types={dict(tc)}")
    print("   ordinals:", ", ".join(f"#{o}" for o in exords))

json.dump(rows, open(SC+r"\text_patches_attr.json","w"), indent=1)
# also dump the set of ordinals for naming
allords=sorted(set(r["export_ord"] for r in rows if r["export_ord"]))
print(f"\n# total distinct exported ordinals patched: {len(allords)}")
print("ORDS:"+",".join(str(o) for o in allords))
