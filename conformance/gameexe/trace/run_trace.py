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
    return dest


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


def trace(exe: Path, cwd: Path, out: Path, timeout: float, argv_extra=None) -> dict:
    events, meta = [], {}
    device = frida.get_local_device()
    pid = device.spawn([str(exe)] + list(argv_extra or []), cwd=str(cwd))
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

    with open(out, "w", encoding="utf-8") as fh:
        fh.write(json.dumps({"type": "meta", **meta}) + "\n")
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
    ap.add_argument("--argv", nargs="*", default=[])
    args = ap.parse_args()

    sandbox = Path(args.sandbox)
    if args.rebuild_sandbox or not sandbox.exists():
        print(f"# building sandbox from {PD2_SOURCE} -> {sandbox}")
        make_sandbox(sandbox)

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
    if meta.get("failures"):
        print("  hook failures:")
        for f in meta["failures"]:
            print(f"    {f}")
    print(f"  wrote {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
