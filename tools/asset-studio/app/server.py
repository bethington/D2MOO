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
import app.meshy as meshy  # noqa: E402
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
		dc6_bytes = assets.png_to_item_dc6(f.read(), it["invwidth"], it["invheight"])
		assets.save_alternate_dc6(item_id, alt_id, dc6_bytes)
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


@flask_app.post("/api/item/<path:item_id>/meshy/render-flippy/<task_id>")
def api_meshy_render_flippy(item_id, task_id):
	"""Blender-turntable the Meshy GLB into a full flippy: one render per original
	flippy frame (a whole tumble), sized+anchored to the original's fall arc.
	Body: {elev} (default 15)."""
	it = _item(item_id)
	if not it:
		return "no such item", 404
	if not it["flippyfile"]:
		return jsonify({"ok": False, "error": "item has no flippyfile"}), 400
	if not blender.available():
		return jsonify({"ok": False, "error": "Blender not found (install it or set BLENDER_EXE)"}), 501
	elev = float((request.json or {}).get("elev", 15))
	try:
		ref = assets.read_original_dc6(it["flippyfile"])
		from pyd2 import dc6 as dc6mod
		n_frames = dc6mod.decode(ref).frames_per_direction
		os.makedirs(MESHY_CACHE, exist_ok=True)
		glb_path = os.path.join(MESHY_CACHE, f"{task_id}.glb")
		if not os.path.exists(glb_path):
			t = meshy.get_task(task_id)
			if t.get("status") != "SUCCEEDED":
				return jsonify({"ok": False, "error": f"task not ready ({t.get('status')})"}), 409
			with open(glb_path, "wb") as f:
				f.write(meshy.download(t["model_urls"]["glb"]))
		out_png = os.path.join(MESHY_CACHE, f"{task_id}_flippy.png")
		paths = blender.render(glb_path, out_png, size=96, azim=0, elev=elev,
		                       frames=n_frames, azim_step=360.0 / n_frames, timeout=600)
		if len(paths) != n_frames:
			return jsonify({"ok": False,
			                "error": f"Blender produced {len(paths)} frames, need {n_frames}"}), 502
		pngs = []
		for p in paths:
			with open(p, "rb") as f:
				pngs.append(f.read())
		flippy_dc6 = assets.pngs_to_flippy_dc6(pngs, ref)
		alt_id = f"blender-{task_id[:8]}-tumble"
		assets.save_flippy_alternate_dc6(item_id, alt_id, flippy_dc6)
	except Exception as e:  # noqa: BLE001
		return jsonify({"ok": False, "error": str(e)}), 502
	return jsonify({"ok": True, "alt_id": alt_id,
	                "flippy_alts": assets.list_flippy_alternates(item_id)})


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


# ---- Meshy.ai image-to-3D pipeline -------------------------------------

@flask_app.get("/api/meshy/status")
def api_meshy_status():
	if not meshy.api_key():
		return jsonify({"ok": False, "hasKey": False, "blender": blender.available(),
		                "note": "no Meshy key (put it in C:\\Diablo2\\AssetStudio\\meshy.key)"})
	try:
		return jsonify({"ok": True, "hasKey": True, "blender": blender.available(),
		                "balance": meshy.balance()})
	except Exception as e:  # noqa: BLE001
		return jsonify({"ok": False, "hasKey": True, "blender": blender.available(), "error": str(e)})


@flask_app.post("/api/item/<path:item_id>/meshy/generate")
def api_meshy_generate(item_id):
	"""S1 (extract original + upscale) -> S2 (submit image-to-3D). Returns the task id. Billable."""
	it = _item(item_id)
	if not it:
		return "no such item", 404
	try:
		png0 = assets.dc6_to_png_bytes(assets.read_original_dc6(it["invfile"]))
		big = Image.open(io.BytesIO(png0)).convert("RGBA").resize((512, 512), Image.LANCZOS)
		buf = io.BytesIO()
		big.save(buf, format="PNG")
		tid = meshy.submit_image_to_3d(buf.getvalue(), should_texture=True)
	except Exception as e:  # noqa: BLE001
		return jsonify({"ok": False, "error": str(e)}), 502
	return jsonify({"ok": True, "task_id": tid})


@flask_app.get("/api/meshy/task/<task_id>")
def api_meshy_task(task_id):
	try:
		t = meshy.get_task(task_id)
	except Exception as e:  # noqa: BLE001
		return jsonify({"ok": False, "error": str(e)}), 502
	return jsonify({"ok": True, "status": t.get("status"), "progress": t.get("progress"),
	                "thumbnail_url": t.get("thumbnail_url"), "error": t.get("task_error")})


@flask_app.get("/api/meshy/preview/<task_id>.png")
def api_meshy_preview(task_id):
	"""Proxy the Meshy 3D-model preview render (so the UI can show it for review)."""
	try:
		t = meshy.get_task(task_id)
		if not t.get("thumbnail_url"):
			return "no preview yet", 404
		return Response(meshy.download(t["thumbnail_url"]), mimetype="image/png")
	except Exception as e:  # noqa: BLE001
		return f"preview error: {e}", 502


