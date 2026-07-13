import json, struct, bisect
SC=r"C:\Users\benam\AppData\Local\Temp\claude\c--Users-benam-source-cpp-D2MOO\4663b38e-1006-4771-b1b8-d7812923eb3c\scratchpad"
REPO=r"C:\Users\benam\source\cpp\D2MOO"
rows=json.load(open(SC+r"\text_patches_attr.json"))
mods=json.load(open(SC+r"\mods.json"))
db=json.load(open(REPO+r"\conformance\profiler\hit_profile_2026-07-08.json"))["functions"]
byord={f["ordinal"]:f for f in db}
BASE=0x6FD50000
mstarts=sorted([(m["base"],m["base"]+m["size"],m["name"]) for m in mods])
starts=[m[0] for m in mstarts]
def mod_of(a):
    i=bisect.bisect_right(starts,a)-1
    if 0<=i<len(mstarts) and mstarts[i][0]<=a<mstarts[i][1]: return mstarts[i][2]
    return None

def rel32_target(rva_start, nbytes, memhex):
    # interpret first 4 bytes at rva_start as a rel32 whose instruction ends at rva_start+4
    mb=bytes.fromhex(memhex)
    if len(mb)<4: return None
    rel=struct.unpack_from("<i",mb,0)[0]
    return (BASE + rva_start + 4 + rel) & 0xFFFFFFFF

from collections import Counter, defaultdict
recat=defaultdict(list)
for r in rows:
    t=r["type"]
    if t in ("JMP-detour","CALL-patch"):
        # already have correct target (E8/E9 + rel32, instr end = start+5)
        tgt=int(r["target"],16); m=mod_of(tgt) or "(other)"
        r["real_target"]=f"0x{tgt:08X}"; r["real_mod"]=m
    elif t=="inline-edit" and r["len"]>=4:
        tgt=rel32_target(r["rva"],r["len"],r["mem"])
        m=mod_of(tgt)
        # also compute disk-side target to see original
        dr=struct.unpack_from("<i",bytes.fromhex(r["disk"]),0)[0]
        disk_tgt=(BASE+r["rva"]+4+dr)&0xFFFFFFFF
        r["real_target"]=f"0x{tgt:08X}" if tgt else None
        r["disk_target"]=f"0x{disk_tgt:08X}"
        r["real_mod"]=m or "(non-branch/other)"
    else:
        r["real_mod"]="(logic/nop/short)"
        r["real_target"]=None
    recat[r["real_mod"]].append(r)

print("# ===== recomputed target module for every text patch site =====")
for m in sorted(recat,key=lambda k:-len(recat[k])):
    rs=recat[m]
    tc=Counter(x["type"] for x in rs)
    print(f"{m:28s} sites={len(rs):3d}  types={dict(tc)}")

# Show a few inline-edit examples with disk->real target to validate
print("\n# sample inline-edit redirections (disk_target -> new_target):")
n=0
for r in rows:
    if r["type"]=="inline-edit" and r.get("real_mod","").startswith("ProjectDiablo"):
        print(f"  ord#{r['export_ord']} rva=0x{r['rva']:05X} disk_tgt={r.get('disk_target')}({mod_of(int(r['disk_target'],16))}) -> {r['real_target']}(ProjectDiablo)")
        n+=1
        if n>=8: break

json.dump(rows, open(SC+r"\final_rows.json","w"), indent=1)
print("\n# saved final_rows.json")
