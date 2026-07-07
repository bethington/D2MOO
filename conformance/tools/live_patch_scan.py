#!/usr/bin/env python3
r"""live_patch_scan.py -- runtime patch / inline-hook detector for a loaded PE module.

GENERAL PURPOSE (any Windows process, any PE module -- not D2-specific): compares a
module's LIVE in-memory executable bytes against the same module's ON-DISK file to
find what was changed AFTER load -- detour/trampoline prologue hooks ("jump to
alternate functionality and back") and inline byte edits. This is the standard
RE / anti-cheat / malware "inline hook scan" technique.

The whole point (conformance/D2COMMON_FULL_SHADOW_PLAN.md context): a mod like
Project Diablo 2 can patch the original functions in memory at launch. Ghidra only
ever sees the on-disk file, so its documentation is stale for any runtime-patched
function -- and a 1:1 reimpl written from that doc would target the WRONG behavior.
This tool produces the patch map up front, over the WHOLE module, instead of
discovering divergences one-at-a-time on the tiny reimplemented-and-exercised subset.

It reads the target with OpenProcess + ReadProcessMemory -- NO injection, no target
cooperation, no debugger. Point it at any pid + module + on-disk file.

Correctness (why a naive byte diff would lie): the OS loader legitimately rewrites
two things at load time, which this masks out so only REAL patches remain --
  1. Base relocations, if the module didn't load at its preferred ImageBase. We
     apply the same reloc delta to the on-disk image before diffing (for a no-ASLR
     module like D2Common at its fixed 0x6fd50000, delta = 0, so this is a no-op).
  2. The IAT / import thunks. Those live in DATA sections; we only diff EXECUTABLE
     sections, so they're excluded by construction (plus an explicit import-dir mask).

Layers:
  * generic core: scan_module(pid, module_name, on_disk_path, exclude_ranges) ->
    list of patch regions. Reusable for ANY target.
  * D2 adapter (main): finds Game.exe, auto-excludes OUR OWN D2Debugger shadow
    dispatcher hooks (from shadow_manifest.json + the coord family), and prints a
    report. Map the reported addresses to functions with your symbol source
    (Ghidra here; an export table or PDB for another target).

Usage:
  python live_patch_scan.py                         # scan Game.exe / D2Common.dll
  python live_patch_scan.py --process Foo.exe --module bar.dll --ondisk C:\path\bar.dll
  python live_patch_scan.py --json                  # machine-readable output
"""
from __future__ import annotations

import argparse
import ctypes
import json
import os
import struct
import sys
from ctypes import wintypes
from pathlib import Path

# --------------------------------------------------------------------------- #
# Win32 process/module enumeration + memory read (no third-party deps).
# --------------------------------------------------------------------------- #
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)

TH32CS_SNAPPROCESS = 0x00000002
TH32CS_SNAPMODULE = 0x00000008
TH32CS_SNAPMODULE32 = 0x00000010
PROCESS_VM_READ = 0x0010
PROCESS_QUERY_INFORMATION = 0x0400
INVALID_HANDLE_VALUE = wintypes.HANDLE(-1).value


class PROCESSENTRY32W(ctypes.Structure):
    _fields_ = [
        ("dwSize", wintypes.DWORD),
        ("cntUsage", wintypes.DWORD),
        ("th32ProcessID", wintypes.DWORD),
        ("th32DefaultHeapID", ctypes.POINTER(ctypes.c_ulong)),
        ("th32ModuleID", wintypes.DWORD),
        ("cntThreads", wintypes.DWORD),
        ("th32ParentProcessID", wintypes.DWORD),
        ("pcPriClassBase", ctypes.c_long),
        ("dwFlags", wintypes.DWORD),
        ("szExeFile", wintypes.WCHAR * 260),
    ]


class MODULEENTRY32W(ctypes.Structure):
    _fields_ = [
        ("dwSize", wintypes.DWORD),
        ("th32ModuleID", wintypes.DWORD),
        ("th32ProcessID", wintypes.DWORD),
        ("GlblcntUsage", wintypes.DWORD),
        ("ProccntUsage", wintypes.DWORD),
        ("modBaseAddr", ctypes.POINTER(ctypes.c_byte)),
        ("modBaseSize", wintypes.DWORD),
        ("hModule", wintypes.HMODULE),
        ("szModule", wintypes.WCHAR * 256),
        ("szExePath", wintypes.WCHAR * 260),
    ]


