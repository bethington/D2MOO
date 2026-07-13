import json, pefile, struct

SC=r"C:\Users\benam\AppData\Local\Temp\claude\c--Users-benam-source-cpp-D2MOO\4663b38e-1006-4771-b1b8-d7812923eb3c\scratchpad"
DISK=r"C:\Diablo2\ProjectD2\D2Common.dll"
d=json.load(open(SC+r"\diffs.json"))
pe=pefile.PE(DISK, fast_load=False)
base=d["base"]; image_base=d["image_base"]

# Export table: build sorted list of (rva, ordinal, name)
exports=[]
if hasattr(pe,"DIRECTORY_ENTRY_EXPORT"):
    ordbase=pe.DIRECTORY_ENTRY_EXPORT.struct.Base
    for e in pe.DIRECTORY_ENTRY_EXPORT.symbols:
        if e.address:
            exports.append((e.address, e.ordinal, (e.name.decode() if e.name else None)))
exports.sort()
exp_rvas=[e[0] for e in exports]
import bisect
def owning_export(rva):
    i=bisect.bisect_right(exp_rvas, rva)-1
    if i<0: return None
    return exports[i]  # nearest preceding export

def classify(disk, mem):
    db=bytes.fromhex(disk); mb=bytes.fromhex(mem)
    if mb[:1]==b"\xe9" and len(mb)>=5:
        # JMP rel32 detour
        rel=struct.unpack("<i", mb[1:5])[0]
        return "JMP-detour", rel
    if mb[:1]==b"\xe8" and len(mb)>=5:
        rel=struct.unpack("<i", mb[1:5])[0]
        return "CALL-patch", rel
    if all(x==0x90 for x in mb):
        return "NOP-out", None
    if 0x90 in mb and len([x for x in mb if x==0x90])>len(mb)//2:
        return "partial-NOP", None
    return "inline-edit", None

textruns=[r for r in d["runs"] if r["section"]==".text"]
datar=[r for r in d["runs"] if r["section"]==".data"]
print(f"# .text runs: {len(textruns)}  .data runs: {len(datar)}")
print()

# Group text runs by owning export
from collections import defaultdict
groups=defaultdict(list)
for r in textruns:
    ex=owning_export(r["rva_start"])
    key=ex if ex else (None,None,None)
    groups[key].append(r)

def hook_target(rel, run_end_rva):
    # jmp/call target absolute VA in the running process
    src_next=base+run_end_rva  # approx; use start+5
    return None

print("# ==== .text code patches, grouped by owning export ====")
rows=[]
for ex in sorted(groups, key=lambda e:(e[0] if e[0] is not None else -1)):
    runs=groups[ex]
    exrva,ordn,name=ex
    off = runs[0]["rva_start"]-exrva if exrva is not None else None
    hdr=f"ord#{ordn} {name if name else '(noname)'} @rva=0x{exrva:05X}" if exrva is not None else "(below first export)"
    types=set()
    for r in runs:
        t,rel=classify(r["disk"],r["mem"])
        types.add(t)
    print(f"[{hdr}]  +{off if off is not None else '?'}  runs={len(runs)}  types={sorted(types)}")
    for r in runs:
        t,rel=classify(r["disk"],r["mem"])
        tgt=""
        if rel is not None:
            abs_t=(base + r["rva_start"] + 5 + rel) & 0xFFFFFFFF
            tgt=f" -> 0x{abs_t:08X}"
        print(f"    rva=0x{r['rva_start']:05X} len={r['len']:3d} {t}{tgt}  {r['disk'][:20]}->{r['mem'][:20]}")
        rows.append({"export_ord":ordn,"export_name":name,"export_rva":exrva,
                     "rva":r["rva_start"],"len":r["len"],"type":t,
                     "target":(f"0x{(base + r['rva_start'] + 5 + rel) & 0xFFFFFFFF:08X}" if rel is not None else None),
                     "disk":r["disk"],"mem":r["mem"]})

json.dump(rows, open(SC+r"\text_patches.json","w"), indent=1)
print()
print(f"# distinct exports touched: {len([k for k in groups if k[0] is not None])}")
print(f"# saved text_patches.json ({len(rows)} runs)")

# Summarize hook target modules (which injected module they point to)
from collections import Counter
tmods=Counter()
for r in rows:
    if r["target"]:
        hi=int(r["target"],16)>>24
        tmods[f"0x{hi:02X}xxxxxx"]+=1
print("# JMP/CALL target high-byte histogram:", dict(tmods))
