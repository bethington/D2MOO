"""PD2 Asset Studio — Flask app.

Browse PD2 items, import PNG alternates for their inventory art, convert to DC6,
build a patch.mpq overlay, and push it live to the running game via the D2Debugger
AssetReload endpoint (:8790).  Run:  python app/server.py   (http://127.0.0.1:5001)
"""

from __future__ import annotations

import json
import os
import sys
import urllib.request

from flask import Flask, Response, jsonify, request, send_from_directory

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
import io  # noqa: E402

from PIL import Image  # noqa: E402

import app.assets as assets  # noqa: E402
import app.blender as blender  # noqa: E402
import app.excel as excel  # noqa: E402
import app.meshy_links as meshy_links  # noqa: E402
import app.meshy_web as meshy_web  # noqa: E402
from app.catalog import build_catalog  # noqa: E402

MESHY_CACHE = os.path.join(assets.WORKSPACE, "meshy_cache")

D2DBG = "http://127.0.0.1:8790"
STATIC = os.path.join(os.path.dirname(__file__), "static")

flask_app = Flask(__name__, static_folder=None)

_CATALOG = {"items": None, "by_id": {}}


def catalog():
	if _CATALOG["items"] is None:
		items, _by_code = build_catalog()
		# apply live uniqueitems.bin edits (own invfile/flippyfile) over the txt-derived view
		over = excel.unique_overrides()
		for it in items:
			if it["category"] == "unique" and it["name"] in over:
				o = over[it["name"]]
				if o.get("invfile"):
					it["invfile"] = o["invfile"]
				if o.get("flippyfile"):
					it["flippyfile"] = o["flippyfile"]
		_CATALOG["items"] = items
		_CATALOG["by_id"] = {it["id"]: it for it in items}
	return _CATALOG


def invalidate_catalog():
	_CATALOG["items"] = None
	_CATALOG["by_id"] = {}


def _dbg(method: str, path: str, body: dict | None = None, timeout: float = 12.0):
	url = D2DBG + path
	data = json.dumps(body).encode() if body is not None else None
	req = urllib.request.Request(url, data=data, method=method,
	                             headers={"Content-Type": "application/json"})
	try:
		with urllib.request.urlopen(req, timeout=timeout) as r:
			return json.loads(r.read().decode()), None
	except Exception as e:  # noqa: BLE001
		return None, str(e)


@flask_app.get("/")
def index():
	return send_from_directory(STATIC, "index.html")


@flask_app.get("/static/<path:p>")
def static_files(p):
	return send_from_directory(STATIC, p)


@flask_app.get("/api/items")
def api_items():
	c = catalog()
	q = (request.args.get("q") or "").lower()
	cat = request.args.get("category") or ""
	out = []
	for it in c["items"]:
		if q and q not in it["name"].lower() and q not in it["code"].lower():
			continue
		if cat and it["category"] != cat:
			continue
		out.append({
			"id": it["id"], "name": it["name"], "code": it["code"],
			"category": it["category"], "invfile": it["invfile"],
			"invwidth": it["invwidth"], "invheight": it["invheight"],
			"invtransform": it["invtransform"],
			"active": assets.active_choice(it["id"]),
			"alts": assets.list_alternates(it["id"]),
			"flippyfile": it["flippyfile"],
			"flippy_active": assets.active_flippy_choice(it["id"]),
			"flippy_alts": assets.list_flippy_alternates(it["id"]),
		})
	out.sort(key=lambda x: (x["category"], x["name"]))
	return jsonify({"count": len(out), "items": out[:1500]})


def _item(item_id):
	return catalog()["by_id"].get(item_id)


@flask_app.get("/api/item/<path:item_id>/original.png")
def api_original_png(item_id):
	it = _item(item_id)
	if not it:
		return "no such item", 404
	try:
		png = assets.dc6_to_png_bytes(assets.read_original_dc6(it["invfile"]))
	except Exception as e:  # noqa: BLE001
		return f"render error: {e}", 500
	return Response(png, mimetype="image/png")


@flask_app.get("/api/item/<path:item_id>/alt/<alt_id>.png")
def api_alt_png(item_id, alt_id):
	try:
		png = assets.dc6_to_png_bytes(assets.alt_dc6_bytes(item_id, alt_id))
	except Exception as e:  # noqa: BLE001
		return f"render error: {e}", 500
	return Response(png, mimetype="image/png")


@flask_app.post("/api/item/<path:item_id>/import")
def api_import(item_id):
	it = _item(item_id)
	if not it:
		return "no such item", 404
	f = request.files.get("file")
	if not f:
		return jsonify({"ok": False, "error": "no file"}), 400
	alt_id = request.form.get("alt_id") or f"png-{len(assets.list_alternates(item_id)) + 1}"
	try:
		raw = f.read()
		dc6_bytes = assets.png_to_item_dc6(raw, it["invwidth"], it["invheight"])
		assets.save_alternate_dc6(item_id, alt_id, dc6_bytes)
		# keep the source PNG as the alt's render so the framing sliders work on imports too
		assets.save_alt_provenance(item_id, alt_id, render_png=raw,
		                           meta={"source": "png-import", "cell": [it["invwidth"], it["invheight"]],
		                                 "fill": 0.94, "dx": 0.0, "dy": 0.0})
	except Exception as e:  # noqa: BLE001
		return jsonify({"ok": False, "error": str(e)}), 500
	return jsonify({"ok": True, "alt_id": alt_id, "alts": assets.list_alternates(item_id)})


