"""Run Game.exe under the tracer and record every observable side effect.

    python run_trace.py --sandbox C:\\tmp\\gameexe-sandbox --out original.jsonl
    python run_trace.py --sandbox ... --exe-from build/Game.exe --out ours.jsonl
    python run_trace.py --sandbox ... --fault no-ini --out original_no_ini.jsonl

Spawns the binary SUSPENDED (frida.spawn does not run it until resume), so
hooks are installed before the first instruction of user code -- there is no
window in which the launcher can act untraced.

SANDBOX. Every run happens in a throwaway copy of the PD2 install, because
fault injection deletes renderer DLLs and config files. What a directory copy
does NOT isolate is the REGISTRY: HKLM/HKCU\\Software\\Blizzard Entertainment
is machine-global, so registry faults are applied through save_registry() /
restore_registry() with the restore verified, never assumed.

STOP POINT. Game.exe's job ends when it resolves the loaded module's
QueryInterface export and hands off; the agent signals `handoff` there and we
terminate. That keeps runs to seconds, needs no display, and keeps
D2Client's behaviour out of a diff about Game.exe.
"""
from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import urllib.request
import time
from pathlib import Path

try:
    import frida
except ImportError:  # pragma: no cover
    sys.exit("frida missing: create D2MOO/.venv-frida and pip install frida")

HERE = Path(__file__).resolve().parent
AGENT = HERE / "agent.js"
PD2_SOURCE = Path(os.environ.get("PD2_INSTALL", r"C:\pd2"))
GAME_SUBDIR = "ProjectD2"

# Fault matrix -- the point of the whole exercise. These force the error
# paths that a normal launch never touches, which is the only way run-once
# error handling gets verified at all.
FAULTS = {
    "none": "unmodified sandbox",
    "no-ini": "delete D2.ini -- forces the ini-missing config path",
    "no-renderer": "delete the renderer DLLs -- forces the load-failure path",
    "bad-ini": "D2.ini full of garbage -- forces the parse-failure path",
    "readonly-dir": "make the game dir read-only -- forces write failures",
}


def check_sandbox_depth(sandbox: Path) -> None:
    """Refuse a sandbox whose depth changes where the game finds its install.

    `GetD2IniPath` counts the backslashes in the working directory, CAPS THE
    COUNT AT TWO, walks forward past that many, truncates there and appends
    "D2.ini". So the install root it derives depends on how deep the game
    directory sits, not on where the game actually is:

        C:\\pd2\\ProjectD2                  -> C:\\pd2\\D2.ini        (correct)
        C:\\tmp\\gameexe-sandbox\\ProjectD2  -> C:\\tmp\\D2.ini        (WRONG)

    Measured: the first sandbox was one level too deep, so the game looked
    for its ini -- and, on the same derived root, its MPQ archives -- in
    C:\\tmp, one directory ABOVE the sandbox. Archive loading then fails,
    GameStart returns 0 before LoadCurrentlySelectedModule, and the trace
    ends without ever reaching the handoff. That looked like a mysterious
    early stop; it was the sandbox path.

    The game directory must sit at exactly two backslashes, i.e. the sandbox
    root must be a single directory directly under a drive root.
    """
    game_dir = sandbox / GAME_SUBDIR
    depth = str(game_dir).count("\\")
    if depth != 2:
        raise SystemExit(
            f"!! sandbox too {'deep' if depth > 2 else 'shallow'}: "
            f"{game_dir} has {depth} backslashes, the game needs exactly 2.\n"
            f"   GetD2IniPath would derive the install root as "
            f"{str(game_dir).split(chr(92))[0]}\\{str(game_dir).split(chr(92))[1]}\\ "
            f"and look for D2.ini and the MPQs there.\n"
            f"   Use a sandbox directly under a drive root, e.g. C:\\gxs"
        )


