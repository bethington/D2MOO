import ctypes, ctypes.wintypes as wt, sys, struct
import pefile

PID = 110012
MODNAME = "D2Common.dll"
DISK = r"C:\Diablo2\ProjectD2\D2Common.dll"

# ---- Toolhelp to find module base + size ----
TH32CS_SNAPMODULE = 0x8
TH32CS_SNAPMODULE32 = 0x10

class MODULEENTRY32(ctypes.Structure):
    _fields_ = [
        ("dwSize", wt.DWORD), ("th32ModuleID", wt.DWORD),
        ("th32ProcessID", wt.DWORD), ("GlblcntUsage", wt.DWORD),
        ("ProccntUsage", wt.DWORD), ("modBaseAddr", ctypes.POINTER(ctypes.c_byte)),
        ("modBaseSize", wt.DWORD), ("hModule", wt.HMODULE),
        ("szModule", ctypes.c_char * 256), ("szExePath", ctypes.c_char * 260),
    ]

k32 = ctypes.WinDLL("kernel32", use_last_error=True)

def find_module():
    snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, PID)
    if snap == -1:
        raise ctypes.WinError(ctypes.get_last_error())
    me = MODULEENTRY32(); me.dwSize = ctypes.sizeof(MODULEENTRY32)
    ok = k32.Module32First(snap, ctypes.byref(me))
    while ok:
        name = me.szModule.decode(errors="replace")
        if name.lower() == MODNAME.lower():
            base = ctypes.cast(me.modBaseAddr, ctypes.c_void_p).value
            k32.CloseHandle(snap)
            return base, me.modBaseSize, me.szExePath.decode(errors="replace")
        ok = k32.Module32Next(snap, ctypes.byref(me))
    k32.CloseHandle(snap)
    raise RuntimeError("module not found")

PROCESS_VM_READ = 0x10
PROCESS_QUERY_INFORMATION = 0x400

def read_mem(base, size):
    h = k32.OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, False, PID)
    if not h:
        raise ctypes.WinError(ctypes.get_last_error())
    buf = (ctypes.c_char * size)()
    nread = ctypes.c_size_t(0)
    ok = k32.ReadProcessMemory(h, ctypes.c_void_p(base), buf, size,
                               ctypes.byref(nread))
    # Some pages may be unreadable; fall back to page-by-page
    out = bytearray(size)
    if ok and nread.value == size:
        out[:] = buf.raw
    else:
        PAGE = 0x1000
        for off in range(0, size, PAGE):
            n = min(PAGE, size - off)
            pbuf = (ctypes.c_char * n)()
            pn = ctypes.c_size_t(0)
            r = k32.ReadProcessMemory(h, ctypes.c_void_p(base + off), pbuf, n,
                                      ctypes.byref(pn))
            if r and pn.value == n:
                out[off:off+n] = pbuf.raw
            else:
                out[off:off+n] = b"\x00" * n  # unreadable
    k32.CloseHandle(h)
    return bytes(out)

base, size, path = find_module()
print(f"# module base=0x{base:08X} size=0x{size:X} path={path}")

mem = read_mem(base, size)

pe = pefile.PE(DISK, fast_load=False)
image_base = pe.OPTIONAL_HEADER.ImageBase
image_size = pe.OPTIONAL_HEADER.SizeOfImage
print(f"# disk preferred base=0x{image_base:08X} SizeOfImage=0x{image_size:X}")
delta = base - image_base
print(f"# relocation delta = 0x{delta & 0xFFFFFFFF:08X} ({delta})")

# Build expected in-memory image from disk file, section by section
expected = bytearray(min(image_size, size))
# headers
hdr = pe.header
expected[:len(hdr)] = hdr
for s in pe.sections:
    va = s.VirtualAddress
    raw = s.get_data()  # raw file bytes for this section (already sized)
    vsize = s.Misc_VirtualSize
    n = min(len(raw), vsize, len(expected) - va)
    if n > 0:
        expected[va:va+n] = raw[:n]

