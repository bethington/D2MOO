"""Compose a two-handed glove sprite from single-hand 3D models.

D2 shows gloves as a PAIR in one inventory sprite, but the re-imagined art library
splits them into single hands (`invtgl-l4.png` / `invtgl-r4.png`) because a 3D model
of one hand is what Meshy can actually generate. This module puts them back together.

Why a hand-tuned template rather than deriving placement from the original: the two
gloves in the original DC6 OVERLAP, so its alpha is a single merged blob — connected-
component analysis on all five glove sprites returns exactly one region each. There is
nothing to split, so each art file carries a saved layout instead (tuned once, reused
across every variant pair: l1+r1, l4+r4, lj2+rj2 ...).

Placement is stored as NEUTRAL-BASED adjustments so every control has an obvious zero:
`dx`/`dy` are offsets from the hand's default anchor (0 = where it naturally sits),
`scale` is a multiplier on the default fit (1 = natural size), `rot` is degrees
clockwise (0 = unrotated). Units are fractions of the canvas, so a template survives
any change of cell size or render resolution.
"""
from __future__ import annotations

import io
import json
import os
import re

from PIL import Image

import app.assets as assets
from pyd2 import dc6

TEMPLATES_PATH = os.path.join(assets.WORKSPACE, "pair_templates.json")

# Where each hand sits with everything at neutral: side by side, vertically centred.
# Tuning moves a hand RELATIVE to this, so dx=dy=0 / scale=1 / rot=0 is a sane sprite
# on its own rather than both hands stacked in the middle.
BASE_ANCHOR = {"left": (0.34, 0.50), "right": (0.66, 0.50)}
BASE_FIT = 0.52          # hand's longest side as a fraction of canvas width at scale 1

NEUTRAL_HAND = {"dx": 0.0, "dy": 0.0, "scale": 1.0, "rot": 0.0}
DEFAULT_TEMPLATE = {
	"left":  dict(NEUTRAL_HAND),
	"right": dict(NEUTRAL_HAND),
	"front": "right",
}

# `-l4` = LEFT hand, variant 4.  `-rj2` = RIGHT hand, variant j2 (the `j` series is a
# second run of redraws).  `-L` / `-R` are the original unnumbered pair.
_HAND_RE = re.compile(r"-(?P<hand>[lr])(?P<series>j?)(?P<num>\d*)$", re.IGNORECASE)


def hand_of(art_file: str | None) -> str | None:
	"""'invtgl-l4.png' -> 'left'; 'invtgl-rj2.png' -> 'right'; 'invtgl.png' -> None.

	The library's own filenames carry the hand, so this never has to be inferred from
	the picture. The `j` series (lj1..lj4 / rj1..rj4) is a second set of redraws.
	"""
	if not art_file:
		return None
	stem = os.path.splitext(os.path.basename(art_file))[0]
	m = _HAND_RE.search(stem)
	if not m:
		return None
	return "left" if m.group("hand").lower() == "l" else "right"


def variant_of(art_file: str | None) -> str | None:
	"""'invtgl-l4.png' -> '4';  'invtgl-rj2.png' -> 'j2';  'invlgl-L.png' -> ''.

	The number is the VARIANT, and it is what makes two files a set: `-l4` and `-r4`
	are the same redraw's two hands and belong together. Pairing across variants
	(l4 with r7) would put two different designs on one pair of hands.
	"""
	if not art_file:
		return None
	stem = os.path.splitext(os.path.basename(art_file))[0]
	m = _HAND_RE.search(stem)
	if not m:
		return None
	return (m.group("series") or "").lower() + (m.group("num") or "")


def variant_label(v: str | None) -> str:
	if v is None:
		return "?"
	return f"variant {v}" if v else "base pair"


def is_split_art(art_file: str | None) -> bool:
	return hand_of(art_file) is not None


# ---- templates ------------------------------------------------------------

def load_templates() -> dict:
	try:
		with open(TEMPLATES_PATH, encoding="utf-8") as f:
			return json.load(f)
	except (FileNotFoundError, ValueError):
		return {}