@flask_app.post("/api/item/<path:item_id>/activate")
def api_activate(item_id):
	it = _item(item_id)
	if not it:
		return "no such item", 404
	choice = (request.json or {}).get("choice", "original")
	assets.activate(item_id, it["invfile"], choice)
	return jsonify({"ok": True, "active": choice})


# ---- uniqueitems.bin cell edits (the txt sliver, plan §7.3) -------------

@flask_app.get("/api/item/<path:item_id>/txt")
def api_txt_get(item_id):
	it = _item(item_id)
	if not it:
		return "no such item", 404
	if it["category"] != "unique":
		return jsonify({"ok": False, "error": "bin edits are supported for uniques only"}), 400
	u = excel.get_unique(it["name"])
	if not u:
		return jsonify({"ok": False, "error": f"{it['name']!r} not found in uniqueitems.bin"}), 404
	u.update({"ok": True, "effective_invfile": it["invfile"],
	          "effective_flippyfile": it["flippyfile"]})
	return jsonify(u)


@flask_app.post("/api/item/<path:item_id>/txt")
def api_txt_set(item_id):
	"""Give a unique its own invfile/flippyfile (or '' to revert to inherited).

	Patches the cell in uniqueitems.bin (overlay copy), then keeps the art overlay
	coherent: an active alternate is re-written under the new filename, and if no art
	exists at a brand-new filename yet the current effective art is seeded there so
	the game never dangles on a missing DC6.
	"""
	it = _item(item_id)
	if not it:
		return "no such item", 404
	if it["category"] != "unique":
		return jsonify({"ok": False, "error": "bin edits are supported for uniques only"}), 400
	body = request.json or {}
	fld = body.get("field", "invfile")
	value = (body.get("value") or "").strip()
	old_file = it["invfile"] if fld == "invfile" else it["flippyfile"]
	try:
		u = excel.set_unique_field(it["name"], fld, value)
	except (ValueError, KeyError) as e:
		return jsonify({"ok": False, "error": str(e)}), 400
	new_file = u[fld] if value else u[f"stock_{fld}"]
	# resolve the inherited fallback when the stock cell is blank
	if not new_file:
		invalidate_catalog()
		new_file = _item(item_id)[fld] if fld == "invfile" else _item(item_id)["flippyfile"]
	seeded = False
	if fld == "invfile" and new_file != old_file:
		choice = assets.active_choice(item_id)
		if choice != "original":
			assets.activate(item_id, old_file, "original")
			if value or choice != "seed-original":
				# move the active alternate's overlay DC6 to the new filename;
				# an auto-seed is dropped on revert (it only existed for the own-file)
				assets.activate(item_id, new_file, choice)
		elif value and assets.try_read_effective(assets.item_dc6_path(new_file)) is None:
			# brand-new filename with no art anywhere: seed it with the current art
			assets.save_alternate_dc6(item_id, "seed-original", assets.read_original_dc6(old_file))
			assets.activate(item_id, new_file, "seed-original")
			seeded = True
	if fld == "flippyfile" and new_file != old_file:
		choice = assets.active_flippy_choice(item_id)
		if choice != "original":
			assets.activate_flippy(item_id, old_file, "original")
			if value or choice != "seed-original":
				assets.activate_flippy(item_id, new_file, choice)
		elif value and assets.try_read_effective(assets.item_dc6_path(new_file)) is None:
			assets.save_flippy_alternate_dc6(item_id, "seed-original",
			                                 assets.read_original_dc6(old_file))
			assets.activate_flippy(item_id, new_file, "seed-original")
			seeded = True
	invalidate_catalog()
	u = excel.get_unique(it["name"])
	u.update({"ok": True, "seeded": seeded, "effective_invfile": _item(item_id)["invfile"],
	          "effective_flippyfile": _item(item_id)["flippyfile"],
	          "note": "push + Full reload for the change to reach the game "
	                  "(data tables load at process start)"})
	return jsonify(u)


# ---- flippy (animated ground-drop) alternates ---------------------------

@flask_app.get("/api/item/<path:item_id>/flippy/original.gif")
def api_flippy_original(item_id):
	it = _item(item_id)
	if not it or not it["flippyfile"]:
		return "no flippy", 404
	try:
		gif = assets.dc6_to_gif_bytes(assets.read_original_dc6(it["flippyfile"]))
	except Exception as e:  # noqa: BLE001
		return f"render error: {e}", 500
	return Response(gif, mimetype="image/gif")


