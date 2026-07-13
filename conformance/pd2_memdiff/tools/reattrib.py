import json, struct, bisect, csv, os
from collections import Counter, defaultdict
OUT=r"C:\tmp\pd2diff"
LABEL="rig"
d=json.load(open(os.path.join(OUT,f"diff_{LABEL}.json")))
mods=sorted(d["modules"],key=lambda m:m["base"])
starts=[m["base"] for m in mods]
def modof(a):
    if a is None: return None
    i=bisect.bisect_right(starts,a)-1
    if 0<=i<len(mods) and mods[i]["base"]<=a<mods[i]["base"]+mods[i]["size"]:
        return mods[i]
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

base_by_dll={s["dll"]:int(s["base"],16) for s in d["summary"] if "base" in s}

def attribute(dll, r):
    base=base_by_dll[dll]
    t=r["type"]
    if t in ("JMP-detour","CALL-patch"):
        return eco(modof(int(r["target"],16))) if r["target"] else "unresolved", "control-redirect"
    if t in ("NOP-out","partial-NOP"):
        return "logic-nop", "code-removed"
    # inline-edit: try abs then rel
    mb=bytes.fromhex(r["mem"])
    if len(mb)>=4:
        absv=struct.unpack_from("<I",mb,0)[0]
        rel=struct.unpack_from("<i",mb,0)[0]
        relt=(base+r["rva"]+4+rel)&0xFFFFFFFF
        ma=modof(absv); mr=modof(relt)
        # prefer a PD2-eco/harness hit
        for cand,kind in [(ma,"abs-ptr"),(mr,"rel-branch")]:
            e=eco(cand)
            if e and (e.startswith("PD2:") or e.startswith("HARNESS:")):
                return e, kind
        # else any module
        for cand,kind in [(ma,"abs-ptr"),(mr,"rel-branch")]:
            if cand: return eco(cand), kind
    return "logic-inline", "value-edit"

# per-DLL rollup
rows=[]
perdll=[]
for s in d["summary"]:
    dll=s["dll"]
    if "base" not in s: continue
    text=d["detail"].get(dll,[])
    hist=Counter(); funcs=defaultdict(lambda:{"sites":0,"bytes":0,"attr":Counter(),"types":Counter()})
    for r in text:
        who,kind=attribute(dll,r)
        r["who"]=who; r["kind"]=kind
        hist[who]+=1
        key=r["ord"] if r["ord"] else ("int",r["exp_rva"])
        f=funcs[key]; f["sites"]+=1; f["bytes"]+=r["len"]; f["attr"][who]+=1; f["types"][r["type"]]+=1
        rows.append({"dll":dll,"rva":f"0x{r['rva']:05X}","ord":r["ord"] or "",
                     "type":r["type"],"who":who,"kind":kind,"target":r.get("target") or "",
                     "len":r["len"]})
    # collapse into pd2/harness/logic
    pd2=sum(v for k,v in hist.items() if k.startswith("PD2:"))
    harness=sum(v for k,v in hist.items() if k.startswith("HARNESS:"))
    logic=sum(v for k,v in hist.items() if k in ("logic-nop","logic-inline"))
    other=len(text)-pd2-harness-logic
    perdll.append({"dll":dll,"sites":len(text),"funcs":len(funcs),
                   "PD2":pd2,"harness":harness,"logic":logic,"other":other,
                   "breakdown":dict(hist.most_common())})

perdll.sort(key=lambda x:-x["sites"])
print(f"{'DLL':16}{'sites':>6}{'PD2':>6}{'harness':>8}{'logic':>6}{'other':>6}   detail")
tot=Counter()
for p in perdll:
    for k,v in [("PD2",p["PD2"]),("harness",p["harness"]),("logic",p["logic"]),("other",p["other"]),("sites",p["sites"])]:
        tot[k]+=v
    det="; ".join(f"{k}:{v}" for k,v in p["breakdown"].items() if not k.startswith("logic"))
    print(f"{p['dll']:16}{p['sites']:>6}{p['PD2']:>6}{p['harness']:>8}{p['logic']:>6}{p['other']:>6}   {det}")
print(f"\n{'TOTAL':16}{tot['sites']:>6}{tot['PD2']:>6}{tot['harness']:>8}{tot['logic']:>6}{tot['other']:>6}")

# write master CSV of every patch site
with open(os.path.join(OUT,"all_patch_sites.csv"),"w",newline="") as fh:
    w=csv.DictWriter(fh,fieldnames=["dll","rva","ord","type","who","kind","target","len"])
    w.writeheader(); [w.writerow(r) for r in rows]
json.dump(perdll, open(os.path.join(OUT,"perdll_rollup.json"),"w"), indent=1)
print(f"\n# CSV -> {OUT}\\all_patch_sites.csv  ({len(rows)} sites)")