def _from_legacy(hand: str, src: dict) -> dict:
	"""Convert a pre-2026-07-19 absolute entry (cx/cy/absolute scale) to the neutral
	form, so templates saved before the controls were re-zeroed still load."""
	ax, ay = BASE_ANCHOR[hand]
	return {
		"dx": float(src.get("cx", ax)) - ax,
		"dy": float(src.get("cy", ay)) - ay,
		"scale": float(src.get("scale", BASE_FIT)) / BASE_FIT,
		"rot": float(src.get("rot", 0.0)),
	}


def get_template(invfile: str) -> dict:
	t = load_templates().get((invfile or "").lower())
	out = json.loads(json.dumps(DEFAULT_TEMPLATE))
	if not t:
		return out
	for hand in ("left", "right"):
		src = t.get(hand) or {}
		if "cx" in src or "cy" in src:
			out[hand].update(_from_legacy(hand, src))
		else:
			out[hand].update({k: float(v) for k, v in src.items()
			                  if k in ("dx", "dy", "scale", "rot")})
	out["front"] = t.get("front", out["front"])
	return out


def save_template(invfile: str, tpl: dict) -> dict:
	all_t = load_templates()
	clean = {}
	for hand in ("left", "right"):
		src = tpl.get(hand) or {}
		if "cx" in src or "cy" in src:
			src = _from_legacy(hand, src)
		clean[hand] = {
			"dx": max(-1.0, min(1.0, float(src.get("dx", 0.0)))),
			"dy": max(-1.0, min(1.0, float(src.get("dy", 0.0)))),
			"scale": max(0.1, min(3.0, float(src.get("scale", 1.0)))),
			"rot": max(-180.0, min(180.0, float(src.get("rot", 0.0)))),
		}
	clean["front"] = "left" if tpl.get("front") == "left" else "right"
	all_t[(invfile or "").lower()] = clean
	tmp = TEMPLATES_PATH + ".tmp"
	with open(tmp, "w", encoding="utf-8") as f:
		json.dump(all_t, f, indent=1)
	os.replace(tmp, TEMPLATES_PATH)
	return clean


# ---- image helpers --------------------------------------------------------

def drop_flat_background(img: Image.Image, thresh: int = 26) -> Image.Image:
	"""The single-hand source art is 1254px with an OPAQUE near-black background (it is
	AI output, not a game sprite), so it needs a cutout before it can be layered. Blender
	renders already arrive transparent and pass through untouched."""
	img = img.convert("RGBA")
	if img.split()[-1].getextrema()[0] < 250:
		return img  # already has real transparency
	px = img.load()
	w, h = img.size
	for y in range(h):
		for x in range(w):
			r, g, b, _a = px[x, y]
			if r < thresh and g < thresh and b < thresh:
				px[x, y] = (r, g, b, 0)
	return img


def _crop_to_content(img: Image.Image) -> Image.Image:
	bb = img.split()[-1].getbbox()
	return img.crop(bb) if bb else img


def place_hand(canvas: Image.Image, hand_png: bytes | Image.Image, spec: dict,
               mirror: bool = False, hand: str = "left") -> Image.Image:
	"""Scale/rotate/position one hand onto the canvas per its template entry.

	`spec` is neutral-based: scale multiplies BASE_FIT, dx/dy offset BASE_ANCHOR, so
	{dx:0, dy:0, scale:1, rot:0} places the hand at its natural spot and size.
	"""
	img = hand_png if isinstance(hand_png, Image.Image) else Image.open(io.BytesIO(hand_png))
	img = drop_flat_background(img)
	img = _crop_to_content(img)
	if mirror:
		img = img.transpose(Image.FLIP_LEFT_RIGHT)
	W, H = canvas.size
	target = max(1, round(BASE_FIT * float(spec.get("scale", 1.0)) * W))
	k = target / max(img.width, img.height)
	img = img.resize((max(1, round(img.width * k)), max(1, round(img.height * k))), Image.LANCZOS)
	rot = float(spec.get("rot", 0.0))
	if rot:
		img = img.rotate(-rot, resample=Image.BICUBIC, expand=True)
	ax, ay = BASE_ANCHOR.get(hand, (0.5, 0.5))
	cx = (ax + float(spec.get("dx", 0.0))) * W
	cy = (ay + float(spec.get("dy", 0.0))) * H
	canvas.alpha_composite(img, (round(cx - img.width / 2), round(cy - img.height / 2)))
	return canvas