@flask_app.post("/api/item/<path:item_id>/meshy/texture/<task_id>")
def api_meshy_texture(item_id, task_id):
	"""S3: retexture the generated 3D model. By default textures FROM the item's original
	sprite (image_style_url); an optional text prompt refines/overrides. Returns a new task id."""
	it = _item(item_id)
	if not it:
		return "no such item", 404
	body = request.json or {}
	prompt = (body.get("prompt") or "").strip() or None
	use_image = body.get("use_image", True)
	try:
		img = None
		if use_image:
			# the item's original inv sprite, upscaled — same source the model came from
			png0 = assets.dc6_to_png_bytes(assets.read_original_dc6(it["invfile"]))
			img = Image.open(io.BytesIO(png0)).convert("RGBA").resize((512, 512), Image.LANCZOS)
			buf = io.BytesIO()
			img.save(buf, format="PNG")
			img = buf.getvalue()
		if not img and not prompt:
			return jsonify({"ok": False, "error": "provide a text prompt or enable texture-from-image"}), 400
		tid = meshy.submit_retexture(task_id, text_prompt=prompt, image_bytes=img)
	except Exception as e:  # noqa: BLE001
		return jsonify({"ok": False, "error": str(e)}), 502
	return jsonify({"ok": True, "task_id": tid})


@flask_app.post("/api/item/<path:item_id>/meshy/use/<task_id>")
def api_meshy_use(item_id, task_id):
	"""Download the finished 3D preview render and save it as a DC6 alternate for the item."""
	it = _item(item_id)
	if not it:
		return "no such item", 404
	try:
		t = meshy.get_task(task_id)
		if t.get("status") != "SUCCEEDED":
			return jsonify({"ok": False, "error": f"task not ready ({t.get('status')})"}), 409
		preview = meshy.download(t["thumbnail_url"])
		dc6_bytes = assets.png_to_item_dc6(preview, it["invwidth"], it["invheight"])
		alt_id = f"meshy-{task_id[:8]}"
		assets.save_alternate_dc6(item_id, alt_id, dc6_bytes)
	except Exception as e:  # noqa: BLE001
		return jsonify({"ok": False, "error": str(e)}), 502
	return jsonify({"ok": True, "alt_id": alt_id, "alts": assets.list_alternates(item_id)})


@flask_app.post("/api/item/<path:item_id>/meshy/render/<task_id>")
def api_meshy_render(item_id, task_id):
	"""S4: download the Meshy GLB (cached) and render it with Blender at the chosen angle,
	then save the sprite as a DC6 alternate. Body: {azim, elev, size}."""
	it = _item(item_id)
	if not it:
		return "no such item", 404
	if not blender.available():
		return jsonify({"ok": False, "error": "Blender not found (install it or set BLENDER_EXE)"}), 501
	body = request.json or {}
	azim = float(body.get("azim", 25))
	elev = float(body.get("elev", 20))
	size = int(body.get("size", 256))
	try:
		os.makedirs(MESHY_CACHE, exist_ok=True)
		glb_path = os.path.join(MESHY_CACHE, f"{task_id}.glb")
		if not os.path.exists(glb_path):
			t = meshy.get_task(task_id)
			if t.get("status") != "SUCCEEDED":
				return jsonify({"ok": False, "error": f"task not ready ({t.get('status')})"}), 409
			with open(glb_path, "wb") as f:
				f.write(meshy.download(t["model_urls"]["glb"]))
		out_png = os.path.join(MESHY_CACHE, f"{task_id}_a{int(azim)}_e{int(elev)}.png")
		paths = blender.render(glb_path, out_png, size=size, azim=azim, elev=elev)
		with open(paths[0], "rb") as f:
			png = f.read()
		dc6_bytes = assets.png_to_item_dc6(png, it["invwidth"], it["invheight"])
		alt_id = f"blender-{task_id[:8]}-a{int(azim)}e{int(elev)}"
		assets.save_alternate_dc6(item_id, alt_id, dc6_bytes)
	except Exception as e:  # noqa: BLE001
		return jsonify({"ok": False, "error": str(e)}), 502
	return jsonify({"ok": True, "alt_id": alt_id, "alts": assets.list_alternates(item_id)})


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


@flask_app.get("/api/game/status")
def api_game_status():
	res, err = _dbg("GET", "/asset/status", timeout=4)
	if err:
		return jsonify({"ok": False, "reachable": False, "error": err})
	return jsonify({"ok": True, "reachable": True, "asset": res})


if __name__ == "__main__":
	print("PD2 Asset Studio -> http://127.0.0.1:5001")
	flask_app.run(host="127.0.0.1", port=5001, debug=False, threaded=True)
