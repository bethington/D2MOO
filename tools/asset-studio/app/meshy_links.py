"""Pair Meshy workspace tasks with catalog items.

Links persist in <workspace>/meshy_links.json ({task_id: {item_id, image_id, phase,
source, name, linked_at}}) so pairings survive server restarts — the server seeds its
in-memory _STUDIO map from here at boot and writes back through remember().

Auto-pairing (scan) uses two layers:
  1. IMAGE MATCH (authoritative): dHash the task's INPUT image against each item's
     sprite run through the same prep as upload (dc6 -> png -> prep_image_for_meshy).
     Studio-created tasks re-hash to ~0 distance; hand-uploaded exports of the same
     sprite land within a few bits. There is no server-side image-name listing on
     Meshy's web API (POST /v1/files/images is upload-only, GET 404s) so content
     hashing is the only exact signal.
  2. NAME MATCH (suggestions only): difflib ratio between Meshy's auto-name
     ("Diamond Shield") and catalog item names; never auto-applied.
"""
from __future__ import annotations

import difflib
import io
import json
import os
import time
import urllib.request

from PIL import Image

import app.assets as assets

LINKS_PATH = os.path.join(assets.WORKSPACE, "meshy_links.json")
HASH_CACHE_PATH = os.path.join(assets.WORKSPACE, "item_dhash_cache.json")
# dHash distance thresholds (of 64 bits). Calibrated against a real workspace scan
# 2026-07-19 with a side-by-side contact sheet: every d==0 pair was correct
# (Studio-uploaded sprite round-trips bit-for-bit), while d in 5..8 was mostly WRONG
# — small dark sprites with similar silhouettes collide (a gauntlet matched a skeleton
# key at d=8, a glove matched a potion at d=7). So only near-exact auto-links; the
# rest become reviewable suggestions with the input thumbnail shown.
AUTO_MAX_DISTANCE = 2
SUGGEST_MAX_DISTANCE = 16  # image candidates worth eyeballing
SUGGEST_MIN_RATIO = 0.55   # name-similarity floor
IMAGE_CANDIDATES = 5       # the review page shows sprites, so offer a wider net
NAME_CANDIDATES = 3


# ---- persistent link store ------------------------------------------------

def load_links() -> dict:
	try:
		with open(LINKS_PATH, encoding="utf-8") as f:
			return json.load(f)
	except (FileNotFoundError, ValueError):
		return {}


def save_links(links: dict) -> None:
	tmp = LINKS_PATH + ".tmp"
	with open(tmp, "w", encoding="utf-8") as f:
		json.dump(links, f, indent=1)
	os.replace(tmp, LINKS_PATH)


def remember(links: dict, task_id: str, *, item_id: str, image_id: str | None,
             phase: str, source: str, name: str = "") -> dict:
	links[task_id] = {"item_id": item_id, "image_id": image_id, "phase": phase,
	                  "source": source, "name": name,
	                  "linked_at": time.strftime("%Y-%m-%dT%H:%M:%S")}
	save_links(links)
	return links[task_id]


def forget(links: dict, task_id: str) -> bool:
	if task_id in links:
		del links[task_id]
		save_links(links)
		return True
	return False


# ---- perceptual hash ------------------------------------------------------

def dhash(png_bytes: bytes) -> int:
	"""64-bit difference hash: grayscale 9x8, each bit = left pixel > right pixel."""
	im = Image.open(io.BytesIO(png_bytes)).convert("L").resize((9, 8), Image.LANCZOS)
	px = list(im.getdata())
	h = 0
	for row in range(8):
		for col in range(8):
			h = (h << 1) | (1 if px[row * 9 + col] > px[row * 9 + col + 1] else 0)
	return h


def hamming(a: int, b: int) -> int:
	return bin(a ^ b).count("1")


def _load_hash_cache() -> dict:
	try:
		with open(HASH_CACHE_PATH, encoding="utf-8") as f:
			return json.load(f)
	except (FileNotFoundError, ValueError):
		return {}


def item_hashes(items: list, progress=None) -> dict:
	"""{item_id: dhash int} over the catalog, built lazily and cached to disk.
	Hashes the item sprite through the SAME prep used when the Studio uploads to
	Meshy, so a Studio-created task's input image re-hashes to (near) zero distance."""
	cache = _load_hash_cache()
	out, dirty, done = {}, False, 0
	for it in items:
		iid = it["id"]
		cached = cache.get(iid)
		if cached is not None:
			out[iid] = int(cached, 16) if isinstance(cached, str) else int(cached)
			continue
		try:
			png = assets.dc6_to_png_bytes(assets.read_original_dc6(it["invfile"]))
			h = dhash(assets.prep_image_for_meshy(png))
			out[iid] = h
			cache[iid] = f"{h:016x}"
			dirty = True
		except Exception:  # noqa: BLE001 -- unreadable invfile: skip, retry next scan
			continue
		done += 1
		if progress and done % 100 == 0:
			progress(done)
	if dirty:
		tmp = HASH_CACHE_PATH + ".tmp"
		with open(tmp, "w", encoding="utf-8") as f:
			json.dump(cache, f)
		os.replace(tmp, HASH_CACHE_PATH)
	return out


