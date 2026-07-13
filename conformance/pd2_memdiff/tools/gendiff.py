import ctypes, ctypes.wintypes as wt, sys, struct, json, bisect, os
import pefile

PID = int(sys.argv[1])
LABEL = sys.argv[2] if len(sys.argv)>2 else "run"
OUTDIR = r"C:\tmp\pd2diff"
os.makedirs(OUTDIR, exist_ok=True)
GAMEDIR = r"c:\diablo2\projectd2"

CLASSIC = {n.lower() for n in [
 "Game.exe","Diablo II.exe","D2Common.dll","D2Game.dll","D2Client.dll","D2Net.dll",
 "D2gfx.dll","D2Win.dll","D2Lang.dll","D2CMP.dll","D2sound.dll","D2Multi.dll",
 "D2MCPClient.dll","D2Launch.dll","D2Glide.dll","D2Gdi.dll","D2Direct3D.dll",
 "D2DDraw.dll","Storm.dll","Fog.dll","Bnclient.dll"]}

TH32CS_SNAPMODULE=0x8; TH32CS_SNAPMODULE32=0x10
class ME(ctypes.Structure):
    _fields_=[("dwSize",wt.DWORD),("th32ModuleID",wt.DWORD),("th32ProcessID",wt.DWORD),
      ("GlblcntUsage",wt.DWORD),("ProccntUsage",wt.DWORD),("modBaseAddr",ctypes.POINTER(ctypes.c_byte)),
      ("modBaseSize",wt.DWORD),("hModule",wt.HMODULE),("szModule",ctypes.c_char*256),("szExePath",ctypes.c_char*260)]
k=ctypes.WinDLL("kernel32",use_last_error=True)

def modules():
    s=k.CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,PID)
    if s==-1: raise ctypes.WinError(ctypes.get_last_error())
    me=ME(); me.dwSize=ctypes.sizeof(ME); out=[]
    ok=k.Module32First(s,ctypes.byref(me))
    while ok:
        b=ctypes.cast(me.modBaseAddr,ctypes.c_void_p).value
        out.append({"base":b,"size":me.modBaseSize,"name":me.szModule.decode(errors='replace'),
                    "path":me.szExePath.decode(errors='replace')})
        ok=k.Module32Next(s,ctypes.byref(me))
    k.CloseHandle(s); return out

def read_mem(h, base, size):
    PAGE=0x1000; out=bytearray(size)
    for off in range(0,size,PAGE):
        n=min(PAGE,size-off); buf=(ctypes.c_char*n)(); pn=ctypes.c_size_t(0)
        if k.ReadProcessMemory(h,ctypes.c_void_p(base+off),buf,n,ctypes.byref(pn)) and pn.value==n:
            out[off:off+n]=buf.raw
    return bytes(out)

mods=modules()
# attribution ranges (all modules), label name@base
mods_sorted=sorted(mods,key=lambda m:m["base"])
astarts=[m["base"] for m in mods_sorted]
def attrib(addr):
    i=bisect.bisect_right(astarts,addr)-1
    if 0<=i<len(mods_sorted):
        m=mods_sorted[i]
        if m["base"]<=addr<m["base"]+m["size"]:
            return f'{m["name"]}@{m["base"]:X}'
    return None

# target DLLs = classic set located in the game dir
targets=[m for m in mods if m["name"].lower() in CLASSIC and m["path"].lower().startswith(GAMEDIR)]
# de-dup by base
seen=set(); T=[]
for m in targets:
    if m["base"] in seen: continue
    seen.add(m["base"]); T.append(m)

h=k.OpenProcess(0x10|0x400,False,PID)
if not h: raise ctypes.WinError(ctypes.get_last_error())

def classify(disk,mem):
    db=bytes.fromhex(disk); mb=bytes.fromhex(mem)
    if mb[:1]==b"\xe9" and len(mb)>=5: return "JMP-detour", struct.unpack("<i",mb[1:5])[0], 5
    if mb[:1]==b"\xe8" and len(mb)>=5: return "CALL-patch", struct.unpack("<i",mb[1:5])[0], 5
    if all(x==0x90 for x in mb): return "NOP-out", None, None
    if mb.count(0x90)>len(mb)//2: return "partial-NOP", None, None
    return "inline-edit", None, None

