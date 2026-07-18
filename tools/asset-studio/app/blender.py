"""Blender headless render driver (S4) — GLB -> transparent sprite at a chosen angle.

Locates blender.exe, runs blender/render_glb.py in --background. Returns the PNG path(s).
"""

from __future__ import annotations

import glob
import os
import subprocess

_SCRIPT = os.path.join(os.path.dirname(__file__), "..", "blender", "render_glb.py")

_CANDIDATES = [
	r"C:\Program Files\Blender Foundation\*\blender.exe",
	r"C:\Program Files (x86)\Steam\steamapps\common\Blender\blender.exe",
]


def blender_exe() -> str | None:
	if os.environ.get("BLENDER_EXE") and os.path.exists(os.environ["BLENDER_EXE"]):
		return os.environ["BLENDER_EXE"]
	for pat in _CANDIDATES:
		hits = sorted(glob.glob(pat))
		if hits:
			return hits[-1]  # newest version
	return None


def available() -> bool:
	return blender_exe() is not None


def render(glb_path: str, out_png: str, *, size: int = 256, azim: float = 0.0,
           elev: float = 20.0, frames: int = 1, azim_step: float = 45.0,
           timeout: float = 240.0) -> list[str]:
	"""Render glb_path to out_png (transparent, orthographic) at (azim, elev). Returns paths."""
	exe = blender_exe()
	if not exe:
		raise RuntimeError("Blender not found (set BLENDER_EXE or install Blender)")
	os.makedirs(os.path.dirname(os.path.abspath(out_png)), exist_ok=True)
	args = [exe, "--background", "--factory-startup", "--python", os.path.abspath(_SCRIPT), "--",
	        "--glb", os.path.abspath(glb_path), "--out", os.path.abspath(out_png),
	        "--size", str(int(size)), "--azim", str(float(azim)), "--elev", str(float(elev)),
	        "--frames", str(int(frames)), "--azim-step", str(float(azim_step))]
	proc = subprocess.run(args, capture_output=True, text=True, timeout=timeout)
	out = proc.stdout + proc.stderr
	oks = [ln.split(" ", 1)[1].strip() for ln in out.splitlines() if ln.startswith("RENDER_OK ")]
	if not oks:
		err = next((ln for ln in out.splitlines() if "RENDER_ERROR" in ln or "Error" in ln), "unknown")
		raise RuntimeError(f"Blender render produced no output ({err})")
	return oks
