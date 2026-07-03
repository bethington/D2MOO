import struct, sys, os

path = os.environ.get("DLL_PATH", r"C:\Diablo2\ProjectD2\D2Common.dll")
data = open(path, "rb").read()

# DOS header -> PE header
e_lfanew = struct.unpack_from("<I", data, 0x3C)[0]
assert data[e_lfanew:e_lfanew+4] == b"PE\0\0", "not PE"
coff = e_lfanew + 4
num_sections = struct.unpack_from("<H", data, coff+2)[0]
opt_size = struct.unpack_from("<H", data, coff+16)[0]
opt = coff + 20
magic = struct.unpack_from("<H", data, opt)[0]
is64 = (magic == 0x20b)
image_base = struct.unpack_from("<Q" if is64 else "<I", data, opt + (24 if is64 else 28))[0]
# data directory offset: PE32 -> opt+96, PE32+ -> opt+112
dd = opt + (112 if is64 else 96)
export_rva, export_size = struct.unpack_from("<II", data, dd)  # dir[0] = export

# section headers to map RVA->file offset
sec_off = opt + opt_size
sections = []
for i in range(num_sections):
    o = sec_off + i*40
    name = data[o:o+8].rstrip(b"\0").decode(errors="replace")
    vsize, vaddr, rawsize, rawptr = struct.unpack_from("<IIII", data, o+8)
    sections.append((vaddr, vsize, rawptr, rawsize, name))

def rva2off(rva):
    for vaddr, vsize, rawptr, rawsize, name in sections:
        if vaddr <= rva < vaddr + max(vsize, rawsize):
            return rawptr + (rva - vaddr)
    return None

eo = rva2off(export_rva)
(char, tstamp, vmaj, vmin, name_rva, ord_base,
 naddr, nnames, addr_rva, names_rva, ords_rva) = struct.unpack_from("<IIHHIIIIIII", data, eo)

addr_off  = rva2off(addr_rva)
names_off = rva2off(names_rva)
ords_off  = rva2off(ords_rva)

# name -> ordinal-index map
name_by_ordindex = {}
for i in range(nnames):
    npr = struct.unpack_from("<I", data, names_off + i*4)[0]
    nm = data[rva2off(npr):].split(b"\0",1)[0].decode(errors="replace")
    oi = struct.unpack_from("<H", data, ords_off + i*2)[0]
    name_by_ordindex[oi] = nm

print(f"image_base=0x{image_base:x} ord_base={ord_base} naddr={naddr} nnames={nnames}")
targets = set(int(x) for x in sys.argv[1:]) if len(sys.argv) > 1 else None
for i in range(naddr):
    fn_rva = struct.unpack_from("<I", data, addr_off + i*4)[0]
    if fn_rva == 0:
        continue
    ordinal = ord_base + i
    va = image_base + fn_rva
    nm = name_by_ordindex.get(i, "")
    if targets is None or ordinal in targets:
        print(f"@{ordinal}\t0x{va:x}\t{nm}")