@flask_app.get("/api/item/<path:item_id>/flippy/alt/<alt_id>.gif")
def api_flippy_alt(item_id, alt_id):
	try:
		gif = assets.dc6_to_gif_bytes(assets.flippy_alt_dc6_bytes(item_id, alt_id))
	except Exception as e:  # noqa: BLE001
		return f"render error: {e}", 500
	return Response(gif, mimetype="image/gif")


@flask_app.post("/api/item/<path:item_id>/activate-flippy")
def api_activate_flippy(item_id):
	it = _item(item_id)
	if not it:
		return "no such item", 404
	if not it["flippyfile"]:
		return jsonify({"ok": False, "error": "item has no flippyfile"}), 400
	choice = (request.json or {}).get("choice", "original")
	assets.activate_flippy(item_id, it["flippyfile"], choice)
	return jsonify({"ok": True, "flippy_active": choice})


@flask_app.post("/api/push")
def api_push():
	"""Build patch.mpq from the overlay and register it live at priority 9000.

	The running game keeps the archive open, so we CLOSE it first (if the game is up)
	to release the file lock before rebuilding, then re-register.
	"""
	_dbg("POST", "/asset/close", {"confirm": True}, timeout=8)  # release the lock if held (ok if game down)
	try:
		path, count = assets.build_patch_mpq()
	except PermissionError as e:  # file still locked
		return jsonify({"ok": False, "stage": "build",
		                "error": f"patch.mpq is locked (game holds it) -- {e}"}), 500
	except Exception as e:  # noqa: BLE001
		return jsonify({"ok": False, "stage": "build", "error": str(e)}), 500
	res, err = _dbg("POST", "/asset/register",
	                {"path": path.replace("\\", "/"), "priority": 9000, "confirm": True})
	if err:
		return jsonify({"ok": False, "stage": "register", "built": count,
		                "error": f"game not reachable on :8790 ({err})"}), 502
	return jsonify({"ok": True, "built": count, "mpq": path, "register": res})


import subprocess
import time

RELAUNCH_SCRIPT = os.path.join(os.path.dirname(__file__), "..", "scripts", "relaunch_pd2_direct.ps1")


def _drive_into_game(register_first: bool) -> tuple[bool, str]:
	"""Drive the menu to in-world: (optionally register overlay), single-player, launch.
	Returns (ok, note). Assumes the game is up (menu pump firing)."""
	if register_first:
		assets.build_patch_mpq()  # rebuild from overlay
		res, _ = _dbg("POST", "/asset/register",
		              {"path": os.path.join(assets.EXPORT_DIR, sorted(
			              f for f in os.listdir(assets.EXPORT_DIR) if f.endswith(".mpq"))[-1]).replace("\\", "/"),
		               "priority": 9000, "confirm": True})
	# reach character-select (single-player click; it may fault-but-advance)
	for _ in range(12):
		s, _e = _dbg("GET", "/status", timeout=3)
		if s and s.get("charListLoaded"):
			break
		_dbg("POST", "/action/main-menu-singleplayer", {"confirm": True}, timeout=6)
		time.sleep(5)
	else:
		return False, "never reached character-select"
	# launch
	r, e = _dbg("POST", "/action/launch-character", {"confirm": True}, timeout=15)
	if e:
		return False, f"launch failed: {e}"
	# wait for in-world
	base = None
	for _ in range(12):
		s, _e = _dbg("GET", "/status", timeout=3)
		if s:
			cc = s.get("captureCount", 0)
			if base is None:
				base = cc
			if cc > base + 40:
				return True, "in-world"
		time.sleep(3)
	return True, "launched (world-entry unconfirmed)"


@flask_app.post("/api/reload")
def api_reload():
	"""Soft reload: save-and-exit to menu, then drive back into the game. NOTE: item art
	already loaded this process may not fully refresh (D2CMP cell cache) -- use full-reload
	for guaranteed-clean art."""
	r1, e1 = _dbg("POST", "/action/exit-to-menu", {"confirm": True})
	if e1:
		return jsonify({"ok": False, "stage": "exit", "error": e1}), 502
	time.sleep(6)
	ok, note = _drive_into_game(register_first=False)
	return jsonify({"ok": ok, "note": note, "hint": "if art didn't change, use Full reload (fresh process)"})


@flask_app.post("/api/full-reload")
def api_full_reload():
	"""Guaranteed-clean reload: relaunch a FRESH game process (empty caches) via the
	elevated -direct launcher (1 UAC), register the overlay at the menu, then enter the
	game so art loads fresh. ~60-90s."""
	try:
		subprocess.Popen(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass",
		                  "-File", os.path.abspath(RELAUNCH_SCRIPT)])
	except Exception as e:  # noqa: BLE001
		return jsonify({"ok": False, "stage": "launch", "error": str(e)}), 500
	# wait for the fresh process to boot (menu pump up)
	for _ in range(40):
		s, _e = _dbg("GET", "/status", timeout=2)
		if s and s.get("menuPumpHooked") and s.get("captureCount", 1) == 0:
			break
		time.sleep(3)
	time.sleep(10)  # let the title screen settle
	ok, note = _drive_into_game(register_first=True)
	return jsonify({"ok": ok, "note": note})


