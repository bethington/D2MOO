"""Generate a corrected ordinal->address->real-name map for a PD2-S12 DLL.

Joins three sources:
  1. the DLL's PE export directory   -> ordinal -> runtime address (ground truth)
  2. Ghidra's list_exports over :8089 -> address -> analyst-assigned real name
  3. D2MOO's *.1.13c.def              -> ordinal -> the name D2MOO *claims*

The D2MOO .def ordinals are known-scrambled vs the real binary (verified for
D2Common and D2Game), so this produces the trustworthy map and flags where the
.def disagrees.

Usage:
  python gen_corrected_map.py <ghidra_program> <dll_path> <def_path> <out_tsv>
"""
import struct, sys, json, re, urllib.request, urllib.parse

GHIDRA = "http://127.0.0.1:8089"


def parse_pe_exports(path):
    data = open(path, "rb").read()
    e_lfanew = struct.unpack_from("<I", data, 0x3C)[0]
    coff = e_lfanew + 4
    num_sections = struct.unpack_from("<H", data, coff + 2)[0]
    opt_size = struct.unpack_from("<H", data, coff + 16)[0]
    opt = coff + 20
    magic = struct.unpack_from("<H", data, opt)[0]
    is64 = magic == 0x20B
    image_base = struct.unpack_from("<Q" if is64 else "<I", data, opt + (24 if is64 else 28))[0]
    dd = opt + (112 if is64 else 96)
    export_rva = struct.unpack_from("<I", data, dd)[0]
    sec_off = opt + opt_size
    sections = []
    for i in range(num_sections):
        o = sec_off + i * 40
        vaddr, vsize, rawsize, rawptr = struct.unpack_from("<IIII", data, o + 8) if False else (
            struct.unpack_from("<I", data, o + 12)[0], struct.unpack_from("<I", data, o + 8)[0],
            struct.unpack_from("<I", data, o + 16)[0], struct.unpack_from("<I", data, o + 20)[0])
        sections.append((vaddr, vsize, rawptr, rawsize))

    def rva2off(rva):
        for vaddr, vsize, rawptr, rawsize in sections:
            if vaddr <= rva < vaddr + max(vsize, rawsize):
                return rawptr + (rva - vaddr)
        return None

    eo = rva2off(export_rva)
    ord_base = struct.unpack_from("<I", data, eo + 16)[0]
    naddr = struct.unpack_from("<I", data, eo + 20)[0]
    addr_rva = struct.unpack_from("<I", data, eo + 28)[0]
    addr_off = rva2off(addr_rva)
    out = []
    for i in range(naddr):
        fn_rva = struct.unpack_from("<I", data, addr_off + i * 4)[0]
        if fn_rva == 0:
            continue
        out.append((ord_base + i, image_base + fn_rva))
    return image_base, out


def fetch_ghidra_exports(program):
    url = f"{GHIDRA}/list_exports?program={urllib.parse.quote(program)}&limit=8000"
    raw = urllib.request.urlopen(url, timeout=60).read().decode("utf-8", "replace")
    try:
        j = json.loads(raw)
        raw = j.get("result", raw) if isinstance(j, dict) else raw
    except Exception:
        pass
    addr2names = {}
    for line in raw.splitlines():
        if "->" in line:
            nm, ad = line.rsplit("->", 1)
            key = ad.strip().lower().replace("0x", "")
            addr2names.setdefault(key, []).append(nm.strip())
    return addr2names


def parse_def(path):
    d = {}
    for line in open(path, encoding="utf-8", errors="replace"):
        m = re.search(r"(\S+)\s+@(\d+)", line)
        if m:
            d[int(m.group(2))] = m.group(1)
    return d


def main():
    program, dll, defp, outp = sys.argv[1:5]
    image_base, exports = parse_pe_exports(dll)
    g = fetch_ghidra_exports(program)
    dmap = parse_def(defp)
    rows = []
    named = def_present = def_agree = 0
    for ordinal, va in exports:
        key = f"{va:x}"
        gnames = g.get(key, [])
        gname = gnames[0] if gnames else ""
        if gname:
            named += 1
        defname = dmap.get(ordinal, "")
        if defname:
            def_present += 1
        # loose agreement: def name appears (case-insensitive substring either way) among ghidra names
        agree = any(defname and (defname.lower() in n.lower() or n.lower() in defname.lower())
                    for n in gnames)
        if agree:
            def_agree += 1
        rows.append((ordinal, f"0x{va:x}", gname, defname, "ok" if agree else ("DIFF" if defname else "")))
    # NOTE: ordinal+address are GROUND TRUTH (PE export table). The two name
    # columns are CANDIDATE names from two fallible sources: Ghidra's analyst name
    # and D2MOO's .def. names_agree is only a naive substring check, NOT a verdict
    # on which (if either) is correct -- see README. Both sources are known to be
    # wrong in different binaries (D2Common/D2Game: .def ordinals scrambled;
    # Fog/Storm: Ghidra names misidentified).
    with open(outp, "w", encoding="utf-8") as f:
        f.write("ordinal\taddress\tghidra_name\tdef_name\tnames_agree\n")
        for ordinal, addr, gname, defname, _ in rows:
            f.write(f"{ordinal}\t{addr}\t{gname}\t{defname}\t"
                    f"{'yes' if any(defname and (defname.lower() in n.lower() or n.lower() in defname.lower()) for n in g.get(addr[2:], [])) else ('no' if defname else '')}\n")
    print(f"{program}: image_base=0x{image_base:x} exports={len(exports)} "
          f"ghidra_named={named} def_declared={def_present} names_agree={def_agree} "
          f"names_differ={def_present - def_agree}")


if __name__ == "__main__":
    main()
