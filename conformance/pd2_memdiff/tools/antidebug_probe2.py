import ctypes, ctypes.wintypes as wt, sys, struct
import pefile
k=ctypes.WinDLL("kernel32",use_last_error=True)
# find pid
class PE32(ctypes.Structure):
    _fields_=[("dwSize",wt.DWORD),("cntUsage",wt.DWORD),("th32ProcessID",wt.DWORD),
      ("th32DefaultHeapID",ctypes.POINTER(wt.ULONG)),("th32ModuleID",wt.DWORD),
      ("cntThreads",wt.DWORD),("th32ParentProcessID",wt.DWORD),("pcPriClassBase",wt.LONG),
      ("dwFlags",wt.DWORD),("szExeFile",ctypes.c_char*260)]
snap=k.CreateToolhelp32Snapshot(0x2,0); pe=PE32(); pe.dwSize=ctypes.sizeof(PE32); pid=None
ok=k.Process32First(snap,ctypes.byref(pe))
while ok:
    if pe.szExeFile.decode(errors='replace').lower()=="game.exe": pid=pe.th32ProcessID; break
    ok=k.Process32Next(snap,ctypes.byref(pe))
k.CloseHandle(snap)
h=k.OpenProcess(0x410,False,pid)
def rpm(a,n):
    b=(ctypes.c_char*n)(); g=ctypes.c_size_t(0)
    return b.raw if k.ReadProcessMemory(h,ctypes.c_void_p(a),b,n,ctypes.byref(g)) and g.value==n else None
# modules
class ME(ctypes.Structure):
    _fields_=[("dwSize",wt.DWORD),("m",wt.DWORD),("p",wt.DWORD),("g",wt.DWORD),("pr",wt.DWORD),
      ("b",ctypes.POINTER(ctypes.c_byte)),("sz",wt.DWORD),("hM",wt.HMODULE),
      ("name",ctypes.c_char*256),("path",ctypes.c_char*260)]
snap=k.CreateToolhelp32Snapshot(0x8|0x10,pid); me=ME(); me.dwSize=ctypes.sizeof(ME); B={}
ok=k.Module32First(snap,ctypes.byref(me))
while ok:
    B[me.name.decode(errors='replace').lower()]=(ctypes.cast(me.b,ctypes.c_void_p).value,me.path.decode(errors='replace'))
    ok=k.Module32Next(snap,ctypes.byref(me))
k.CloseHandle(snap)
def modof(a):
    for n,(base,_) in B.items():
        # need size; approximate: read via toolhelp again omitted, use name ranges from B with sizes
        pass
    return None
# build ranges
snap=k.CreateToolhelp32Snapshot(0x8|0x10,pid); me=ME(); me.dwSize=ctypes.sizeof(ME); R=[]
ok=k.Module32First(snap,ctypes.byref(me))
while ok:
    b=ctypes.cast(me.b,ctypes.c_void_p).value; R.append((b,b+me.sz,me.name.decode(errors='replace')))
    ok=k.Module32Next(snap,ctypes.byref(me))
k.CloseHandle(snap)
R.sort()
import bisect
starts=[x[0] for x in R]
def whose(a):
    i=bisect.bisect_right(starts,a)-1
    if 0<=i<len(R) and R[i][0]<=a<R[i][1]: return R[i][2]
    return "?"
def expaddr(dll,fn):
    base,path=B[dll]; p=pefile.PE(path,fast_load=True)
    p.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_EXPORT']])
    for e in p.DIRECTORY_ENTRY_EXPORT.symbols:
        if e.name and e.name.decode(errors='replace')==fn: return base+e.address, path, e.address
    return None,path,None
def clean_bytes(path,rva,n):
    p=pefile.PE(path,fast_load=True); d=open(path,'rb').read()
    for s in p.sections:
        if s.VirtualAddress<=rva<s.VirtualAddress+max(s.Misc_VirtualSize,s.SizeOfRawData):
            o=s.PointerToRawData+(rva-s.VirtualAddress); return d[o:o+n]
print(f"# pid={pid}")
# Follow kernel32 forwarder stubs to final impl and check impl prologue
for dll,fn in [("kernel32.dll","IsDebuggerPresent"),("kernel32.dll","OutputDebugStringA"),
               ("kernel32.dll","CheckRemoteDebuggerPresent"),("kernelbase.dll","IsDebuggerPresent")]:
    if dll not in B: print(f"  {dll} not loaded"); continue
    va,path,rva=expaddr(dll,fn)
    if not va: print(f"  {dll}!{fn} not found"); continue
    stub=rpm(va,6)
    line=f"  {dll}!{fn} @0x{va:X} stub={stub.hex()}"
    if stub and stub[0]==0xFF and stub[1]==0x25:
        ptr=struct.unpack('<I',stub[2:6])[0]
        dest=rpm(ptr,4)
        if dest:
            d=struct.unpack('<I',dest)[0]
            line+=f" -> jmp[{ptr:#x}] -> 0x{d:08X} in {whose(d)}"
    else:
        # inline impl: compare to clean
        cb=clean_bytes(path,rva,6)
        line+=f" clean={cb.hex() if cb else '?'} {'HOOKED' if cb and stub and cb[:5]!=stub[:5] else 'clean-impl'}"
    print(line)
k.CloseHandle(h)
print("# done")