@flask_app.get("/api/item/<path:item_id>/alt/<alt_id>/cell.png")
def api_alt_cell_preview(item_id, alt_id):
	"""Live in-cell framing preview: composite the alt's saved render into its inventory cell
	at ?fill=&dx=&dy= (no re-render, no save). Used by the UI framing sliders."""
	it = _item(item_id)
	if not it:
		return "no such item", 404
	render = assets.alt_render_png(item_id, alt_id)
	if render is None:
		return "no saved render for this alternate", 404
	fill = float(request.args.get("fill", 0.94))
	dx = float(request.args.get("dx", 0.0))
	dy = float(request.args.get("dy", 0.0))
	grade = {k: request.args.get(k) for k in ("brightness", "warmth", "saturation", "contrast")
	         if request.args.get(k) is not None} or None
	try:
		png = assets.cell_preview_png(render, it["invwidth"], it["invheight"], fill=fill, dx=dx, dy=dy, grade=grade)
	except Exception as e:  # noqa: BLE001
		return f"preview error: {e}", 500
	return Response(png, mimetype="image/png")


@flask_app.post("/api/item/<path:item_id>/alt/<alt_id>/refit")
def api_alt_refit(item_id, alt_id):
	"""Apply new framing (fill/dx/dy) to an alt by re-cropping its saved render into a new DC6.
	Instant (no Blender). Body: {fill, dx, dy}."""
	it = _item(item_id)
	if not it:
		return "no such item", 404
	body = request.json or {}
	ok = assets.refit_alt(item_id, alt_id, it["invwidth"], it["invheight"],
	                      float(body.get("fill", 0.94)), float(body.get("dx", 0.0)),
	                      float(body.get("dy", 0.0)), grade=body.get("grade") or None)
	if not ok:
		return jsonify({"ok": False, "error": "no saved render for this alternate (re-render first)"}), 409
	return jsonify({"ok": True, "alt_id": alt_id})


@flask_app.post("/api/item/<path:item_id>/alt/<alt_id>/open-blender")
def api_alt_open_blender(item_id, alt_id):
	"""Open the alt's retained GLB in the Blender GUI for hands-on tweaking."""
	glb = assets.alt_asset(item_id, alt_id, ".glb")
	if not os.path.exists(glb):
		return jsonify({"ok": False, "error": "no GLB saved for this alternate"}), 404
	if not blender.open_gui(glb):
		return jsonify({"ok": False, "error": "Blender not found or GLB missing"}), 501
	return jsonify({"ok": True, "opened": glb})


@flask_app.post("/api/item/<path:item_id>/drop")
def api_drop(item_id):
	"""Spawn the item on the ground at the player's feet (to test its art in-game).
	Uses D2Debugger /showcase/item. The drop lands the item, then a post-drop notify step
	faults harmlessly (SEH-caught) — so we report a 'faulted' result as a successful drop."""
	it = _item(item_id)
	if not it:
		return "no such item", 404
	res, err = _dbg("POST", "/showcase/item",
	                {"code": it["code"], "drop": True, "confirm": True}, timeout=8)
	if err:
		return jsonify({"ok": False, "error": f"game not reachable ({err})"}), 502
	if res.get("ok"):
		return jsonify({"ok": True, "dropped": True, "note": "dropped at your feet"})
	emsg = res.get("error", "")
	if "FAULT" in emsg:
		return jsonify({"ok": True, "dropped": True,
		                "note": "dropped at your feet (a post-drop step faults harmlessly) — "
		                        "pick it up to see the inventory art; the flippy plays on landing"})
	if "in-world" in emsg or "IN a game" in emsg:
		return jsonify({"ok": False, "error": "be in-world (enter a game) to drop an item"}), 409
	return jsonify({"ok": False, "error": emsg}), 502


@flask_app.post("/api/set/spawn")
def api_set_spawn():
	"""Spawn every piece of a named set into the player's inventory (forced SET quality,
	auto-identified). Body: {"set": "tal rasha"}. Each piece is created on its correct base
	with setRow forced, dropped, and picked up via the native 0x16 replay. Must be in-world."""
	from app.catalog import set_pieces
	name = (request.json or {}).get("set", "").strip()
	if not name:
		return jsonify({"ok": False, "error": "want {\"set\":\"tal rasha\"}"}), 400
	pieces = set_pieces(name)
	if not pieces:
		return jsonify({"ok": False, "error": f"no set pieces match {name!r}"}), 404
	results = []
	for p in pieces:
		# Drop at the player's feet (dest "feet") -- the proven client-synced path. Set pieces drop
		# unidentified (vanilla); the player IDs + grabs them in-game. (Auto-pickup + auto-identify of
		# set jewelry crashed the client, see AssetStudioPlan §30.)
		res, err = _dbg("POST", "/showcase/item",
		                {"code": p["base"], "dest": "feet", "setRow": p["row"], "confirm": True},
		                timeout=12)
		ok = bool(res and res.get("ok"))
		results.append({"index": p["index"], "base": p["base"], "row": p["row"],
		                "ok": ok, "detail": (err or (res or {}).get("error") or (res or {}).get("note"))})
		time.sleep(0.6)  # pace the drops so the frame-tick pump keeps up
	spawned = sum(1 for r in results if r["ok"])
	return jsonify({"ok": spawned > 0, "set": pieces[0]["set"], "spawned": spawned,
	                "total": len(pieces), "pieces": results})


