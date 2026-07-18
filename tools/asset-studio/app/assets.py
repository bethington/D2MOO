"""Asset conversion + overlay management for the Asset Studio app.

- Render an item's inv DC6 (from the MPQs) to a PNG for the browser.
- Import a PNG as an alternate: fit to the item's cell grid, quantize to the act
  palette, encode DC6.
- Activate original / an alternate: mirror the chosen DC6 into the overlay tree.
- Build patch.mpq from the overlay (v1 + PKWARE) for the in-game override channel.
"""

from __future__ import annotations

import io
import json
import os
import sys

from PIL import Image

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from pyd2 import dc6  # noqa: E402
from pyd2.mpq import read_effective, build_archive  # noqa: E402
from pyd2.palette import load_pal_dat, frame_to_rgba  # noqa: E402

WORKSPACE = os.environ.get("ASSET_STUDIO_WS", r"C:\Diablo2\AssetStudio")
OVERLAY = os.path.join(WORKSPACE, "overlay")
ALTERNATES = os.path.join(WORKSPACE, "alternates")
EXPORT_MPQ = os.path.join(WORKSPACE, "export", "patch.mpq")
MANIFEST = os.path.join(WORKSPACE, "manifest.json")
CELL_PX = 29  # inventory grid cell size in pixels

_PAL_CACHE = {}


def _palette(act: int = 1):
	if act not in _PAL_CACHE:
		data, _ = read_effective(f"data\\global\\palette\\ACT{act}\\pal.dat")
		_PAL_CACHE[act] = load_pal_dat(data)
	return _PAL_CACHE[act]


def item_dc6_path(invfile: str) -> str:
	return f"data\\global\\items\\{invfile}.dc6"


def read_original_dc6(invfile: str) -> bytes:
	data, _src = read_effective(item_dc6_path(invfile))
	return data


def try_read_effective(rel_path: str):
	"""read_effective, but None instead of raising when the file doesn't resolve."""
	try:
		data, _src = read_effective(rel_path)
		return data
	except Exception:  # noqa: BLE001
		return None


def dc6_to_png_bytes(dc6_bytes: bytes, frame_index: int = 0) -> bytes:
	sprite = dc6.decode(dc6_bytes)
	frame = sprite.frames[frame_index]
	w, h, rgba = frame_to_rgba(frame, _palette())
	buf = io.BytesIO()
	Image.frombytes("RGBA", (w, h), rgba).save(buf, format="PNG")
	return buf.getvalue()


def _quantize_to_palette(img: Image.Image, palette):
	"""Nearest-palette-index quantize an RGBA image; alpha<128 -> transparent (index -1)."""
	import numpy as np

	rgba = img.convert("RGBA")
	arr = np.asarray(rgba, dtype=np.int32)  # H,W,4 -- int32 so squared distances don't overflow
	h, w = arr.shape[0], arr.shape[1]
	pal = np.array([palette[i] for i in range(256)], dtype=np.int32)  # 256,3
	rgb = arr[:, :, :3].reshape(-1, 3)  # N,3
	# nearest palette index by squared distance (skip index 0 = transparent slot)
	d = ((rgb[:, None, :] - pal[None, 1:, :]) ** 2).sum(axis=2)  # N,255
	idx = d.argmin(axis=1) + 1  # 1..255
    # transparency from alpha
	alpha = arr[:, :, 3].reshape(-1)
	rows = []
	flat = idx.reshape(h, w)
	amask = (alpha < 128).reshape(h, w)
	for y in range(h):
		row = []
		for x in range(w):
			row.append(dc6.TRANSPARENT if amask[y, x] else int(flat[y, x]))
		rows.append(row)
	return rows


