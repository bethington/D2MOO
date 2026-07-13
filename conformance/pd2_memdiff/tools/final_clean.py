import json, os, csv
from collections import Counter, defaultdict
OUT=r"C:\tmp\pd2diff"
d=json.load(open(os.path.join(OUT,"diff_rig.json")))
# re-load attributed rows via reattrib logic result saved? We recompute who from detail using same rules quickly.
import struct, bisect
mods=sorted(d["modules"],key=lambda m:m["base"]); starts=[m["base"] for m in mods]
def modof(a):
    if a is None: return None
    i=bisect.bisect_right(starts,a)-1
    if 0<=i<len(mods) and mods[i]["base"]<=a<mods[i]["base"]+mods[i]["size"]: return mods[i]
    return None
PD2ECO={"projectdiablo.dll","bh.dll","sgd2freeres.dll","sgd2freedisplayfix.dll","pd2_ext.dll"}
def eco(m):
    if not m: return None
    n=m["name"].lower()
    if n in PD2ECO: return "PD2:"+m["name"]
    if n=="d2debugger.dll": return "HARNESS:D2Debugger"
    if n=="d2common.dll" and m["base"]==0x6CAD0000: return "HARNESS:D2MOO-reimpl"
    if n=="d2.detours.dll": return "HARNESS:D2.Detours"
    return "other:"+m["name"]
base_by={s["dll"]:int(s["base"],16) for s in d["summary"] if "base" in s}
def who_of(dll,r):
    base=base_by[dll]; t=r["type"]
    if t in ("JMP-detour","CALL-patch"):
        return eco(modof(int(r["target"],16))) if r["target"] else "unresolved"
    if t in ("NOP-out","partial-NOP"): return "logic-nop"
    mb=bytes.fromhex(r["mem"])
    if len(mb)>=4:
        absv=struct.unpack_from("<I",mb,0)[0]; rel=struct.unpack_from("<i",mb,0)[0]
        relt=(base+r["rva"]+4+rel)&0xFFFFFFFF
        for cand in (modof(absv),modof(relt)):
            e=eco(cand)
            if e and (e.startswith("PD2:") or e.startswith("HARNESS:")): return e
        for cand in (modof(absv),modof(relt)):
            if cand: return eco(cand)
    return "logic-inline"

# ---- D2Common overlap analysis: harness detour ranges vs PD2 edit ranges ----
tc=d["detail"]["D2COMMON.dll"]
harness=[]; pd2=[]
for r in tc:
    w=who_of("D2COMMON.dll",r)
    r["who"]=w
    seg=(r["rva"], r["rva"]+r["len"])
    if w.startswith("HARNESS:"): harness.append(seg)
    elif w.startswith("PD2:") or w.startswith("logic"): pd2.append((seg,w,r))
overlaps=[]
for (a,b),w,r in pd2:
    for (ha,hb) in harness:
        if a<hb and ha<b:
            overlaps.append((r["rva"],w,(ha,hb)))
print("=== D2Common masking check (PD2 edit byte-range overlapping a D2MOO detour) ===")
print(f"  harness detour sites: {len(harness)}   PD2/logic edit sites: {len(pd2)}")
print(f"  OVERLAPS (potential masking): {len(overlaps)}")
for o in overlaps[:20]: print("   ",o)
print("  => If 0 overlaps: rig's PD2 D2Common edits are the COMPLETE clean-PD2 set (no masking).")

# ---- Clean-PD2 per-DLL table (harness stripped) ----
print("\n=== CLEAN PD2 in-memory edits per game binary (D2MOO/D2Debugger harness removed) ===")
print(f"{'DLL':16}{'PD2 sites':>10}{'funcs*':>8}   dominant patcher(s)")
rows_clean=[]
grand=0
for s in sorted(d["summary"],key=lambda x:-x.get('text_sites',0)):
    dll=s.get("dll");
    if "base" not in s: continue
    text=d["detail"].get(dll,[])
    hist=Counter(); funcs=set()
    for r in text:
        w=who_of(dll,r)
        if w.startswith("HARNESS:"): continue
        if w.startswith("other:"): continue
        hist[w]+=1
        funcs.add(r["ord"] if r["ord"] else ("int",r["exp_rva"]))
        rows_clean.append({"dll":dll,"rva":f"0x{r['rva']:05X}","ord":r["ord"] or "",
                           "type":r["type"],"patcher":w,"len":r["len"],"target":r.get("target") or ""})
    n=sum(hist.values()); grand+=n
    if n==0: continue
    dom="; ".join(f"{k.replace('PD2:','')}:{v}" for k,v in hist.most_common(4))
    print(f"{dll:16}{n:>10}{len(funcs):>8}   {dom}")
print(f"{'TOTAL':16}{grand:>10}")
print("  *funcs = distinct owning exports; under-counts DLLs with sparse export tables (D2Client/D2Game).")

with open(os.path.join(OUT,"clean_pd2_patch_sites.csv"),"w",newline="") as fh:
    w=csv.DictWriter(fh,fieldnames=["dll","rva","ord","type","patcher","len","target"])
    w.writeheader(); [w.writerow(r) for r in rows_clean]
print(f"\n# CSV -> {OUT}\\clean_pd2_patch_sites.csv ({len(rows_clean)} sites)")