def make_sandbox(dest: Path, link_data: bool = True) -> Path:
    """Copy the game directory; hard-link the bulk MPQ data rather than copy.

    ProjectD2 is ~117 MB and is what faults touch; the MPQs are ~1.9 GB and
    are read-only inputs. Copying those every run would make the fault matrix
    too slow to run often, and a verification step nobody runs is not one.
    """
    if dest.exists():
        shutil.rmtree(dest, ignore_errors=True)
    dest.mkdir(parents=True, exist_ok=True)
    for entry in PD2_SOURCE.iterdir():
        target = dest / entry.name
        if entry.is_dir():
            shutil.copytree(entry, target, dirs_exist_ok=True)
        elif link_data and entry.suffix.lower() == ".mpq":
            try:
                os.link(entry, target)          # same volume: instant
            except OSError:
                shutil.copy2(entry, target)
        else:
            shutil.copy2(entry, target)
    # Record what was copied, so verify_sandbox() checks the sandbox against
    # its own build rather than against a live install that keeps changing.
    game_dir = dest / GAME_SUBDIR
    if game_dir.is_dir():
        (dest / ".sandbox-manifest.json").write_text(
            json.dumps(sorted(p.name for p in game_dir.iterdir()
                              if p.is_file() and _is_binary(p))),
            encoding="utf-8")
    return dest


def _is_binary(p: Path) -> bool:
    """Only executables and libraries are worth verifying.

    The manifest covers .exe/.dll ONLY, because the game rewrites and DELETES
    its own files as it runs -- a traced launch removed the D2*.txt log that
    had been copied in, and the check then reported the sandbox as corrupt on
    the very next run. The failure this guard exists for is a MISSING BINARY
    (an interrupted copy leaving the loader unable to resolve a DLL, which
    surfaces as "LoadLibraryW failed with error code 7e"); logs and saves are
    the game's business, not ours.
    """
    return p.suffix.lower() in (".exe", ".dll")


def verify_sandbox(sandbox: Path) -> None:
    """Refuse a sandbox that is not a complete copy of the game directory.

    An interrupted `--rebuild-sandbox` leaves a PARTIAL tree, because the
    rebuild deletes before it copies. Measured 2026-08-08: a killed
    fault-matrix run left C:\\gxs\\ProjectD2 with 63 of 67 files, and the next
    launch died in SlashGaming's loader with "LoadLibraryW failed with error
    code 7e" (ERROR_MOD_NOT_FOUND) -- a dialog on the operator's screen and a
    trace that proved nothing. Worse, the pristine snapshot taken for fast
    fault resets captured that broken state, so it would have poisoned every
    later run too.

    Compare against the real install and refuse rather than run.
    """
    dst = sandbox / GAME_SUBDIR
    if not dst.is_dir():
        raise SystemExit(f"!! no game directory in {sandbox} -- rebuild it")
    manifest = sandbox / ".sandbox-manifest.json"
    if not manifest.exists():
        # Built before manifests existed; nothing trustworthy to check against.
        return
    # Compare against WHAT WAS COPIED, not against the live install. The live
    # install is a MOVING TARGET: the game writes logs into its own directory
    # (a D2*.txt appeared there mid-session), so diffing against it reports
    # files "missing" from the sandbox that never existed when it was built.
    # A check that cries wolf gets switched off, which would have cost the
    # real detection this exists for.
    expected = set(json.loads(manifest.read_text(encoding="utf-8")))
    actual = {p.name for p in dst.iterdir() if p.is_file()}
    missing = expected - actual
    if missing:
        raise SystemExit(
            f"!! sandbox is INCOMPLETE: {len(missing)} of {len(expected)} "
            f"file(s) missing from {dst}\n   e.g. "
            f"{', '.join(sorted(missing)[:6])}\n"
            f"   An interrupted --rebuild-sandbox leaves a partial tree. "
            f"Delete {sandbox} and rebuild."
        )


