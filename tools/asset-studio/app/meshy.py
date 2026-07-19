"""Meshy.ai client — image-to-3D generation for the Asset Studio pipeline.

Key resolution: C:\\Diablo2\\AssetStudio\\meshy.key (one line) or the MESHY_API_KEY env var
(the .key file is outside the git repo). Never commit the key.

Staged flow (decision #6): S1 source-prep (local upscale) -> S2 image-to-3D (Meshy, billable)
-> [S4 Blender re-render — needs Blender] -> S5 DC6 encode. Without Blender, the app uses
Meshy's rendered preview (thumbnail/turntable) as the sprite source for a working v1 loop.
"""

from __future__ import annotations

import base64
import json
import os
import urllib.request

API = "https://api.meshy.ai/openapi"
KEY_FILE = os.environ.get("MESHY_KEY_FILE", r"C:\Diablo2\AssetStudio\meshy.key")


class MeshyError(RuntimeError):
	pass


def api_key() -> str | None:
	if os.path.exists(KEY_FILE):
		try:
			with open(KEY_FILE, encoding="utf-8-sig") as f:
				k = f.read().strip()
				if k:
					return k
		except OSError:
			pass
	return os.environ.get("MESHY_API_KEY")


def _req(method: str, path: str, body: dict | None = None, timeout: float = 30.0):
	key = api_key()
	if not key:
		raise MeshyError("no Meshy API key (set C:\\Diablo2\\AssetStudio\\meshy.key or MESHY_API_KEY)")
	data = json.dumps(body).encode() if body is not None else None
	req = urllib.request.Request(API + path, data=data, method=method, headers={
		"Authorization": f"Bearer {key}",
		"Content-Type": "application/json",
	})
	try:
		with urllib.request.urlopen(req, timeout=timeout) as r:
			raw = r.read().decode()
			return json.loads(raw) if raw else {}
	except urllib.error.HTTPError as e:  # noqa: PERF203
		detail = e.read().decode(errors="replace")[:400]
		raise MeshyError(f"HTTP {e.code} {path}: {detail}") from e


def balance() -> int:
	return _req("GET", "/v1/balance", timeout=15).get("balance", 0)


def _data_uri(png_bytes: bytes) -> str:
	return "data:image/png;base64," + base64.b64encode(png_bytes).decode()


def submit_image_to_3d(png_bytes: bytes, *, should_texture: bool = True,
                       ai_model: str = "latest", target_polycount: int = 30000) -> str:
	"""Submit a PNG for image-to-3D. Textures FROM the source image (Meshy's normal flow):
	uses the latest model (Meshy 6), passes the sprite as the texture reference, and enables
	the pixel-art input enhancements. Returns the task id. Billable."""
	data_uri = _data_uri(png_bytes)
	body = {
		"image_url": data_uri,
		"ai_model": ai_model,
		"should_texture": should_texture,
		"texture_image_url": data_uri,   # texture FROM the source sprite
		"image_enhancement": True,       # Meshy 6: clean up the low-res pixel-art input
		"remove_lighting": True,         # Meshy 6: neutralize baked-in shading
		"should_remesh": True,
		"target_polycount": target_polycount,
		"enable_pbr": False,
		"hd_texture": True,              # Meshy 6: 4K base-color texture (sharper on the sprite)
		"save_pre_remeshed_model": True, # keep the higher-detail GLB for Blender work
		"alpha_thumbnail": True,         # transparent-background preview (Blender-free fast path)
	}
	res = _req("POST", "/v1/image-to-3d", body, timeout=40)
	tid = res.get("result") or res.get("id")
	if not tid:
		raise MeshyError(f"no task id in response: {res}")
	return tid


def get_task(task_id: str) -> dict:
	"""Poll a task by id. Tries image-to-3D first, then retexture (they share the schema:
	status/progress/thumbnail_url/model_urls). status in PENDING|IN_PROGRESS|SUCCEEDED|FAILED."""
	try:
		return _req("GET", f"/v1/image-to-3d/{task_id}", timeout=20)
	except MeshyError:
		return _req("GET", f"/v1/retexture/{task_id}", timeout=20)


def submit_retexture(input_task_id: str, *, text_prompt: str | None = None,
                     image_bytes: bytes | None = None, enable_pbr: bool = False) -> str:
	"""Retexture an existing 3D model. Prefers texturing FROM a reference image
	(image_style_url = the source sprite) when image_bytes is given; falls back to a text
	prompt. image_style_url takes priority over text if both are set. Returns task id. Billable."""
	body = {"input_task_id": input_task_id, "enable_pbr": enable_pbr, "enable_original_uv": True}
	if image_bytes is not None:
		body["image_style_url"] = _data_uri(image_bytes)
	if text_prompt:
		body["text_style_prompt"] = text_prompt
	if "image_style_url" not in body and "text_style_prompt" not in body:
		raise MeshyError("retexture needs an image or a text prompt")
	res = _req("POST", "/v1/retexture", body, timeout=40)
	tid = res.get("result") or res.get("id")
	if not tid:
		raise MeshyError(f"no task id in retexture response: {res}")
	return tid


def download(url: str, timeout: float = 90.0) -> bytes:
	with urllib.request.urlopen(url, timeout=timeout) as r:
		return r.read()