@flask_app.get("/api/set/list")
def api_set_list():
	"""List the pieces of a named set (row/base/index) without spawning. ?q=tal+rasha"""
	from app.catalog import set_pieces
	q = request.args.get("q", "").strip()
	if not q:
		return jsonify({"ok": False, "error": "want ?q=<set name>"}), 400
	return jsonify({"ok": True, "pieces": set_pieces(q)})


# ==== Generation Studio (Meshy web-session: draft -> preview -> reroll -> texture -> accept) ====

# task_id -> {item_id, image_id, phase}; seeded from meshy_links.json so pairings
# survive restarts, and every mutation writes back through _remember().
_LINKS = meshy_links.load_links()
_STUDIO = {tid: {"item_id": l["item_id"], "image_id": l.get("image_id"),
                 "phase": l.get("phase", "draft")} for tid, l in _LINKS.items()}


def _remember(tid, item_id, image_id, phase, source, name="", invfile=None):
	_STUDIO[tid] = {"item_id": item_id, "image_id": image_id, "phase": phase}
	meshy_links.remember(_LINKS, tid, item_id=item_id, image_id=image_id,
	                     phase=phase, source=source, name=name, invfile=invfile)


@flask_app.get("/studio")
def studio_page():
	return send_from_directory(STATIC, "studio.html")


@flask_app.get("/api/studio/session")
def api_studio_session():
	s = meshy_web.session_status()
	s["blender"] = blender.available()
	return jsonify(s)


@flask_app.post("/api/studio/session/launch")
def api_studio_launch():
	return jsonify(meshy_web.launch_session())


@flask_app.post("/api/studio/generate")
def api_studio_generate():
	"""Register the item's sprite (aspect-preserved) + create a DRAFT. Returns the draft task id."""
	body = request.json or {}
	it = _item(body.get("item_id", ""))
	if not it:
		return jsonify({"ok": False, "error": "no such item"}), 404
	opts = body.get("opts") or {}
	try:
		png0 = assets.dc6_to_png_bytes(assets.read_original_dc6(it["invfile"]))
		sprite = assets.prep_image_for_meshy(png0)
		image_id = meshy_web.register_image(sprite, filename=f"{it['code'].strip()}.png")
		tid = meshy_web.create_draft(image_id, ai_model=opts.get("aiModel", "avocado"),
		                             model_type=opts.get("modelType", "standard"),
		                             topology=opts.get("topology", "triangle"),
		                             symmetry=int(opts.get("symmetry", 0)),
		                             seed=int(opts.get("seed", 0)))
		_remember(tid, it["id"], image_id, "draft", "studio-generate", name=it["name"],
		          invfile=it["invfile"].lower())
	except Exception as e:  # noqa: BLE001
		return jsonify({"ok": False, "error": str(e)}), 502
	return jsonify({"ok": True, "task_id": tid, "phase": "draft"})


@flask_app.post("/api/studio/reroll")
def api_studio_reroll():
	"""FREE ×8 in-place re-roll (POST /v2/tasks/{id}/retry, captured 2026-07-19). Meshy
	REPLACES the draft: the returned task_id is NEW and the old one 404s — the caller must
	poll the new id. Costs 0 credits (deducts one of the draft's 8 free retries)."""
	body = request.json or {}
	src = body.get("task_id", "")
	st = _STUDIO.get(src)
	if not st:
		return jsonify({"ok": False, "error": "unknown draft task (generate first)"}), 400
	try:
		tid = meshy_web.retry_task(src)
		if not tid:
			return jsonify({"ok": False, "error": "retry accepted but replacement task id "
			                "not found (re-list tasks)"}), 502
		_remember(tid, st["item_id"], st["image_id"], "draft", "studio-reroll")
		_STUDIO.pop(src, None)  # old id is dead server-side (404)
		meshy_links.forget(_LINKS, src)
	except Exception as e:  # noqa: BLE001
		return jsonify({"ok": False, "error": str(e)}), 502
	return jsonify({"ok": True, "task_id": tid, "phase": "draft", "free": True,
	                "note": "in-place free retry; old task id is gone — poll the new id"})


