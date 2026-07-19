"""Compose a two-handed glove sprite from single-hand 3D models.

D2 shows gloves as a PAIR in one inventory sprite, but the re-imagined art library
splits them into single hands (`invtgl-l4.png` / `invtgl-r4.png`) because a 3D model
of one hand is what Meshy can actually generate. This module puts them back together.

Why a hand-tuned template rather than deriving placement from the original: the two
gloves in the original DC6 OVERLAP, so its alpha is a single merged blob — connected-
component analysis on all five glove sprites returns exactly one region each. There is
nothing to split, so each art file carries a saved layout instead (tuned once, reused
across every variant pair: l1+r1, l4+r4, lj2+rj2 ...).

Placement is stored in NORMALISED units so a template survives any change of cell size
or render resolution: `cx`/`cy` are the hand's centre as a fraction of the canvas,
`scale` is its longest side as a fraction of canvas width, `rot` is degrees clockwise.
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

# Seed layout: one hand up-left and behind, the other down-right and in front —
# the arrangement D2's glove sprites use. Only a starting point; tuning is per file.
DEFAULT_TEMPLATE = {
	"left":  {"cx": 0.37, "cy": 0.42, "scale": 0.52, "rot": -10},
	"right": {"cx": 0.62, "cy": 0.58, "scale": 0.52, "rot": 8},
	"front": "right",
}

_HAND_RE = re.compile(r"-(?P<hand>[lr])j?\d*$", re.IGNORECASE)


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


def is_split_art(art_file: str | None) -> bool:
	return hand_of(art_file) is not None


# ---- templates ------------------------------------------------------------

def load_templates() -> dict:
	try:
		with open(TEMPLATES_PATH, encoding="utf-8") as f:
			return json.load(f)
	except (FileNotFoundError, ValueError):
		return {}


def get_template(invfile: str) -> dict:
	t = load_templates().get((invfile or "").lower())
	if not t:
		return json.loads(json.dumps(DEFAULT_TEMPLATE))
	out = json.loads(json.dumps(DEFAULT_TEMPLATE))
	for hand in ("left", "right"):
		out[hand].update(t.get(hand) or {})
	out["front"] = t.get("front", out["front"])
	return out


def save_template(invfile: str, tpl: dict) -> dict:
	all_t = load_templates()
	clean = {}
	for hand in ("left", "right"):
		src = tpl.get(hand) or {}
		clean[hand] = {
			"cx": max(-0.5, min(1.5, float(src.get("cx", DEFAULT_TEMPLATE[hand]["cx"])))),
			"cy": max(-0.5, min(1.5, float(src.get("cy", DEFAULT_TEMPLATE[hand]["cy"])))),
			"scale": max(0.05, min(2.0, float(src.get("scale", DEFAULT_TEMPLATE[hand]["scale"])))),
			"rot": max(-180.0, min(180.0, float(src.get("rot", DEFAULT_TEMPLATE[hand]["rot"])))),
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
               mirror: bool = False) -> Image.Image:
	"""Scale/rotate/position one hand onto the canvas per its template entry."""
	img = hand_png if isinstance(hand_png, Image.Image) else Image.open(io.BytesIO(hand_png))
	img = drop_flat_background(img)
	img = _crop_to_content(img)
	if mirror:
		img = img.transpose(Image.FLIP_LEFT_RIGHT)
	W, H = canvas.size
	target = max(1, round(float(spec.get("scale", 0.6)) * W))
	k = target / max(img.width, img.height)
	img = img.resize((max(1, round(img.width * k)), max(1, round(img.height * k))), Image.LANCZOS)
	rot = float(spec.get("rot", 0.0))
	if rot:
		img = img.rotate(-rot, resample=Image.BICUBIC, expand=True)
	cx = float(spec.get("cx", 0.5)) * W
	cy = float(spec.get("cy", 0.5)) * H
	canvas.alpha_composite(img, (round(cx - img.width / 2), round(cy - img.height / 2)))
	return canvas


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
		place_hand(canvas, png, template.get(hand) or DEFAULT_TEMPLATE[hand], mirror=mir)
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
