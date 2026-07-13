import ctypes, ctypes.wintypes as wt, sys, struct, os
import pefile

# find Game.exe pid
import subprocess
pid=None
snapclass=0x2
class PE32(ctypes.Structure):
    _fields_=[("dwSize",wt.DWORD),("cntUsage",wt.DWORD),("th32ProcessID",wt.DWORD),
      ("th32DefaultHeapID",ctypes.POINTER(wt.ULONG)),("th32ModuleID",wt.DWORD),
      ("cntThreads",wt.DWORD),("th32ParentProcessID",wt.DWORD),("pcPriClassBase",wt.LONG),
      ("dwFlags",wt.DWORD),("szExeFile",ctypes.c_char*260)]
k=ctypes.WinDLL("kernel32",use_last_error=True)
snap=k.CreateToolhelp32Snapshot(snapclass,0)
pe=PE32(); pe.dwSize=ctypes.sizeof(PE32)
ok=k.Process32First(snap,ctypes.byref(pe))
while ok:
    if pe.szExeFile.decode(errors='replace').lower()=="game.exe": pid=pe.th32ProcessID; break
    ok=k.Process32Next(snap,ctypes.byref(pe))
k.CloseHandle(snap)
if not pid: print("Game.exe not running"); sys.exit(1)
print(f"# Game.exe pid={pid}")

PROCESS_QUERY_INFORMATION=0x400; PROCESS_VM_READ=0x10
h=k.OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,False,pid)
if not h: raise ctypes.WinError(ctypes.get_last_error())
def rpm(addr,n):
    buf=(ctypes.c_char*n)(); got=ctypes.c_size_t(0)
    if k.ReadProcessMemory(h,ctypes.c_void_p(addr),buf,n,ctypes.byref(got)) and got.value==n:
        return buf.raw
    return None

ntdll=ctypes.WinDLL("ntdll")
# --- PEB32 via ProcessWow64Information (class 26) ---
NtQIP=ntdll.NtQueryInformationProcess
peb32=ctypes.c_uint64(0)
st=NtQIP(h,26,ctypes.byref(peb32),8,None)
peb=peb32.value
print(f"# ProcessWow64Information -> PEB32=0x{peb:X} (status=0x{st & 0xffffffff:X})")
if peb:
    bd=rpm(peb+0x2,1); ngf=rpm(peb+0x68,4)
    print(f"# PEB.BeingDebugged      = {bd.hex() if bd else '??'}  (00=no debugger flag set)")
    print(f"# PEB.NtGlobalFlag(+0x68)= {struct.unpack('<I',ngf)[0]:#010x} if ngf else '??'  (0x70 bits => heap debug flags)"
          if ngf else "# NtGlobalFlag unreadable")

# --- module bases in target (32-bit ntdll/kernel32 from SysWOW64) ---
TH32CS_SNAPMODULE=0x8; TH32CS_SNAPMODULE32=0x10
class ME(ctypes.Structure):
    _fields_=[("dwSize",wt.DWORD),("th32ModuleID",wt.DWORD),("th32ProcessID",wt.DWORD),
      ("GlblcntUsage",wt.DWORD),("ProccntUsage",wt.DWORD),("modBaseAddr",ctypes.POINTER(ctypes.c_byte)),
      ("modBaseSize",wt.DWORD),("hModule",wt.HMODULE),("szModule",ctypes.c_char*256),("szExePath",ctypes.c_char*260)]
snap=k.CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid)
me=ME(); me.dwSize=ctypes.sizeof(ME); bases={}
ok=k.Module32First(snap,ctypes.byref(me))
while ok:
    bases[me.szModule.decode(errors='replace').lower()]=(ctypes.cast(me.modBaseAddr,ctypes.c_void_p).value, me.szExePath.decode(errors='replace'))
    ok=k.Module32Next(snap,ctypes.byref(me))
k.CloseHandle(snap)

# --- hook check: compare in-process prologue vs clean on-disk SysWOW64 bytes ---
APIS={
 "ntdll.dll":["NtQueryInformationProcess","NtSetInformationThread","NtQueryObject","NtClose","DbgUiRemoteBreakin","DbgBreakPoint","NtQuerySystemInformation"],
 "kernel32.dll":["IsDebuggerPresent","CheckRemoteDebuggerPresent","OutputDebugStringA"],
}
print("\n# API prologue hook check (in-process vs clean on-disk SysWOW64):")
for dll,fns in APIS.items():
    if dll not in bases: print(f"  {dll}: not loaded"); continue
    base,path=bases[dll]
    try:
        p=pefile.PE(path,fast_load=True); p.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_EXPORT']])
    except Exception as e:
        print(f"  {dll}: cant parse {path}: {e}"); continue
    exp={}
    filedata=open(path,'rb').read()
    def rva2off(rva):
        for s in p.sections:
            if s.VirtualAddress<=rva<s.VirtualAddress+max(s.Misc_VirtualSize,s.SizeOfRawData): return s.PointerToRawData+(rva-s.VirtualAddress)
    for e in p.DIRECTORY_ENTRY_EXPORT.symbols:
        if e.name: exp[e.name.decode(errors='replace')]=e.address
    for fn in fns:
        if fn not in exp: print(f"  {dll}!{fn}: not exported"); continue
        rva=exp[fn]; live=rpm(base+rva,16)
        clean=filedata[rva2off(rva):rva2off(rva)+16]
        if live is None: print(f"  {dll}!{fn}: unreadable"); continue
        status="HOOKED" if live[:5]!=clean[:5] else "clean"
        extra=""
        if status=="HOOKED": extra=f"  live={live[:6].hex()} clean={clean[:6].hex()}"
        print(f"  {dll}!{fn:28} {status}{extra}")

# --- is ProjectDiablo IsDebuggerPresent IAT slot pointing at real kernel32 or a hook? ---
if "projectdiablo.dll" in bases:
    pdbase,_=bases["projectdiablo.dll"]
    slot=pdbase+0x27008C  # from static analysis (rva of IsDebuggerPresent IAT)
    v=rpm(slot,4)
    if v:
        tgt=struct.unpack('<I',v)[0]
        kbase=bases.get("kernel32.dll",(0,))[0]
        ink32 = kbase<=tgt<kbase+0x200000
        print(f"\n# ProjectDiablo IsDebuggerPresent IAT -> 0x{tgt:08X} ({'kernel32' if ink32 else 'ELSEWHERE(hook?)'})")
k.CloseHandle(h)
print("# done")