@flask_app.post("/api/studio/texture")
def api_studio_texture():
	"""Texture an approved draft. Returns the texture task id."""
	body = request.json or {}
	src = body.get("task_id", "")
	st = _STUDIO.get(src)
	if not st:
		return jsonify({"ok": False, "error": "unknown draft task"}), 400
	opts = body.get("opts") or {}
	try:
		tid = meshy_web.create_texture(src, st["image_id"], art_style=opts.get("artStyle", "realistic"),
		                               enable_pbr=bool(opts.get("enablePBR", True)),
		                               prompt=opts.get("prompt", ""))
		_remember(tid, st["item_id"], st["image_id"], "texture", "studio-texture")
	except Exception as e:  # noqa: BLE001
		return jsonify({"ok": False, "error": str(e)}), 502
	return jsonify({"ok": True, "task_id": tid, "phase": "texture"})


@flask_app.get("/api/studio/task/<tid>")
def api_studio_task(tid):
	try:
		t = meshy_web.get_task(tid)
	except Exception as e:  # noqa: BLE001
		return jsonify({"ok": False, "error": str(e)}), 502
	return jsonify({"ok": True, "status": t.get("status"), "phase": t.get("phase"),
	                "progress": t.get("progress"), "hasGlb": bool(meshy_web.task_glb_url(t))})


@flask_app.get("/api/studio/glb/<tid>.glb")
def api_studio_glb(tid):
	"""Proxy the task's GLB (three.js loads it from us; the Meshy URL is signed/short-lived)."""
	try:
		t = meshy_web.get_task(tid)
		url = meshy_web.task_glb_url(t)
		if not url:
			return "no GLB yet", 404
		data = meshy_web.download(url)
	except Exception as e:  # noqa: BLE001
		return f"glb error: {e}", 502
	return Response(data, mimetype="model/gltf-binary")


@flask_app.post("/api/studio/accept")
def api_studio_accept():
	"""Render the chosen model's GLB at the inventory angle, color-grade, crop-to-fill DC6, save as
	an alternate + activate. Body: {task_id, azim, elev, fill, dx, dy, grade}."""
	body = request.json or {}
	tid = body.get("task_id", "")
	st = _STUDIO.get(tid)
	if not st:
		return jsonify({"ok": False, "error": "unknown task"}), 400
	it = _item(st["item_id"])
	if not it:
		return jsonify({"ok": False, "error": "item gone"}), 404
	if not blender.available():
		return jsonify({"ok": False, "error": "Blender not found"}), 501
	azim = float(body.get("azim", 25)); elev = float(body.get("elev", 15))
	fill = float(body.get("fill", 0.94)); dx = float(body.get("dx", 0)); dy = float(body.get("dy", 0))
	grade = body.get("grade") or None
	iw, ih = it["invwidth"], it["invheight"]
	try:
		os.makedirs(MESHY_CACHE, exist_ok=True)
		t = meshy_web.get_task(tid)
		glb_path = os.path.join(MESHY_CACHE, f"studio_{tid}.glb")
		with open(glb_path, "wb") as f:
			f.write(meshy_web.download(meshy_web.task_glb_url(t)))
		out_png = os.path.join(MESHY_CACHE, f"studio_{tid}_a{int(azim)}e{int(elev)}.png")
		K = 64
		paths = blender.render(glb_path, out_png, azim=azim, elev=elev, margin=1.06,
		                       res_x=iw * K, res_y=ih * K)
		with open(paths[0], "rb") as f:
			png = f.read()
		dc6_bytes = assets.png_to_item_dc6(png, iw, ih, fill=fill, dx=dx, dy=dy, grade=grade)
		alt_id = f"studio-{tid[:8]}"
		assets.save_alternate_dc6(st["item_id"], alt_id, dc6_bytes)
		src = assets.dc6_to_png_bytes(assets.read_original_dc6(it["invfile"]))
		assets.save_alt_provenance(st["item_id"], alt_id, glb=open(glb_path, "rb").read(),
		                           render_png=png, source_png=src,
		                           meta={"source": "studio", "task_id": tid, "azim": azim,
		                                 "elev": elev, "fill": fill, "dx": dx, "dy": dy, "grade": grade})
		assets.activate(st["item_id"], it["invfile"], alt_id)
	except Exception as e:  # noqa: BLE001
		return jsonify({"ok": False, "error": str(e)}), 502
	return jsonify({"ok": True, "alt_id": alt_id, "note": "activated -- Push to game to see it"})


@flask_app.get("/pairing")
def pairing_page():
	return send_from_directory(STATIC, "pairing.html")


@flask_app.post("/api/meshy/links/batch")
def api_meshy_link_batch():
	"""Link many task->item picks in one go (the pairing page's 'Link all picked').
	Body: {picks: [{task_id, item_id}, ...]}. Reports per-pick ok/error."""
	picks = (request.json or {}).get("picks") or []
	done, failed = [], []
	for p in picks:
		tid, item_id = p.get("task_id", ""), p.get("item_id", "")
		it = _item(item_id)
		if not it:
			failed.append({"task_id": tid, "error": "no such item"})
			continue
		try:
			t = meshy_web.get_task(tid)
		except Exception as e:  # noqa: BLE001
			failed.append({"task_id": tid, "error": str(e)[:120]})
			continue
		_remember(tid, item_id, meshy_links._task_image_id(t), t.get("phase") or "draft",
		          p.get("source") or "reviewed", name=t.get("name") or "",
		          invfile=(p.get("invfile") or it["invfile"]).lower())
		done.append({"task_id": tid, "item_id": item_id, "item_name": it["name"]})
	return jsonify({"ok": True, "linked": done, "failed": failed})