def reset_game_dir(sandbox: Path) -> str:
    """Restore only the game directory, not the whole sandbox.

    Every fault is destructive (deleted ini, deleted renderer DLLs,
    corrupted config), so the sandbox has to be clean before each run --
    but rebuilding all of it re-copies ~2 GB of MPQ archives that no fault
    ever touches, which made a four-fault matrix take twenty minutes. The
    game directory is ~117 MB and is the only part faults reach, so keep a
    pristine copy of it and restore that instead. A verification step too
    slow to run often stops being run.
    """
    game_dir = sandbox / GAME_SUBDIR
    pristine = sandbox / (GAME_SUBDIR + "__pristine")
    if not pristine.exists():
        shutil.copytree(game_dir, pristine)
        return f"snapshotted {GAME_SUBDIR} ({GAME_SUBDIR}__pristine)"
    subprocess.run(["attrib", "-R", str(game_dir / "*.*"), "/S"],
                   capture_output=True)   # readonly-dir fault would block rmtree
    shutil.rmtree(game_dir, ignore_errors=True)
    shutil.copytree(pristine, game_dir)
    return f"restored {GAME_SUBDIR} from pristine copy"


def apply_fault(sandbox: Path, fault: str) -> list:
    """Apply a named fault. Returns a human-readable list of what changed."""
    game_dir = sandbox / GAME_SUBDIR
    done = []
    if fault in (None, "none"):
        return ["(none)"]
    if fault == "no-ini":
        for p in list(sandbox.rglob("D2.ini")):
            p.unlink()
            done.append(f"deleted {p.relative_to(sandbox)}")
    elif fault == "bad-ini":
        for p in list(sandbox.rglob("D2.ini")):
            p.write_text("!!! not an ini !!!\n\x00\x01garbage\n", encoding="latin-1")
            done.append(f"corrupted {p.relative_to(sandbox)}")
    elif fault == "no-renderer":
        for name in ("D2Direct3D.dll", "D2Glide.dll", "D2DDraw.dll", "D2Gdi.dll"):
            p = game_dir / name
            if p.exists():
                p.unlink()
                done.append(f"deleted {name}")
    elif fault == "readonly-dir":
        subprocess.run(["attrib", "+R", str(game_dir / "*.*")],
                       capture_output=True)
        done.append("set +R on game dir")
    else:
        raise SystemExit(f"unknown fault {fault!r}; known: {', '.join(FAULTS)}")
    if not done:
        # An "applied" fault that changed nothing would silently produce a
        # clean-run trace labelled as a fault run -- the worst kind of pass.
        raise SystemExit(f"fault {fault!r} matched no files in {sandbox}")
    return done


DASHBOARD_KILL = "http://127.0.0.1:5000/api/oracle/kill"

# A FIXED environment for every traced launch.
#
# The CRT copies the whole process environment onto the heap during startup,
# so each variable becomes a HeapAlloc whose SIZE is that variable's length.
# Two runs launched from shells with any difference at all -- a changed PWD,
# an exported flag, the shell's `_` variable -- allocate different sizes and
# the diff reports a divergence that has nothing to do with the binary.
# Measured: a byte-IDENTICAL patched Game.exe differed from the original on
# exactly one event, HeapAlloc(0x41) against HeapAlloc(0x3b), purely because
# the two runs inherited different environments.
#
# Pinning the environment makes runs hermetic and comparable across sessions
# and machines. Keep it minimal but sufficient: Windows needs SystemRoot to
# load DLLs at all, and the game reads TEMP.
FIXED_ENV = {
    "SystemRoot": os.environ.get("SystemRoot", r"C:\Windows"),
    "windir": os.environ.get("windir", r"C:\Windows"),
    "TEMP": os.environ.get("TEMP", r"C:\Windows\Temp"),
    "TMP": os.environ.get("TMP", r"C:\Windows\Temp"),
    "PATH": os.environ.get("SystemRoot", r"C:\Windows") + r"\system32",
    "NUMBER_OF_PROCESSORS": "1",
    "PROCESSOR_ARCHITECTURE": "x86",
}


