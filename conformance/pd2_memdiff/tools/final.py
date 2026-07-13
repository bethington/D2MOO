import json
from collections import defaultdict, Counter
SC=r"C:\Users\benam\AppData\Local\Temp\claude\c--Users-benam-source-cpp-D2MOO\4663b38e-1006-4771-b1b8-d7812923eb3c\scratchpad"
REPO=r"C:\Users\benam\source\cpp\D2MOO"
rows=json.load(open(SC+r"\text_patches_attr.json"))
db=json.load(open(REPO+r"\conformance\profiler\hit_profile_2026-07-08.json"))["functions"]
byord={f["ordinal"]:f for f in db}

# per-function aggregation
funcs=defaultdict(lambda:{"sites":0,"types":Counter(),"patchers":Counter(),"len":0})
for r in rows:
    o=r["export_ord"]
    key=o if o else "internal@0x%05X"%r["export_rva"] if r.get("export_rva") else "internal"
    f=funcs[key]
    f["sites"]+=1
    f["types"][r["type"]]+=1
    f["patchers"][r["patcher"]]+=1
    f["len"]+=r["len"]

def nm(o):
    if isinstance(o,int) and o in byord:
        return byord[o]["name"], byord[o]["subsystem"]
    return "(unnamed)","?"

# classify each function's dominant nature
rowsout=[]
for key,f in funcs.items():
    if isinstance(key,int):
        name,sub=nm(key); ordn=key
    else:
        name,sub=("(internal, non-exported)","-"); ordn=key
    patchers=", ".join(f"{k}×{v}" for k,v in f["patchers"].most_common())
    types=", ".join(f"{k}×{v}" for k,v in f["types"].most_common())
    rowsout.append({"ord":ordn,"name":name,"sub":sub,"sites":f["sites"],
                    "bytes":f["len"],"types":types,"patchers":patchers,
                    "primary_patcher":f["patchers"].most_common(1)[0][0]})

# order: by patcher class then subsystem then name
def sortkey(r):
    return (r["primary_patcher"], r["sub"], str(r["name"]))
rowsout.sort(key=sortkey)

# Summary by primary patcher
print("===== SUMMARY: functions affected, by primary patcher =====")
pc=Counter(r["primary_patcher"] for r in rowsout)
for p,c in pc.most_common():
    print(f"  {p:32s} {c} functions")
print(f"  TOTAL affected exported+internal functions: {len(rowsout)}")
print()

# print full table
print("===== FULL LIST =====")
hdr=f"{'ord':>6} {'subsystem':11} {'name':45} {'sites':>5} {'bytes':>5}  patcher(s) / patch-types"
print(hdr); print("-"*len(hdr))
for r in rowsout:
    print(f"{str(r['ord']):>6} {r['sub'][:11]:11} {str(r['name'])[:45]:45} {r['sites']:>5} {r['bytes']:>5}  {r['patchers']}  [{r['types']}]")

json.dump(rowsout, open(SC+r"\final_report.json","w"), indent=1)
print("\n# saved final_report.json")