@flask_app.post("/api/meshy/none")
def api_meshy_none():
	"""'None' for an art file: unlink every generation currently paired to it. The
	generations return to Unplaced so a wrong auto-match frees them to be reassigned
	rather than being stuck on the wrong DC6."""
	invfile = ((request.json or {}).get("invfile") or "").lower()
	freed = [tid for tid, l in _LINKS.items() if (l.get("invfile") or "").lower() == invfile]
	for tid in freed:
		_STUDIO.pop(tid, None)
		meshy_links.forget(_LINKS, tid)
	return jsonify({"ok": True, "invfile": invfile, "freed": freed})


@flask_app.post("/api/meshy/ignore")
def api_meshy_ignore():
	"""'None' for a generation: never pair this one (non-game art, experiments).
	Persisted so a rescan cannot re-link it; reversible."""
	body = request.json or {}
	tid = body.get("task_id", "")
	ignored = body.get("ignored", True)
	if not tid:
		return jsonify({"ok": False, "error": "task_id required"}), 400
	_STUDIO.pop(tid, None)
	meshy_links.set_ignored(_LINKS, tid, bool(ignored))
	return jsonify({"ok": True, "task_id": tid, "ignored": bool(ignored)})


@flask_app.get("/api/dc6/<name>.png")
def api_dc6_png(name):
	"""Render a DC6 by FILE NAME. The pairing page is anchored on art files, some of
	which (PD2 customs like invch1) no catalog item references."""
	try:
		png = assets.dc6_to_png_bytes(assets.read_original_dc6(name))
	except Exception as e:  # noqa: BLE001
		return f"render error: {e}", 404
	return Response(png, mimetype="image/png")


@flask_app.post("/api/meshy/primary")
def api_meshy_primary():
	"""Choose WHICH generation is the one to use for a DC6 (several can target one
	file -- e.g. four re-imagined variants of invtgl). Exclusive per invfile."""
	body = request.json or {}
	tid, invfile = body.get("task_id", ""), (body.get("invfile") or "").lower()
	if tid not in _LINKS:
		return jsonify({"ok": False, "error": "task not linked"}), 404
	for k, l in _LINKS.items():
		if (l.get("invfile") or "").lower() == invfile:
			l["primary"] = (k == tid)
	meshy_links.save_links(_LINKS)
	return jsonify({"ok": True, "invfile": invfile, "task_id": tid})


@flask_app.get("/api/meshy/pairs")
def api_meshy_pairs():
	"""The pairing view, anchored on DC6 ART FILES: every file that has at least one
	matched Meshy generation, plus the generations still needing a home."""
	tasks = {}
	try:
		for pg in (1, 2):
			for t in meshy_web.list_tasks(page_num=pg, page_size=30):
				tasks[t["id"]] = t
	except Exception as e:  # noqa: BLE001
		return jsonify({"ok": False, "error": str(e)}), 502
	c = catalog()
	byfile = {}
	for it in c["items"]:
		byfile.setdefault((it["invfile"] or "").lower(), []).append(it)

	rows = {}
	for tid, l in _LINKS.items():
		if l.get("ignored"):
			continue
		f = (l.get("invfile") or "").lower()
		if not f:
			continue
		t = tasks.get(tid) or {}
		grp = byfile.get(f, [])
		row = rows.setdefault(f, {
			"invfile": f,
			"item_id": l.get("item_id"),
			"items": [i["name"] for i in grp],
			"item_count": len(grp),
			"generations": [],
		})
		row["generations"].append({
			"task_id": tid,
			"name": l.get("name") or "",
			"prompt": ((t.get("args") or {}).get("draft") or {}).get("prompt", ""),
			"input_image": meshy_links._task_input_url(t) or "",
			"preview": ((t.get("result") or {}).get("previewUrl") or ""),
			"status": t.get("status"), "phase": t.get("phase"),
			"retries_left": 8 - (t.get("retryCount") or 0),
			"source": l.get("source") or "",
			"primary": bool(l.get("primary")),
			"alive": tid in tasks,
		})
	for r in rows.values():
		gens = r["generations"]
		if gens and not any(g["primary"] for g in gens):
			gens[0]["primary"] = True   # default: first one wins until you choose
		gens.sort(key=lambda g: (not g["primary"], g["name"] or g["prompt"]))

	unpaired = [{
		"task_id": t["id"],
		"name": t.get("name") or "",
		"prompt": ((t.get("args") or {}).get("draft") or {}).get("prompt", ""),
		"input_image": meshy_links._task_input_url(t) or "",
		"preview": ((t.get("result") or {}).get("previewUrl") or ""),
	} for t in tasks.values()
		if t["id"] not in _LINKS and t.get("phase") in ("generate", "draft")
		and t.get("status") == "SUCCEEDED"]

	ignored = [{
		"task_id": tid,
		"name": (_LINKS[tid].get("name") or ""),
		"prompt": ((tasks.get(tid, {}).get("args") or {}).get("draft") or {}).get("prompt", ""),
		"input_image": meshy_links._task_input_url(tasks.get(tid, {})) or "",
	} for tid in _LINKS if _LINKS[tid].get("ignored")]

	return jsonify({"ok": True,
	                "pairs": sorted(rows.values(), key=lambda r: r["invfile"]),
	                "unpaired": unpaired, "ignored": ignored})


