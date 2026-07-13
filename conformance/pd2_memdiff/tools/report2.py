import json, csv
from collections import defaultdict, Counter
SC=r"C:\Users\benam\AppData\Local\Temp\claude\c--Users-benam-source-cpp-D2MOO\4663b38e-1006-4771-b1b8-d7812923eb3c\scratchpad"
REPO=r"C:\Users\benam\source\cpp\D2MOO"
rows=json.load(open(SC+r"\final_rows.json"))
db=json.load(open(REPO+r"\conformance\profiler\hit_profile_2026-07-08.json"))["functions"]
byord={f["ordinal"]:f for f in db}

# classify patcher: PD2 (ProjectDiablo + logic/nop assumed gameplay) vs harness/mods
def bucket(mod):
    if mod=="ProjectDiablo.dll": return "PD2 (ProjectDiablo.dll)"
    if mod in ("(logic/nop/short)","(non-branch/other)"): return "PD2 inline logic/data edit"
    if mod=="D2Common.dll": return "D2MOO reimpl detour"
    if mod=="SGD2FreeRes.dll": return "SGD2FreeRes (HD mod)"
    if mod=="D2Debugger.dll": return "D2Debugger (oracle)"
    return mod

funcs=defaultdict(lambda:{"sites":0,"bytes":0,"types":Counter(),"buckets":Counter(),"targets":set()})
for r in rows:
    o=r["export_ord"]
    key=("ord",o) if o else ("int",r.get("export_rva"))
    f=funcs[key]
    f["sites"]+=1; f["bytes"]+=r["len"]
    f["types"][r["type"]]+=1
    f["buckets"][bucket(r["real_mod"])]+=1
    if r.get("real_target"): f["targets"].add(r["real_target"])

out=[]
for key,f in funcs.items():
    if key[0]=="ord":
        o=key[1]; rec=byord.get(o,{})
        name=rec.get("name","(unnamed)"); sub=rec.get("subsystem","?"); ordn=o
    else:
        name="(internal, non-exported)"; sub="-"; ordn=""
    prim=f["buckets"].most_common(1)[0][0]
    out.append({"ordinal":ordn,"subsystem":sub,"name":name,"sites":f["sites"],
                "bytes":f["bytes"],
                "patch_types":"; ".join(f"{k}:{v}" for k,v in f["types"].most_common()),
                "attributed_to":"; ".join(f"{k}:{v}" for k,v in f["buckets"].most_common()),
                "primary":prim})

order=["PD2 (ProjectDiablo.dll)","PD2 inline logic/data edit","D2MOO reimpl detour",
       "SGD2FreeRes (HD mod)","D2Debugger (oracle)"]
out.sort(key=lambda r:(order.index(r["primary"]) if r["primary"] in order else 9, r["subsystem"], str(r["name"])))

# summary
print("===== affected functions grouped by who patched them =====")
bc=Counter(r["primary"] for r in out)
for b in order:
    if bc[b]: print(f"  {b:32s} {bc[b]} functions")
for b,c in bc.items():
    if b not in order: print(f"  {b:32s} {c} functions")
print(f"  TOTAL: {len(out)} functions, {sum(r['sites'] for r in out)} patch sites, {sum(r['bytes'] for r in out)} bytes")

# write CSV
csvpath=r"C:\tmp\pd2_d2common_memdiff.csv"
with open(csvpath,"w",newline="") as fh:
    w=csv.DictWriter(fh,fieldnames=["primary","ordinal","subsystem","name","sites","bytes","patch_types","attributed_to"])
    w.writeheader()
    for r in out: w.writerow(r)
print(f"\n# CSV -> {csvpath}")
json.dump(out, open(SC+r"\report_final.json","w"), indent=1)

# print PD2-only concise list
print("\n===== FUNCTIONS PD2 ITSELF MODIFIES (ProjectDiablo.dll redirects + inline edits) =====")
for r in out:
    if r["primary"].startswith("PD2"):
        print(f"  #{r['ordinal']:<6} {r['subsystem']:10} {r['name'][:44]:44} {r['sites']}site/{r['bytes']}B  [{r['patch_types']}]")
