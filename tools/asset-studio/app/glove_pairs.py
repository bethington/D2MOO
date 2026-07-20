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
import math
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


# ---- auto-fit -------------------------------------------------------------
#
# A glove's OUTER (pinky) edge is a long straight run -- measured across the real
# library it holds for 57-73% of the glove's height, and it mirrors cleanly between
# hands (invlgl-l2: right edge 73% at +27.2 deg; invlgl-r2: left edge 73% at -27.2).
# So: rotate that edge parallel to the border, grow to fill the height, snap to the
# side. Left hand goes to the RIGHT border, right hand to the LEFT.

STRAIGHT_TOL_PX = 2.0        # rms deviation allowed when calling a run "straight"
EDGE_SAMPLE_ROWS = 200       # working height for edge tracing

# Measured over all 68 single-hand glove files: edge detection alone agrees with the
# filename only 60/68 (88%). Every disagreement is a SYMMETRIC MIRROR PAIR
# (invtgl-l7/r7, lj2/rj2, lj3/rj3, invvgl-l4/r4) at a margin of 0.05-0.08 -- the
# filenames are self-consistent and the detector is what flips, on designs like the
# clawed invtgl whose cuff makes both edges similarly straight. So the filename
# decides, and detection only raises a flag when it disagrees DECISIVELY. Every real
# disagreement measured so far is below 0.10, so 0.15 gives no false alarms while
# still catching genuinely mirrored art.
DECISIVE_MARGIN = 0.15


def _outer_edge_points(img: Image.Image, side: str) -> list:
	"""Outermost opaque pixel per row on `side` ('L' or 'R')."""
	a = img.split()[-1].load()
	w, h = img.size
	pts = []
	for y in range(h):
		rng = range(w) if side == "L" else range(w - 1, -1, -1)
		for x in rng:
			if a[x, y] > 8:
				pts.append((float(x), float(y)))
				break
	return pts


def longest_straight_run(pts: list, tol: float = STRAIGHT_TOL_PX) -> dict:
	"""Longest contiguous stretch of edge points fitting a line within `tol` rms.

	Deliberately a longest-RUN fit rather than two extreme points or a convex hull:
	a cuff flare, a thumb or a claw tip simply isn't part of the longest straight
	stretch, so it cannot drag the angle off.
	"""
	n = len(pts)
	best = {"count": 0, "tilt": 0.0, "rms": 999.0, "frac": 0.0}
	if n < 10:
		return best
	for start in range(0, n - 9):
		for end in range(n - 1, start + 8, -1):
			if (end - start + 1) <= best["count"]:
				break
			seg = pts[start:end + 1]
			mx = sum(q[0] for q in seg) / len(seg)
			my = sum(q[1] for q in seg) / len(seg)
			syy = sum((q[1] - my) ** 2 for q in seg)
			if syy == 0:
				continue
			slope = sum((q[0] - mx) * (q[1] - my) for q in seg) / syy   # dx per dy
			rms = math.sqrt(sum(((q[0] - mx) - slope * (q[1] - my)) ** 2 for q in seg) / len(seg))
			if rms <= tol:
				best = {"count": len(seg), "tilt": math.degrees(math.atan(slope)),
				        "rms": rms, "frac": len(seg) / n}
				break          # longest fitting run from this start; move to the next
			# too curved -- shrink the segment from the far end and retry
	return best