@flask_app.get("/api/meshy/tasks")
def api_meshy_tasks():
	"""Recent Meshy workspace tasks (slim), with any linked item, for the pairing UI."""
	page = int(request.args.get("page", 1))
	try:
		tasks = meshy_web.list_tasks(page_num=page, page_size=30)
	except Exception as e:  # noqa: BLE001
		return jsonify({"ok": False, "error": str(e)}), 502
	out = []
	for t in tasks:
		tid = t.get("id")
		link = _LINKS.get(tid)
		it = _item(link["item_id"]) if link else None
		out.append({
			"id": tid, "name": t.get("name") or "", "phase": t.get("phase"),
			"status": t.get("status"), "retryCount": t.get("retryCount") or 0,
			"createdAt": t.get("createdAt"),
			"preview": ((t.get("result") or {}).get("previewUrl") or ""),
			"linked_item": link["item_id"] if link else None,
			"linked_item_name": it["name"] if it else None,
		})
	return jsonify({"ok": True, "page": page, "tasks": out})


@flask_app.get("/api/meshy/links")
def api_meshy_links():
	out = []
	for tid, l in sorted(_LINKS.items(), key=lambda kv: kv[1].get("linked_at", ""), reverse=True):
		it = _item(l["item_id"])
		out.append(dict(l, task_id=tid, item_name=(it["name"] if it else "?")))
	return jsonify({"ok": True, "links": out})


@flask_app.post("/api/meshy/links")
def api_meshy_link():
	"""Manually pair a Meshy task with a catalog item (or confirm a scan suggestion).
	Recovers the task's registered image_id so texture/re-roll work on the pairing."""
	body = request.json or {}
	tid = body.get("task_id", ""); item_id = body.get("item_id", "")
	it = _item(item_id)
	if not it:
		return jsonify({"ok": False, "error": "no such item"}), 404
	try:
		t = meshy_web.get_task(tid)
	except Exception as e:  # noqa: BLE001
		return jsonify({"ok": False, "error": f"no such task: {e}"}), 404
	_remember(tid, item_id, meshy_links._task_image_id(t), t.get("phase") or "draft",
	          body.get("source") or "manual", name=t.get("name") or "",
	          invfile=(body.get("invfile") or it["invfile"]).lower())
	return jsonify({"ok": True, "task_id": tid, "item_id": item_id, "item_name": it["name"],
	                "invfile": (body.get("invfile") or it["invfile"]).lower()})


@flask_app.delete("/api/meshy/links/<tid>")
def api_meshy_unlink(tid):
	_STUDIO.pop(tid, None)
	return jsonify({"ok": meshy_links.forget(_LINKS, tid)})


@flask_app.post("/api/meshy/links/scan")
def api_meshy_scan():
	"""Auto-pair unlinked drafts: image-hash matches link immediately; name matches
	come back as suggestions for one-click confirm. Body: {pages?: 2}."""
	pages = min(10, int((request.json or {}).get("pages", 2)))
	tasks = []
	try:
		for p in range(1, pages + 1):
			batch = meshy_web.list_tasks(page_num=p, page_size=30)
			if not batch:
				break
			tasks.extend(batch)
	except Exception as e:  # noqa: BLE001
		return jsonify({"ok": False, "error": str(e)}), 502
	res = meshy_links.auto_pair(tasks, catalog()["items"], _LINKS)
	for a in res["auto"]:  # mirror fresh links into the live studio map
		l = _LINKS[a["task_id"]]
		_STUDIO[a["task_id"]] = {"item_id": l["item_id"], "image_id": l.get("image_id"),
		                         "phase": l.get("phase", "draft")}
	return jsonify({"ok": True, "scanned": len(tasks), **res})


@flask_app.get("/api/game/status")
def api_game_status():
	res, err = _dbg("GET", "/asset/status", timeout=4)
	if err:
		return jsonify({"ok": False, "reachable": False, "error": err})
	return jsonify({"ok": True, "reachable": True, "asset": res})


if __name__ == "__main__":
	print("PD2 Asset Studio -> http://127.0.0.1:5001")
	flask_app.run(host="127.0.0.1", port=5001, debug=False, threaded=True)