# Apply base relocations to `expected` so it matches the loaded image
reloc_rvas = set()
if delta != 0 and hasattr(pe, "DIRECTORY_ENTRY_BASERELOC"):
    for block in pe.DIRECTORY_ENTRY_BASERELOC:
        for e in block.entries:
            if e.type == 0:  # IMAGE_REL_BASED_ABSOLUTE (padding)
                continue
            rva = e.rva
            reloc_rvas.add(rva)
            if rva + 4 <= len(expected):
                orig = struct.unpack_from("<I", expected, rva)[0]
                struct.pack_into("<I", expected, rva, (orig + delta) & 0xFFFFFFFF)

# Mark IAT ranges to ignore (loader fills these)
ignore = bytearray(len(expected))  # 1 = ignore
def mark(rva, length):
    for i in range(rva, min(rva+length, len(ignore))):
        ignore[i] = 1

dd = pe.OPTIONAL_HEADER.DATA_DIRECTORY
# Import Address Table directory (entry 12)
iat = dd[12]
if iat.VirtualAddress and iat.Size:
    mark(iat.VirtualAddress, iat.Size)
# Also mark the IAT thunks referenced by the import directory (entry 1)
if hasattr(pe, "DIRECTORY_ENTRY_IMPORT"):
    for imp in pe.DIRECTORY_ENTRY_IMPORT:
        first = imp.struct.FirstThunk
        # each thunk is 4 bytes; count imports
        for i, ie in enumerate(imp.imports):
            mark(first + i*4, 4)
# reloc entries themselves already applied; not ignored (we expect them to match now)

# Compare
n = min(len(expected), len(mem))
diffs = []  # (rva, disk_byte, mem_byte)
run_start = None
runs = []  # (start_rva, end_rva)  half-open
i = 0
while i < n:
    if not ignore[i] and expected[i] != mem[i]:
        if run_start is None:
            run_start = i
        last = i
    else:
        if run_start is not None:
            runs.append((run_start, last+1))
            run_start = None
    i += 1
if run_start is not None:
    runs.append((run_start, last+1))

# Merge runs separated by small gaps (<=8 bytes) to group a patch site
merged = []
GAP = 8
for r in runs:
    if merged and r[0] - merged[-1][1] <= GAP:
        merged[-1] = (merged[-1][0], r[1])
    else:
        merged.append(list(r))

# Map RVA to section
def sect_of(rva):
    for s in pe.sections:
        if s.VirtualAddress <= rva < s.VirtualAddress + max(s.Misc_VirtualSize, s.SizeOfRawData):
            return s.Name.rstrip(b"\x00").decode(errors="replace")
    return "?"

print(f"# total differing runs (gap<= {GAP}): {len(merged)}")
total_bytes = sum(r[1]-r[0] for r in merged)
print(f"# total differing bytes: {total_bytes}")
print("# RVA_start  RVA_end  len  section   disk_bytes -> mem_bytes")
out_lines = []
for (a,b) in merged:
    L = b-a
    db = bytes(expected[a:b])
    mb = bytes(mem[a:b])
    line = f"0x{a:06X} 0x{b:06X} {L:4d} {sect_of(a):8s} {db[:24].hex()} -> {mb[:24].hex()}"
    print(line)
    out_lines.append((a,b,L,sect_of(a),db,mb))

# Save for mapping step
import json
with open(r"C:\Users\benam\AppData\Local\Temp\claude\c--Users-benam-source-cpp-D2MOO\4663b38e-1006-4771-b1b8-d7812923eb3c\scratchpad\diffs.json","w") as f:
    json.dump({"base":base,"image_base":image_base,"delta":delta,
               "runs":[{"rva_start":a,"rva_end":b,"len":L,"section":sec,
                        "disk":db.hex(),"mem":mb.hex()} for (a,b,L,sec,db,mb) in out_lines]}, f, indent=1)
print("# saved diffs.json")