summary=[]
alldetail={}
for m in T:
    base=m["base"]; size=m["size"]; path=m["path"]; name=m["name"]
    try:
        pe=pefile.PE(path, fast_load=False)
    except Exception as e:
        summary.append({"dll":name,"error":str(e)}); continue
    ib=pe.OPTIONAL_HEADER.ImageBase; isz=pe.OPTIONAL_HEADER.SizeOfImage
    delta=base-ib
    mem=read_mem(h, base, min(size,isz))
    expected=bytearray(min(isz,size))
    hdr=pe.header; expected[:len(hdr)]=hdr[:len(expected)]
    for s in pe.sections:
        va=s.VirtualAddress; raw=s.get_data(); vs=s.Misc_VirtualSize
        n=min(len(raw),vs,len(expected)-va)
        if n>0: expected[va:va+n]=raw[:n]
    # apply relocations
    if delta!=0 and hasattr(pe,"DIRECTORY_ENTRY_BASERELOC"):
        for blk in pe.DIRECTORY_ENTRY_BASERELOC:
            for e in blk.entries:
                if e.type==0: continue
                rva=e.rva
                if rva+4<=len(expected):
                    orig=struct.unpack_from("<I",expected,rva)[0]
                    struct.pack_into("<I",expected,rva,(orig+delta)&0xFFFFFFFF)
    ignore=bytearray(len(expected))
    def mark(rva,length):
        for i in range(rva,min(rva+length,len(ignore))): ignore[i]=1
    dd=pe.OPTIONAL_HEADER.DATA_DIRECTORY
    if dd[12].VirtualAddress and dd[12].Size: mark(dd[12].VirtualAddress,dd[12].Size)
    if hasattr(pe,"DIRECTORY_ENTRY_IMPORT"):
        for imp in pe.DIRECTORY_ENTRY_IMPORT:
            for i,_ in enumerate(imp.imports): mark(imp.struct.FirstThunk+i*4,4)
    # sections map
    secs=[(s.VirtualAddress, s.VirtualAddress+max(s.Misc_VirtualSize,s.SizeOfRawData),
           s.Name.rstrip(b"\x00").decode(errors="replace")) for s in pe.sections]
    def sect(rva):
        for a,b,nm in secs:
            if a<=rva<b: return nm
        return "?"
    # exports
    exps=[]
    if hasattr(pe,"DIRECTORY_ENTRY_EXPORT"):
        for e in pe.DIRECTORY_ENTRY_EXPORT.symbols:
            if e.address: exps.append((e.address,e.ordinal,(e.name.decode() if e.name else None)))
    exps.sort(); exrvas=[e[0] for e in exps]
    def own(rva):
        i=bisect.bisect_right(exrvas,rva)-1
        return exps[i] if i>=0 else None
    # diff
    n=min(len(expected),len(mem)); runs=[]; rs=None; last=None
    i=0
    while i<n:
        if not ignore[i] and expected[i]!=mem[i]:
            if rs is None: rs=i
            last=i
        else:
            if rs is not None: runs.append((rs,last+1)); rs=None
        i+=1
    if rs is not None: runs.append((rs,last+1))
    merged=[]
    for r in runs:
        if merged and r[0]-merged[-1][1]<=8: merged[-1]=(merged[-1][0],r[1])
        else: merged.append(list(r))
    text=[]; data_runs=0; data_bytes=0
    for a,b in merged:
        sc=sect(a)
        if sc==".text" or sc.lower() in ("text","code"):
            db=bytes(expected[a:b]).hex(); mb=bytes(mem[a:b]).hex()
            t,rel,ilen=classify(db,mb)
            if rel is not None:
                tgt=(base+a+ilen+rel)&0xFFFFFFFF; tmod=attrib(tgt)
            elif t=="inline-edit" and (b-a)>=4:
                rel4=struct.unpack_from("<i",bytes.fromhex(mb),0)[0]
                tgt=(base+a+4+rel4)&0xFFFFFFFF; tmod=attrib(tgt)
                if tmod is None: tgt=None
            else:
                tgt=None; tmod=None
            ex=own(a)
            text.append({"rva":a,"len":b-a,"type":t,
                         "target":(f"0x{tgt:08X}" if tgt else None),"tmod":tmod,
                         "ord":(ex[1] if ex else None),"exp_rva":(ex[0] if ex else None),
                         "disk":db[:24],"mem":mb[:24]})
        else:
            data_runs+=1; data_bytes+=(b-a)
    alldetail[name]=text
    # per-dll attribution histogram + functions
    from collections import Counter,defaultdict
    funcset=defaultdict(lambda:{"sites":0,"bytes":0,"types":Counter(),"mods":Counter()})
    for r in text:
        key=r["ord"] if r["ord"] else ("int",r["exp_rva"])
        f=funcset[key]; f["sites"]+=1; f["bytes"]+=r["len"]; f["types"][r["type"]]+=1
        f["mods"][r["tmod"] or "(logic/nop/unresolved)"]+=1
    modhist=Counter()
    for r in text: modhist[r["tmod"] or "(logic/nop/unresolved)"]+=1
    summary.append({"dll":name,"base":f"0x{base:08X}","disk_base":f"0x{ib:08X}",
        "relocated":delta!=0,"text_sites":len(text),
        "text_bytes":sum(r["len"] for r in text),"functions":len(funcset),
        "data_runs":data_runs,"data_bytes":data_bytes,
        "attribution":dict(modhist.most_common())})

k.CloseHandle(h)
json.dump({"pid":PID,"label":LABEL,"modules":mods,"summary":summary,"detail":alldetail},
          open(os.path.join(OUTDIR,f"diff_{LABEL}.json"),"w"), indent=1)

print(f"# ===== {LABEL} (PID {PID}) : per-DLL in-memory vs on-disk =====")
print(f"{'DLL':16} {'reloc':5} {'funcs':>5} {'txt_sites':>9} {'txt_bytes':>9} {'data_runs':>9}  attribution")
for s in sorted(summary,key=lambda x:-x.get('text_sites',0)):
    if "error" in s: print(f"{s['dll']:16} ERROR {s['error']}"); continue
    att="; ".join(f"{kk}:{vv}" for kk,vv in s["attribution"].items())
    print(f"{s['dll']:16} {str(s['relocated']):5} {s['functions']:>5} {s['text_sites']:>9} {s['text_bytes']:>9} {s['data_runs']:>9}  {att}")
print(f"\n# saved {OUTDIR}\\diff_{LABEL}.json")
