import ctypes, ctypes.wintypes as wt, json
PID=110012
TH32CS_SNAPMODULE=0x8; TH32CS_SNAPMODULE32=0x10
class ME(ctypes.Structure):
    _fields_=[("dwSize",wt.DWORD),("th32ModuleID",wt.DWORD),("th32ProcessID",wt.DWORD),
      ("GlblcntUsage",wt.DWORD),("ProccntUsage",wt.DWORD),("modBaseAddr",ctypes.POINTER(ctypes.c_byte)),
      ("modBaseSize",wt.DWORD),("hModule",wt.HMODULE),("szModule",ctypes.c_char*256),("szExePath",ctypes.c_char*260)]
k=ctypes.WinDLL("kernel32",use_last_error=True)
s=k.CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,PID)
me=ME(); me.dwSize=ctypes.sizeof(ME)
mods=[]
ok=k.Module32First(s,ctypes.byref(me))
while ok:
    b=ctypes.cast(me.modBaseAddr,ctypes.c_void_p).value
    mods.append((b,me.modBaseSize,me.szModule.decode(errors='replace'),me.szExePath.decode(errors='replace')))
    ok=k.Module32Next(s,ctypes.byref(me))
k.CloseHandle(s)
mods.sort()
print(f"# {len(mods)} modules")
for b,sz,n,p in mods:
    print(f"0x{b:08X}-0x{b+sz:08X} {n}")
json.dump([{"base":b,"size":sz,"name":n,"path":p} for b,sz,n,p in mods],
          open(r"C:\Users\benam\AppData\Local\Temp\claude\c--Users-benam-source-cpp-D2MOO\4663b38e-1006-4771-b1b8-d7812923eb3c\scratchpad\mods.json","w"),indent=1)
