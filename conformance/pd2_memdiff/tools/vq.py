import ctypes, ctypes.wintypes as wt
PID=110012
k=ctypes.WinDLL("kernel32",use_last_error=True)
class MBI(ctypes.Structure):
    _fields_=[("BaseAddress",ctypes.c_void_p),("AllocationBase",ctypes.c_void_p),
      ("AllocationProtect",wt.DWORD),("RegionSize",ctypes.c_size_t),
      ("State",wt.DWORD),("Protect",wt.DWORD),("Type",wt.DWORD)]
h=k.OpenProcess(0x400|0x10,False,PID)
STATE={0x1000:"COMMIT",0x2000:"RESERVE",0x10000:"FREE"}
TYPE={0x20000:"PRIVATE",0x40000:"MAPPED",0x1000000:"IMAGE"}
for addr in [0xA042F5C6,0xA0484A0C,0x1023F410,0x1040E9DC]:
    m=MBI()
    k.VirtualQueryEx(h,ctypes.c_void_p(addr),ctypes.byref(m),ctypes.sizeof(m))
    ab=m.AllocationBase or 0; bs=m.BaseAddress or 0
    print(f"0x{addr:08X}: AllocBase=0x{ab:08X} RegionSize=0x{m.RegionSize:X} "
          f"State={STATE.get(m.State,m.State)} Type={TYPE.get(m.Type,m.Type)} Protect=0x{m.Protect:X}")
k.CloseHandle(h)