_OUTLINE_CACHE = {}


def silhouette_points(img: Image.Image, samples: int = 48) -> list:
	"""Sample the opaque outline as normalised points in the image's own 0..1 box.

	Per column we take the topmost and bottommost opaque pixel, which traces the real
	shape closely enough to clamp against while staying tiny to ship and cheap to
	rotate on every drag frame (vs. testing thousands of pixels).
	"""
	img = img.convert("RGBA")
	bb = img.split()[-1].getbbox()
	if not bb:
		return []
	img = img.crop(bb)
	w, h = img.size
	small = img.resize((min(samples, w), min(samples, h)), Image.NEAREST)
	sw, sh = small.size
	a = small.split()[-1].load()
	pts = []
	for x in range(sw):
		col = [y for y in range(sh) if a[x, y] > 8]
		if not col:
			continue
		for y in (col[0], col[-1]):
			pts.append([round(x / max(1, sw - 1), 4), round(y / max(1, sh - 1), 4)])
	return pts


def outline_for(path: str) -> list:
	if path not in _OUTLINE_CACHE:
		try:
			with open(path, "rb") as f:
				img = drop_flat_background(Image.open(io.BytesIO(f.read())))
			_OUTLINE_CACHE[path] = silhouette_points(img)
		except Exception:  # noqa: BLE001
			_OUTLINE_CACHE[path] = []
	return _OUTLINE_CACHE[path]


def aspect_for(path: str) -> float:
	"""Cropped content aspect (w/h) -- the client needs it to size the outline box."""
	try:
		with open(path, "rb") as f:
			img = drop_flat_background(Image.open(io.BytesIO(f.read())))
		bb = img.split()[-1].getbbox()
		if not bb:
			return 1.0
		return (bb[2] - bb[0]) / max(1, (bb[3] - bb[1]))
	except Exception:  # noqa: BLE001
		return 1.0


def original_size(invfile: str) -> tuple[int, int] | None:
	"""Pixel size of the game's own sprite, so a replacement matches it exactly."""
	try:
		png = assets.dc6_to_png_bytes(assets.read_original_dc6(invfile))
		im = Image.open(io.BytesIO(png))
		return im.size
	except Exception:  # noqa: BLE001
		return None


def composite(left_png, right_png, template: dict, invwidth: int, invheight: int,
              mirror_left: bool = False, mirror_right: bool = False,
              size: tuple[int, int] | None = None) -> Image.Image:
	"""Build the paired sprite.

	Sized to the ORIGINAL sprite's pixel dimensions when known (invtgl is 56x56, not the
	2*29=58 the cell grid implies) so the tuning ghost overlays the result 1:1 and the
	replacement drops in at the same scale as every other item. Falls back to the cell
	grid for art no original could be read for.

	A missing hand is mirrored from the other so the result is ALWAYS a pair — the game
	never shows a one-handed glove.
	"""
	if size:
		W, H = size
	else:
		W = max(1, invwidth) * assets.CELL_PX
		H = max(1, invheight) * assets.CELL_PX
	canvas = Image.new("RGBA", (W, H), (0, 0, 0, 0))
	front = template.get("front", "right")
	order = ["left", "right"] if front == "right" else ["right", "left"]
	src = {"left": (left_png, mirror_left), "right": (right_png, mirror_right)}
	for hand in order:  # back first, front last
		png, mir = src[hand]
		if png is None:
			continue
		place_hand(canvas, png, template.get(hand) or DEFAULT_TEMPLATE[hand],
		           mirror=mir, hand=hand)
	return canvas


def canvas_to_dc6(canvas: Image.Image) -> bytes:
	"""Encode an already-positioned canvas straight to DC6.

	Deliberately NOT assets.png_to_item_dc6: that crops to the alpha bbox and re-fills
	the cell, which would undo the placement this whole module exists to get right.
	"""
	rows = assets._quantize_to_palette(canvas, assets._palette())
	frame = dc6.Dc6Frame(flip=0, width=canvas.width, height=canvas.height,
	                     offset_x=0, offset_y=0, pixels=rows)
	sprite = dc6.Dc6File(directions=1, frames_per_direction=1,
	                     termination=b"\xee\xee\xee\xee", frames=[frame])
	return dc6.encode(sprite)