def analyse_hand_art(path: str, hand: str) -> dict:
	"""Measure both outer edges and decide the pinky side.

	The filename decides (left hand -> right edge, right hand -> left edge) and the
	measurement cross-checks it. Disagreement means the art is probably mirrored or
	mislabelled, so it is reported rather than silently fitted backwards.
	"""
	img = drop_flat_background(Image.open(path))
	bb = img.split()[-1].getbbox()
	if bb:
		img = img.crop(bb)
	if img.height > EDGE_SAMPLE_ROWS:
		img = img.resize((max(1, round(img.width * EDGE_SAMPLE_ROWS / img.height)),
		                  EDGE_SAMPLE_ROWS), Image.LANCZOS)
	runs = {side: longest_straight_run(_outer_edge_points(img, side)) for side in ("L", "R")}
	expected = "R" if hand == "left" else "L"        # pinky faces the border it snaps to
	measured = "L" if runs["L"]["frac"] >= runs["R"]["frac"] else "R"
	margin = abs(runs["L"]["frac"] - runs["R"]["frac"])
	return {
		"expected_side": expected,
		"measured_side": measured,
		"agrees": expected == measured,
		"margin": round(margin, 3),
		# only a confident contradiction is worth interrupting for
		"suspect_mirrored": (expected != measured) and margin >= DECISIVE_MARGIN,
		"tilt": runs[expected]["tilt"],
		"run_frac": runs[expected]["frac"],
		"rms": runs[expected]["rms"],
		"runs": {k: {"frac": round(v["frac"], 3), "tilt": round(v["tilt"], 1)}
		         for k, v in runs.items()},
	}


def autofit_hand(path: str, hand: str, out_w: int, out_h: int) -> dict:
	"""Template entry that stands the pinky edge parallel to its border, fills the
	height and snaps to the side. Returns the entry plus the diagnosis."""
	info = analyse_hand_art(path, hand)
	# Rotate so the pinky edge becomes vertical. `tilt` is dx-per-dy in degrees, and
	# place_hand rotates by -rot, so negating aligns the edge with the border.
	rot = -info["tilt"]

	img = drop_flat_background(Image.open(path))
	bb = img.split()[-1].getbbox()
	if bb:
		img = img.crop(bb)
	rotated = img.rotate(-rot, resample=Image.BICUBIC, expand=True)
	rb = rotated.split()[-1].getbbox() or (0, 0, rotated.width, rotated.height)
	rot_w, rot_h = rb[2] - rb[0], rb[3] - rb[1]

	eps_y = 1.0 / max(8, out_h)
	eps_x = 1.0 / max(8, out_w)
	# fill the height: silhouette height == canvas height less the safety inset
	target_h_frac = 1.0 - 2 * eps_y
	# place_hand sizes by the LONGEST side of the UNROTATED art
	longest_unrot = max(img.width, img.height)
	px_per_unit = rot_h / longest_unrot            # rotated height per unit of longest side
	scale = (target_h_frac * out_h) / (px_per_unit * BASE_FIT * out_w)

	# snap: left hand to the RIGHT border, right hand to the LEFT
	half_w_frac = (rot_w / longest_unrot) * BASE_FIT * scale / 2
	ax, ay = BASE_ANCHOR[hand]
	cx = (1.0 - eps_x - half_w_frac) if hand == "left" else (eps_x + half_w_frac)
	entry = {"dx": round(cx - ax, 4), "dy": round(0.5 - ay, 4),
	         "scale": round(scale, 4), "rot": round(rot, 2)}
	return {"entry": entry, "info": info}


def autofit_template(left_path: str | None, right_path: str | None,
                     out_w: int, out_h: int) -> dict:
	"""Auto-fit both hands. A missing hand mirrors the other, so it is fitted from the
	same art with the opposite snap."""
	tpl = json.loads(json.dumps(DEFAULT_TEMPLATE))
	notes = {}
	for hand, path in (("left", left_path), ("right", right_path)):
		src = path or (right_path if hand == "left" else left_path)
		if not src or not os.path.exists(src):
			continue
		try:
			r = autofit_hand(src, hand, out_w, out_h)
		except Exception as e:  # noqa: BLE001
			notes[hand] = {"error": str(e)[:120]}
			continue
		tpl[hand] = r["entry"]
		notes[hand] = r["info"]
	tpl["front"] = "right"
	return {"template": tpl, "notes": notes}


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