def refuse_if_game_running() -> None:
    """Never launch alongside another Diablo II.

    Diablo II permits one instance; a second launch throws a modal
    "Only one copy of Diablo II may run at a time" dialog on the operator's
    screen and produces a trace that stops early and proves nothing. That
    happened repeatedly while building this -- always because a PREVIOUS
    traced process had leaked, not because anyone was playing. Check before
    spawning rather than discovering it afterwards.
    """
    out = subprocess.run(["tasklist", "/FI", "IMAGENAME eq Game.exe", "/NH"],
                         capture_output=True, text=True)
    if "Game.exe" in (out.stdout or ""):
        pids = [ln.split()[1] for ln in out.stdout.splitlines()
                if "Game.exe" in ln and len(ln.split()) > 1]
        raise SystemExit(
            "!! a Diablo II is already running (pid {}).\n"
            "   Tracing now would trip the single-instance check and pop a "
            "modal dialog on the operator's screen.\n"
            "   If it is a leaked trace process, clear it with:\n"
            "     curl -X POST {} -H \"Content-Type: application/json\" "
            "-d \"{{\\\"confirm\\\": true}}\""
            .format(", ".join(pids), DASHBOARD_KILL)
        )


def ensure_dead(pid: int, grace: float = 3.0) -> str:
    """Guarantee the traced process is gone before returning.

    `device.kill()` is NOT sufficient. Game.exe applies a deny-all DACL to
    its own process (ApplyProcessSecurityRestrictions @ 0x408120), so an
    ordinary same-user kill returns "Access is denied" and the process
    SURVIVES. Measured 2026-08-08: two traced processes were left alive at
    once, and the next launch tripped Diablo II's single-instance check and
    threw a modal dialog on the operator's screen. A tracer that leaks a
    process makes its own next run fail, and someone else's game too.

    The elevated dashboard holds SeDebugPrivilege and can kill through the
    DACL without a UAC prompt, so fall back to it and then VERIFY. Never
    assume the kill worked -- that assumption is what let this happen.
    """
    deadline = time.time() + grace
    while time.time() < deadline:
        if not _pid_alive(pid):
            return "exited"
        time.sleep(0.2)
    try:
        req = urllib.request.Request(
            DASHBOARD_KILL, data=json.dumps({"confirm": True}).encode(),
            headers={"Content-Type": "application/json"}, method="POST")
        urllib.request.urlopen(req, timeout=30).read()
    except Exception as e:  # noqa: BLE001
        return f"LEAKED pid {pid}: dashboard kill unreachable ({str(e)[:60]})"
    time.sleep(0.5)
    if _pid_alive(pid):
        return (f"LEAKED pid {pid} -- STILL RUNNING. Kill it before tracing "
                f"again or the next run will trip the single-instance dialog.")
    return "killed via elevated dashboard"


def _pid_alive(pid: int) -> bool:
    out = subprocess.run(["tasklist", "/FI", f"PID eq {pid}", "/NH"],
                         capture_output=True, text=True)
    return str(pid) in (out.stdout or "")