psapi = ctypes.WinDLL("psapi", use_last_error=True)

# EnumProcessModulesEx filter flags.
LIST_MODULES_32BIT = 0x01
LIST_MODULES_64BIT = 0x02
LIST_MODULES_ALL = 0x03


class MODULEINFO(ctypes.Structure):
    _fields_ = [
        ("lpBaseOfDll", ctypes.c_void_p),
        ("SizeOfImage", wintypes.DWORD),
        ("EntryPoint", ctypes.c_void_p),
    ]


kernel32.CreateToolhelp32Snapshot.restype = wintypes.HANDLE
kernel32.CreateToolhelp32Snapshot.argtypes = [wintypes.DWORD, wintypes.DWORD]
kernel32.Process32FirstW.argtypes = [wintypes.HANDLE, ctypes.POINTER(PROCESSENTRY32W)]
kernel32.Process32NextW.argtypes = [wintypes.HANDLE, ctypes.POINTER(PROCESSENTRY32W)]
kernel32.OpenProcess.restype = wintypes.HANDLE
kernel32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
kernel32.ReadProcessMemory.restype = wintypes.BOOL
kernel32.ReadProcessMemory.argtypes = [
    wintypes.HANDLE, wintypes.LPCVOID, wintypes.LPVOID, ctypes.c_size_t,
    ctypes.POINTER(ctypes.c_size_t),
]
kernel32.CloseHandle.argtypes = [wintypes.HANDLE]

# psapi: the cross-bitness-correct way to walk a 32-bit target's modules from a
# 64-bit process (Toolhelp's TH32CS_SNAPMODULE returns ERROR_ACCESS_DENIED there).
psapi.EnumProcessModulesEx.restype = wintypes.BOOL
psapi.EnumProcessModulesEx.argtypes = [
    wintypes.HANDLE, ctypes.POINTER(wintypes.HMODULE), wintypes.DWORD,
    ctypes.POINTER(wintypes.DWORD), wintypes.DWORD,
]
psapi.GetModuleBaseNameW.restype = wintypes.DWORD
psapi.GetModuleBaseNameW.argtypes = [
    wintypes.HANDLE, wintypes.HMODULE, wintypes.LPWSTR, wintypes.DWORD]
psapi.GetModuleFileNameExW.restype = wintypes.DWORD
psapi.GetModuleFileNameExW.argtypes = [
    wintypes.HANDLE, wintypes.HMODULE, wintypes.LPWSTR, wintypes.DWORD]
psapi.GetModuleInformation.restype = wintypes.BOOL
psapi.GetModuleInformation.argtypes = [
    wintypes.HANDLE, wintypes.HMODULE, ctypes.POINTER(MODULEINFO), wintypes.DWORD]


def find_pids_by_name(name: str) -> list[int]:
    name_l = name.lower()
    snap = kernel32.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
    if snap == INVALID_HANDLE_VALUE:
        raise ctypes.WinError(ctypes.get_last_error())
    pids = []
    try:
        entry = PROCESSENTRY32W()
        entry.dwSize = ctypes.sizeof(PROCESSENTRY32W)
        ok = kernel32.Process32FirstW(snap, ctypes.byref(entry))
        while ok:
            if entry.szExeFile.lower() == name_l:
                pids.append(entry.th32ProcessID)
            ok = kernel32.Process32NextW(snap, ctypes.byref(entry))
    finally:
        kernel32.CloseHandle(snap)
    return pids


def find_module(hproc, module_name: str):
    """Return (base_address:int, size:int, on_disk_path:str) for module_name in the
    already-opened process handle, or None. Uses EnumProcessModulesEx(LIST_MODULES_ALL)
    so it works cross-bitness (64-bit caller, 32-bit target)."""
    name_l = module_name.lower()
    needed = wintypes.DWORD(0)
    # first call to size the array
    psapi.EnumProcessModulesEx(hproc, None, 0, ctypes.byref(needed), LIST_MODULES_ALL)
    count = needed.value // ctypes.sizeof(wintypes.HMODULE)
    if count == 0:
        return None
    arr = (wintypes.HMODULE * count)()
    if not psapi.EnumProcessModulesEx(hproc, arr, ctypes.sizeof(arr),
                                      ctypes.byref(needed), LIST_MODULES_ALL):
        raise ctypes.WinError(ctypes.get_last_error())
    for hmod in arr[:count]:
        name_buf = ctypes.create_unicode_buffer(260)
        if not psapi.GetModuleBaseNameW(hproc, hmod, name_buf, 260):
            continue
        if name_buf.value.lower() != name_l:
            continue
        path_buf = ctypes.create_unicode_buffer(1024)
        psapi.GetModuleFileNameExW(hproc, hmod, path_buf, 1024)
        mi = MODULEINFO()
        if not psapi.GetModuleInformation(hproc, hmod, ctypes.byref(mi),
                                          ctypes.sizeof(mi)):
            raise ctypes.WinError(ctypes.get_last_error())
        return mi.lpBaseOfDll or 0, mi.SizeOfImage, path_buf.value
    return None