# ---- auto-pair ------------------------------------------------------------

def _task_input_url(task: dict) -> str | None:
	d = (task.get("args") or {}).get("draft") or {}
	return d.get("imageUrl") or (d.get("imageUrls") or [None])[0]


def _task_image_id(task: dict) -> str | None:
	d = (task.get("args") or {}).get("draft") or {}
	return d.get("imageId") or (d.get("imageIds") or [None])[0]


def _download(url: str, timeout: float = 30.0) -> bytes:
	with urllib.request.urlopen(url, timeout=timeout) as r:
		return r.read()


def is_high_confidence(source: str) -> bool:
	"""Was this link made on evidence, or on a blind guess? Near-exact image matches,
	Studio-created links, and human review count; anything else (notably the legacy
	`fuzzy-confirmed` picks made before candidates showed sprites) gets re-offered."""
	s = (source or "").strip()
	if s.startswith("auto-image d"):
		try:
			return int(s.rsplit("d", 1)[1]) <= AUTO_MAX_DISTANCE
		except ValueError:
			return False
	return s.startswith(("manual", "reviewed", "studio-"))


def auto_pair(tasks: list, items: list, links: dict, progress=None,
              review_low_confidence: bool = True) -> dict:
	"""Pair unlinked root drafts. Returns {auto, suggestions, unmatched, errors};
	`auto` entries are ALREADY written into `links` (source auto-image).

	With `review_low_confidence`, tasks already linked on weak evidence are re-offered
	as suggestions carrying `current_item_id`, so a blind guess can be corrected."""
	hashes = item_hashes(items, progress=progress)
	by_id = {it["id"]: it for it in items}
	auto, suggestions, unmatched, errors = [], [], [], []
	names = [(it["id"], it["name"]) for it in items]

	for t in tasks:
		tid = t.get("id")
		if not tid:
			continue
		existing = links.get(tid)
		if existing and not (review_low_confidence
		                     and not is_high_confidence(existing.get("source", ""))):
			continue
		if t.get("phase") not in ("generate", "draft"):
			continue  # texture/etc. follow their root's link
		url = _task_input_url(t)
		ranked = []
		if url:
			try:
				h = dhash(_download(url))
				ranked = sorted(((hamming(h, ih), iid) for iid, ih in hashes.items()))[:IMAGE_CANDIDATES]
			except Exception as e:  # noqa: BLE001 -- signed URL expired / decode fail
				errors.append({"task_id": tid, "error": str(e)[:120]})
		if ranked and ranked[0][0] <= AUTO_MAX_DISTANCE and not existing:
			best_d, best_item = ranked[0]
			remember(links, tid, item_id=best_item, image_id=_task_image_id(t),
			         phase=t.get("phase") or "draft", source=f"auto-image d{best_d}",
			         name=t.get("name") or "")
			auto.append({"task_id": tid, "task_name": t.get("name") or "",
			             "item_id": best_item, "item_name": by_id[best_item]["name"],
			             "distance": best_d})
			continue

		# Not near-exact: offer candidates to eyeball. Image-similarity first (it beats
		# name matching when Meshy's auto-name is a description, e.g. the Plate Mail
		# sprite it named "Chainmail hauberk"), then name similarity.
		cand, seen = [], set()
		for d, iid in ranked:
			if d <= SUGGEST_MAX_DISTANCE and iid not in seen:
				seen.add(iid)
				cand.append({"item_id": iid, "item_name": by_id[iid]["name"],
				             "why": f"image d{d}", "rank": d})
		tname = (t.get("name") or "").strip()
		if tname:
			scored = sorted(((difflib.SequenceMatcher(None, tname.lower(), n.lower()).ratio(), iid, n)
			                 for iid, n in names), reverse=True)
			for s, iid, n in scored[:NAME_CANDIDATES]:
				if s >= SUGGEST_MIN_RATIO and iid not in seen:
					seen.add(iid)
					cand.append({"item_id": iid, "item_name": n,
					             "why": f"name {int(s * 100)}%", "rank": 100 - s})
		cand.sort(key=lambda c: c["rank"])
		cur_id = (existing or {}).get("item_id")
		if cur_id and cur_id not in seen and cur_id in by_id:
			cand.insert(0, {"item_id": cur_id, "item_name": by_id[cur_id]["name"],
			                "why": "current pick", "rank": -1})
		row = {"task_id": tid, "task_name": tname,
		       "preview": ((t.get("result") or {}).get("previewUrl") or ""),
		       "input_image": url or "",
		       "best_distance": (ranked[0][0] if ranked else None),
		       "current_item_id": cur_id,
		       "current_item_name": (by_id[cur_id]["name"] if cur_id in by_id else None),
		       "current_source": (existing or {}).get("source"),
		       "candidates": cand}
		# A task with a weak existing link always needs review, even with no candidates.
		(suggestions if (cand or cur_id) else unmatched).append(row)
	return {"auto": auto, "suggestions": suggestions, "unmatched": unmatched, "errors": errors}