def trace(exe: Path, cwd: Path, out: Path, timeout: float, argv_extra=None) -> dict:
    events, meta = [], {}
    device = frida.get_local_device()
    pid = device.spawn([str(exe)] + list(argv_extra or []), cwd=str(cwd),
                       env=FIXED_ENV)
    session = device.attach(pid)
    script = session.create_script(AGENT.read_text(encoding="utf-8"))

    done = {"handoff": False}

    def on_message(message, data):
        if message["type"] == "send":
            p = message["payload"]
            kind = p.get("type")
            if kind == "ready":
                meta.update(p)
            elif kind == "handoff":
                done["handoff"] = True
                meta["handoff_after"] = p.get("after")
            elif kind == "truncated":
                meta["truncated_at"] = p.get("at")
            else:
                events.append(p)
        else:
            events.append({"type": "agent_error", "detail": message})

    # The spawned game MUST die even if hook installation raises. Without
    # this, a crashing driver leaves a live Game.exe behind -- and a stray
    # second launcher shares the machine's registry and named events with a
    # real game session.
    try:
        script.on("message", on_message)
        script.load()
        script.exports_sync.start({"stopOnHandoff": True})
        device.resume(pid)

        deadline = time.time() + timeout
        while time.time() < deadline and not done["handoff"]:
            time.sleep(0.05)
        meta["reached_handoff"] = done["handoff"]
        meta["elapsed_sec"] = round(timeout - (deadline - time.time()), 2)
    finally:
        for shutdown in (lambda: session.detach(), lambda: device.kill(pid)):
            try:
                shutdown()
            except Exception:
                pass
        meta["cleanup"] = ensure_dead(pid)

    with open(out, "w", encoding="utf-8") as fh:
        # `type` LAST: meta carries the agent's own "ready" type and would
        # otherwise override this, writing the header as an event. The differ
        # then found no meta at all and reported both runs as "did not reach
        # the handoff" while happily diffing them.
        fh.write(json.dumps({**meta, "type": "meta"}) + "\n")
        for e in events:
            fh.write(json.dumps(e) + "\n")
    meta["events"] = len(events)
    return meta


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--sandbox", required=True, help="throwaway copy directory")
    ap.add_argument("--rebuild-sandbox", action="store_true",
                    help="recreate the sandbox from PD2_INSTALL first")
    ap.add_argument("--exe-from", help="replace Game.exe with this build before tracing")
    ap.add_argument("--fault", default="none", choices=sorted(FAULTS))
    ap.add_argument("--out", required=True)
    ap.add_argument("--timeout", type=float, default=30.0)
    ap.add_argument("--allow-concurrent-game", action="store_true",
                    help="trace even if a Diablo II is already running. Safe "
                         "ONLY because the agent hides other instances from "
                         "the traced process (d2gfx checks via FindWindowA); "
                         "without that this pops a modal dialog.")
    ap.add_argument("--argv", nargs="*", default=[])
    args = ap.parse_args()

    sandbox = Path(args.sandbox)
    check_sandbox_depth(sandbox)
    if args.rebuild_sandbox or not sandbox.exists():
        print(f"# building sandbox from {PD2_SOURCE} -> {sandbox}")
        make_sandbox(sandbox)

    verify_sandbox(sandbox)
    if not args.allow_concurrent_game:
        refuse_if_game_running()
    print(f"  {reset_game_dir(sandbox)}")
    for change in apply_fault(sandbox, args.fault):
        print(f"  fault[{args.fault}]: {change}")

    exe = sandbox / GAME_SUBDIR / "Game.exe"
    if not exe.exists():
        exe = sandbox / "Game.exe"
    if args.exe_from:
        shutil.copy2(args.exe_from, exe)
        print(f"  using build: {args.exe_from}")
    if not exe.exists():
        return print(f"!! no Game.exe under {sandbox}") or 1

    meta = trace(exe, exe.parent, Path(args.out), args.timeout, args.argv)
    print(f"\n  imports {meta.get('imports')} hooked {meta.get('hooked')} "
          f"failed {meta.get('failed')}")
    print(f"  events {meta.get('events')}  handoff={meta.get('reached_handoff')}  "
          f"{meta.get('elapsed_sec')}s")
    cleanup = meta.get("cleanup", "?")
    marker = "  <<<< LEAK" if "LEAK" in str(cleanup) else ""
    print(f"  cleanup: {cleanup}{marker}")
    if meta.get("failures"):
        print("  hook failures:")
        for f in meta["failures"]:
            print(f"    {f}")
    print(f"  wrote {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