def png_to_item_dc6(png_bytes: bytes, invwidth: int, invheight: int) -> bytes:
	"""Fit a PNG to the item's cell grid, quantize to the palette, encode a 1-frame DC6."""
	target_w = invwidth * CELL_PX
	target_h = invheight * CELL_PX
	img = Image.open(io.BytesIO(png_bytes)).convert("RGBA")
	# scale to fit within the cell grid, preserving aspect, centered
	img.thumbnail((target_w, target_h), Image.LANCZOS)
	canvas = Image.new("RGBA", (target_w, target_h), (0, 0, 0, 0))
	canvas.paste(img, ((target_w - img.width) // 2, (target_h - img.height) // 2))
	rows = _quantize_to_palette(canvas, _palette())
	frame = dc6.Dc6Frame(flip=0, width=target_w, height=target_h, offset_x=0, offset_y=0, pixels=rows)
	sprite = dc6.Dc6File(directions=1, frames_per_direction=1, termination=b"\xee\xee\xee\xee", frames=[frame])
	return dc6.encode(sprite)


# ---- flippy (animated ground-drop DC6) ----------------------------------

def pngs_to_flippy_dc6(png_frames: list[bytes], ref_dc6_bytes: bytes) -> bytes:
	"""Encode a tumble-frame sequence as a flippy DC6 matched to the original's geometry.

	The original flippy's per-frame offsets trace the item's fall arc (offset_y climbs
	from ~-140 back to 0) and its frame sizes set the on-ground visual scale.  We keep
	the frame count, reuse each original frame's offsets (center-corrected for our
	uniform box), and size the box just above the original's largest frame so the drop
	reads at the same scale as every other item.
	"""
	ref = dc6.decode(ref_dc6_bytes)
	if len(png_frames) != len(ref.frames):
		raise ValueError(f"need {len(ref.frames)} frames to match the original flippy, "
		                 f"got {len(png_frames)}")
	box = max(max(f.width, f.height) for f in ref.frames) + 4
	pal = _palette()
	frames = []
	for png, orig in zip(png_frames, ref.frames):
		img = Image.open(io.BytesIO(png)).convert("RGBA")
		img.thumbnail((box, box), Image.LANCZOS)
		canvas = Image.new("RGBA", (box, box), (0, 0, 0, 0))
		canvas.paste(img, ((box - img.width) // 2, (box - img.height) // 2))
		rows = _quantize_to_palette(canvas, pal)
		ox = orig.offset_x - (box - orig.width) // 2  # keep the original frame's center
		oy = orig.offset_y  # keep the fall-arc anchor (bottom-up draw)
		frames.append(dc6.Dc6Frame(flip=0, width=box, height=box,
		                           offset_x=ox, offset_y=oy, pixels=rows))
	sprite = dc6.Dc6File(directions=ref.directions,
	                     frames_per_direction=ref.frames_per_direction,
	                     termination=b"\xee\xee\xee\xee", frames=frames)
	return dc6.encode(sprite)


def dc6_to_gif_bytes(dc6_bytes: bytes, bg=(48, 48, 56)) -> bytes:
	"""Animated-GIF preview of a multi-frame DC6 (frames composited on a dark bg)."""
	sprite = dc6.decode(dc6_bytes)
	pal = _palette()
	# common canvas that fits every frame at its anchor
	w = max(f.width for f in sprite.frames)
	h = max(f.height for f in sprite.frames)
	ims = []
	for f in sprite.frames:
		fw, fh, rgba = frame_to_rgba(f, pal)
		fr = Image.frombytes("RGBA", (fw, fh), rgba)
		canvas = Image.new("RGBA", (w, h), bg + (255,))
		canvas.paste(fr, ((w - fw) // 2, (h - fh) // 2), fr)
		ims.append(canvas.convert("P", palette=Image.ADAPTIVE))
	buf = io.BytesIO()
	ims[0].save(buf, format="GIF", save_all=True, append_images=ims[1:],
	            duration=80, loop=0, disposal=2)
	return buf.getvalue()


# ---- overlay + manifest -------------------------------------------------

def _load_manifest():
	if os.path.exists(MANIFEST):
		try:
			with open(MANIFEST, encoding="utf-8-sig") as f:  # tolerate a BOM
				return json.load(f)
		except (json.JSONDecodeError, OSError):
			pass
	return {"version": 1, "assets": {}, "owned_overlay_files": []}


def _save_manifest(m):
	os.makedirs(os.path.dirname(MANIFEST), exist_ok=True)
	with open(MANIFEST, "w", encoding="utf-8") as f:
		json.dump(m, f, indent=1)


def alt_dir(item_id: str) -> str:
	return os.path.join(ALTERNATES, item_id.replace("/", os.sep))


def list_alternates(item_id: str):
	d = alt_dir(item_id)
	if not os.path.isdir(d):
		return []
	out = []
	for name in sorted(os.listdir(d)):
		if name.endswith(".dc6"):
			out.append(name[:-4])
	return out


def save_alternate_dc6(item_id: str, alt_id: str, dc6_bytes: bytes):
	d = alt_dir(item_id)
	os.makedirs(d, exist_ok=True)
	with open(os.path.join(d, alt_id + ".dc6"), "wb") as f:
		f.write(dc6_bytes)


def alt_dc6_bytes(item_id: str, alt_id: str) -> bytes:
	with open(os.path.join(alt_dir(item_id), alt_id + ".dc6"), "rb") as f:
		return f.read()


def _write_overlay(rel: str, payload: bytes):
	dest = os.path.join(OVERLAY, *rel.split("\\"))
	os.makedirs(os.path.dirname(dest), exist_ok=True)
	tmp = dest + ".tmp"
	with open(tmp, "wb") as f:
		f.write(payload)
	os.replace(tmp, dest)


def _remove_overlay(rel: str):
	dest = os.path.join(OVERLAY, *rel.split("\\"))
	if os.path.exists(dest):
		os.remove(dest)


def activate(item_id: str, invfile: str, choice: str):
	"""choice = 'original' or an alt_id. Writes/removes the overlay DC6 and updates manifest."""
	m = _load_manifest()
	rel = item_dc6_path(invfile)
	owned = set(m.get("owned_overlay_files", []))
	entry = m["assets"].get(item_id, {})
	if choice == "original":
		_remove_overlay(rel)
		owned.discard(rel)
		entry.pop("active", None)
		entry.pop("invfile", None)
	else:
		_write_overlay(rel, alt_dc6_bytes(item_id, choice))
		owned.add(rel)
		entry["active"] = choice
		entry["invfile"] = invfile
	if entry:
		m["assets"][item_id] = entry
	else:
		m["assets"].pop(item_id, None)
	m["owned_overlay_files"] = sorted(owned)
	_save_manifest(m)
	return m


def activate_flippy(item_id: str, flippyfile: str, choice: str):
	"""Like activate(), for the item's animated ground-drop DC6 (flippyfile)."""
	m = _load_manifest()
	rel = item_dc6_path(flippyfile)  # flippies live in the same items\ dir
	owned = set(m.get("owned_overlay_files", []))
	entry = m["assets"].get(item_id, {})
	if choice == "original":
		_remove_overlay(rel)
		owned.discard(rel)
		entry.pop("flippy_active", None)
		entry.pop("flippyfile", None)
	else:
		_write_overlay(rel, flippy_alt_dc6_bytes(item_id, choice))
		owned.add(rel)
		entry["flippy_active"] = choice
		entry["flippyfile"] = flippyfile
	if entry:
		m["assets"][item_id] = entry
	else:
		m["assets"].pop(item_id, None)
	m["owned_overlay_files"] = sorted(owned)
	_save_manifest(m)
	return m


def active_choice(item_id: str) -> str:
	m = _load_manifest()
	return m.get("assets", {}).get(item_id, {}).get("active", "original")


def active_flippy_choice(item_id: str) -> str:
	m = _load_manifest()
	return m.get("assets", {}).get(item_id, {}).get("flippy_active", "original")


def list_flippy_alternates(item_id: str):
	d = os.path.join(alt_dir(item_id), "flippy")
	if not os.path.isdir(d):
		return []
	return sorted(n[:-4] for n in os.listdir(d) if n.endswith(".dc6"))


def save_flippy_alternate_dc6(item_id: str, alt_id: str, dc6_bytes: bytes):
	d = os.path.join(alt_dir(item_id), "flippy")
	os.makedirs(d, exist_ok=True)
	with open(os.path.join(d, alt_id + ".dc6"), "wb") as f:
		f.write(dc6_bytes)


def flippy_alt_dc6_bytes(item_id: str, alt_id: str) -> bytes:
	with open(os.path.join(alt_dir(item_id), "flippy", alt_id + ".dc6"), "rb") as f:
		return f.read()


EXPORT_DIR = os.path.join(WORKSPACE, "export")


def build_patch_mpq(out_path: str | None = None) -> tuple[str, int]:
	"""Author a patch.mpq from every file in the overlay tree. Returns (path, count).

	Writes to a fresh rotating filename (patch_<n>.mpq) so a copy the running game still
	holds open never blocks the rebuild; the game closes the old one when it registers
	the new. Stale, now-unlocked patch_*.mpq are pruned.
	"""
	files = {}
	for root, _dirs, names in os.walk(OVERLAY):
		for n in names:
			disk = os.path.join(root, n)
			arc = os.path.relpath(disk, OVERLAY).replace("/", "\\")
			files[arc] = disk
	os.makedirs(EXPORT_DIR, exist_ok=True)
	if out_path is None:
		# pick the next free patch_<n>.mpq
		n = 0
		while True:
			cand = os.path.join(EXPORT_DIR, f"patch_{n}.mpq")
			if not os.path.exists(cand):
				out_path = cand
				break
			try:  # an old one we can overwrite (not locked) is fine to reuse
				os.remove(cand)
				out_path = cand
				break
			except OSError:
				n += 1
	build_archive(out_path, files)  # v1 + PKWARE (D2-compatible)
	write_autoload(out_path)
	# prune other now-unlocked patch_*.mpq (best-effort)
	for name in os.listdir(EXPORT_DIR):
		p = os.path.join(EXPORT_DIR, name)
		if name.startswith("patch_") and name.endswith(".mpq") and p != out_path:
			try:
				os.remove(p)
			except OSError:
				pass
	return out_path, len(files)


AUTOLOAD = os.path.join(WORKSPACE, "autoload.txt")


def write_autoload(mpq_path: str, priority: int = 9000):
	"""Point the D2Debugger early-registration hook at the freshest patch.mpq.

	The hook (DATATBLS_LoadAllTxts detour) reads this file at process startup and
	registers the archive BEFORE data tables load, so excel-bin edits (uniqueitems
	invfile etc.) take effect on a plain full reload.  Written on every build.
	"""
	tmp = AUTOLOAD + ".tmp"
	with open(tmp, "w", encoding="ascii") as f:
		f.write(os.path.abspath(mpq_path) + "\n" + str(int(priority)) + "\n")
	os.replace(tmp, AUTOLOAD)