def read_mem(hproc, address: int, size: int) -> bytes:
    buf = (ctypes.c_char * size)()
    nread = ctypes.c_size_t(0)
    ok = kernel32.ReadProcessMemory(hproc, ctypes.c_void_p(address), buf, size,
                                    ctypes.byref(nread))
    if not ok:
        raise ctypes.WinError(ctypes.get_last_error())
    return bytes(buf[:nread.value])


# --------------------------------------------------------------------------- #
# Minimal PE32 parse (sections + reloc dir + import dir + ImageBase).
# --------------------------------------------------------------------------- #
IMAGE_SCN_MEM_EXECUTE = 0x20000000


class PE:
    def __init__(self, data: bytes):
        self.data = data
        e_lfanew = struct.unpack_from("<I", data, 0x3C)[0]
        if data[e_lfanew:e_lfanew + 4] != b"PE\x00\x00":
            raise ValueError("not a PE file")
        coff = e_lfanew + 4
        self.num_sections = struct.unpack_from("<H", data, coff + 2)[0]
        size_opt = struct.unpack_from("<H", data, coff + 16)[0]
        opt = coff + 20
        magic = struct.unpack_from("<H", data, opt)[0]
        if magic != 0x10B:
            raise ValueError(f"only PE32 (0x10b) supported here, got magic 0x{magic:x}")
        self.image_base = struct.unpack_from("<I", data, opt + 28)[0]
        num_dirs = struct.unpack_from("<I", data, opt + 92)[0]
        dir_base = opt + 96
        self.dirs = []
        for i in range(num_dirs):
            rva, size = struct.unpack_from("<II", data, dir_base + i * 8)
            self.dirs.append((rva, size))
        sec_base = opt + size_opt
        self.sections = []
        for i in range(self.num_sections):
            off = sec_base + i * 40
            name = data[off:off + 8].rstrip(b"\x00").decode("latin1", "replace")
            vsize, vaddr, rawsize, rawptr = struct.unpack_from("<IIII", data, off + 8)
            chars = struct.unpack_from("<I", data, off + 36)[0]
            self.sections.append({
                "name": name, "vsize": vsize, "vaddr": vaddr,
                "rawsize": rawsize, "rawptr": rawptr, "chars": chars,
            })

    def exec_sections(self):
        return [s for s in self.sections if s["chars"] & IMAGE_SCN_MEM_EXECUTE]

    def reloc_sites_highlow(self):
        """Yield RVA of every 4-byte IMAGE_REL_BASED_HIGHLOW relocation site."""
        if len(self.dirs) <= 5:
            return
        rva, size = self.dirs[5]
        if not rva or not size:
            return
        # reloc dir bytes come from the file at the section that contains `rva`
        base_off = self.rva_to_off(rva)
        if base_off is None:
            return
        end = base_off + size
        p = base_off
        while p + 8 <= end:
            page_rva, block_size = struct.unpack_from("<II", self.data, p)
            if block_size < 8:
                break
            n = (block_size - 8) // 2
            for i in range(n):
                entry = struct.unpack_from("<H", self.data, p + 8 + i * 2)[0]
                typ = entry >> 12
                off = entry & 0xFFF
                if typ == 3:  # HIGHLOW (x86 32-bit absolute)
                    yield page_rva + off
            p += block_size

    def rva_to_off(self, rva: int):
        for s in self.sections:
            if s["vaddr"] <= rva < s["vaddr"] + max(s["vsize"], s["rawsize"]):
                return s["rawptr"] + (rva - s["vaddr"])
        return None


# --------------------------------------------------------------------------- #
# Generic core: diff live vs on-disk executable bytes.
# --------------------------------------------------------------------------- #
def _classify(live: bytes, on_disk_base: int, region_addr: int, hproc) -> dict:
    """Classify a differing region by its LIVE first bytes. E9/EB = relative jmp,
    FF25 = absolute indirect jmp -> a detour prologue hook; else inline edit."""
    kind = "inline-edit"
    target = None
    if live[:1] == b"\xE9" and len(live) >= 5:
        rel = struct.unpack_from("<i", live, 1)[0]
        target = region_addr + 5 + rel
        kind = "prologue-jmp-hook(rel32)"
    elif live[:1] == b"\xEB" and len(live) >= 2:
        rel = struct.unpack_from("<b", live, 1)[0]
        target = region_addr + 2 + rel
        kind = "prologue-jmp-hook(short)"
    elif live[:2] == b"\xFF\x25" and len(live) >= 6:
        ptr = struct.unpack_from("<I", live, 2)[0]
        kind = "prologue-jmp-hook(indirect)"
        try:
            target = struct.unpack_from("<I", read_mem(hproc, ptr, 4), 0)[0]
        except OSError:
            target = None
        target = target if target else None
    return {"kind": kind, "hook_target": (f"0x{target:x}" if target else None)}


def scan_module(pid: int, module_name: str, on_disk_path: str | None = None,
                exclude_ranges: list[tuple[int, int]] | None = None,
                min_run: int = 1) -> dict:
    """Diff a live module's executable sections against its on-disk file.

    exclude_ranges: list of (rva_start, rva_end) to ignore (e.g. hooks the caller
    installed itself). Returns {ok, base, image_base, delta, patches:[...], ...}.
    Each patch: {rva, addr, len, on_disk(hex), live(hex), kind, hook_target}.
    """
    exclude_ranges = exclude_ranges or []
    hproc = kernel32.OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, False, pid)
    if not hproc:
        werr = ctypes.WinError(ctypes.get_last_error())
        hint = ""
        if getattr(werr, "winerror", None) == 5:
            hint = (" -- ACCESS DENIED. If an elevated scanner still can't open it "
                    "(verified against a benign process), the target is PROTECTING its "
                    "own process (anti-cheat/anti-tamper DACL, e.g. Project Diablo 2). "
                    "External ReadProcessMemory can't reach it; read it FROM INSIDE the "
                    "process instead (an injected module reading its own memory).")
        return {"ok": False, "error": f"OpenProcess(pid {pid}) failed: {werr}{hint}"}

    found = find_module(hproc, module_name)
    if not found:
        kernel32.CloseHandle(hproc)
        return {"ok": False, "error": f"module {module_name} not found in pid {pid}"}
    base, size, loaded_path = found
    on_disk_path = on_disk_path or loaded_path
    if not os.path.exists(on_disk_path):
        kernel32.CloseHandle(hproc)
        return {"ok": False, "error": f"on-disk file not found: {on_disk_path}"}

    file_bytes = Path(on_disk_path).read_bytes()
    pe = PE(file_bytes)
    delta = base - pe.image_base

    # Build an "expected loaded image" for exec sections: on-disk bytes with the
    # base-reloc delta applied (no-op when delta == 0).
    reloc_sites = set(pe.reloc_sites_highlow()) if delta else set()

    def excluded(rva, length):
        for a, b in exclude_ranges:
            if rva < b and (rva + length) > a:
                return True
        return False

    patches = []
    read_errors = []
    try:
        for s in pe.exec_sections():
            n = min(s["vsize"], s["rawsize"])
            if n <= 0:
                continue
            disk = bytearray(file_bytes[s["rawptr"]:s["rawptr"] + n])
            # apply reloc delta to disk copy at HIGHLOW sites inside this section
            if delta:
                for site in reloc_sites:
                    if s["vaddr"] <= site < s["vaddr"] + n - 3:
                        off = site - s["vaddr"]
                        val = struct.unpack_from("<I", disk, off)[0]
                        struct.pack_into("<I", disk, off, (val + delta) & 0xFFFFFFFF)
            try:
                live = read_mem(hproc, base + s["vaddr"], n)
            except OSError as e:
                read_errors.append(f"{s['name']}: {e}")
                continue
            m = min(len(live), len(disk))
            # find contiguous differing runs
            i = 0
            while i < m:
                if live[i] != disk[i]:
                    j = i
                    while j < m and live[j] != disk[j]:
                        j += 1
                    rva = s["vaddr"] + i
                    length = j - i
                    if length >= min_run and not excluded(rva, length):
                        cls = _classify(bytes(live[i:i + 16]), base, base + rva, hproc)
                        patches.append({
                            "section": s["name"], "rva": rva, "addr": f"0x{base + rva:x}",
                            "len": length,
                            "on_disk": disk[i:i + min(length, 24)].hex(),
                            "live": bytes(live[i:i + min(length, 24)]).hex(),
                            **cls,
                        })
                    i = j
                else:
                    i += 1
    finally:
        kernel32.CloseHandle(hproc)

    patches.sort(key=lambda p: p["rva"])
    return {
        "ok": True, "pid": pid, "module": module_name, "loaded_path": loaded_path,
        "on_disk_path": on_disk_path, "base": f"0x{base:x}",
        "image_base": f"0x{pe.image_base:x}", "delta": delta,
        "exec_sections": [s["name"] for s in pe.exec_sections()],
        "read_errors": read_errors, "patch_count": len(patches), "patches": patches,
    }


# --------------------------------------------------------------------------- #
# D2 adapter.
# --------------------------------------------------------------------------- #
_D2COMMON_BASE = 0x6FD50000
# Coord-family dispatcher offsets (LiveDispatch_CoordFamily.h) -- OUR hooks.
_COORD_OFFSETS = [0x4dac0, 0x4d8a0, 0x4d8c0, 0x4db40, 0x4db70]


def _our_dispatcher_exclusions(repo: Path, window: int = 24):
    """RVA ranges of OUR OWN D2Debugger shadow-dispatcher hooks, so they don't
    show up as 'patches'. From shadow_manifest.json + the coord family."""
    offs = list(_COORD_OFFSETS)
    manifest = repo / "conformance" / "shadow_manifest.json"
    if manifest.exists():
        try:
            m = json.loads(manifest.read_text(encoding="utf-8"))
            for e in m.get("entries", []):
                o = e["offset"]
                offs.append(int(o, 0) if isinstance(o, str) else int(o))
        except Exception:
            pass
    return [(o, o + window) for o in offs]


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--process", default="Game.exe", help="target process name")
    ap.add_argument("--pid", type=int, default=None, help="target pid (overrides --process)")
    ap.add_argument("--module", default="D2Common.dll", help="module to scan")
    ap.add_argument("--ondisk", default=None,
                    help="on-disk file to diff against (default: the path the module was loaded from)")
    ap.add_argument("--no-exclude-ours", action="store_true",
                    help="do NOT auto-exclude our own D2Debugger shadow-dispatcher hooks")
    ap.add_argument("--json", action="store_true", help="emit JSON")
    args = ap.parse_args()

    pid = args.pid
    if pid is None:
        pids = find_pids_by_name(args.process)
        if not pids:
            print(f"ERROR: no process named {args.process!r} is running", file=sys.stderr)
            sys.exit(2)
        pid = pids[0]
        if len(pids) > 1:
            print(f"note: multiple {args.process} pids {pids}; using {pid}", file=sys.stderr)

    repo = Path(__file__).resolve().parents[2]
    exclude = None if args.no_exclude_ours else _our_dispatcher_exclusions(repo)

    result = scan_module(pid, args.module, on_disk_path=args.ondisk, exclude_ranges=exclude)

    if args.json:
        print(json.dumps(result, indent=2))
        return

    if not result.get("ok"):
        print(f"ERROR: {result.get('error')}", file=sys.stderr)
        sys.exit(1)

    print(f"module   : {result['module']} @ {result['base']} (pid {result['pid']})")
    print(f"loaded   : {result['loaded_path']}")
    print(f"on-disk  : {result['on_disk_path']}")
    print(f"imagebase: {result['image_base']}  reloc-delta: {result['delta']}")
    print(f"exec sec : {result['exec_sections']}")
    if result["read_errors"]:
        print(f"read errs: {result['read_errors']}")
    excl = "OUR shadow hooks auto-excluded" if not args.no_exclude_ours else "NO exclusions"
    print(f"exclude  : {excl}")
    print(f"\n=== {result['patch_count']} runtime patch region(s) found ===")
    for p in result["patches"]:
        print(f"  {p['addr']} (rva 0x{p['rva']:x}) len={p['len']:<4} {p['kind']}"
              + (f" -> {p['hook_target']}" if p.get("hook_target") else ""))
        print(f"      on-disk: {p['on_disk']}")
        print(f"      live   : {p['live']}")
    if result["patch_count"] == 0:
        print("  (none -- live executable bytes match the on-disk file exactly,\n"
              "   aside from any excluded regions: no PD2 runtime code patches detected)")


if __name__ == "__main__":
    main()
