"""Meshy WEB-session driver — use your plan's FREE regenerations (which the openapi API key
cannot reach) by reading your live browser login token and calling the web app's own endpoint.

Why: Meshy runs two auth systems. The openapi API key (billed per call) hits
`api.meshy.ai/openapi/v1/*` and has NO free-retry endpoint. Free regeneration lives ONLY on the
web app's internal API `api.meshy.ai/web/v2/tasks/{id}/regenerate`, which authenticates with the
browser LOGIN session (a Supabase JWT), not the API key (verified: the key gets 401 there).

Approach (no manual token-paste): launch a dedicated Chrome with a debug port + persistent
profile pointed at app.meshy.ai; the user logs in ONCE; Supabase auto-refreshes the token while
that tab stays open, so we read a FRESH token from its localStorage over the DevTools Protocol
each time we regenerate. The token never leaves this machine and is never persisted by us.
"""

from __future__ import annotations

import json
import os
import subprocess
import time
import urllib.request

import websocket  # websocket-client

WORKSPACE = os.environ.get("ASSET_STUDIO_WS", r"C:\Diablo2\AssetStudio")
PROFILE_DIR = os.path.join(WORKSPACE, "chrome-meshy-profile")
DEBUG_PORT = int(os.environ.get("MESHY_CHROME_PORT", "9223"))
WEB_API = "https://api.meshy.ai/web"
LOGIN_URL = "https://app.meshy.ai/workspace?model-tab=image-to-3d"

_CHROME_CANDIDATES = [
	r"C:\Program Files\Google\Chrome\Application\chrome.exe",
	r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe",
	os.path.expandvars(r"%LOCALAPPDATA%\Google\Chrome\Application\chrome.exe"),
]


def _chrome() -> str | None:
	for p in _CHROME_CANDIDATES:
		if os.path.exists(p):
			return p
	return os.environ.get("CHROME_EXE")


def _cdp_targets() -> list | None:
	"""GET the debug port's target list, or None if the port isn't up."""
	try:
		with urllib.request.urlopen(f"http://127.0.0.1:{DEBUG_PORT}/json", timeout=3) as r:
			return json.loads(r.read().decode())
	except Exception:  # noqa: BLE001
		return None


def browser_running() -> bool:
	return _cdp_targets() is not None


def launch_browser() -> dict:
	"""Launch (or reuse) the dedicated debug Chrome at the Meshy login page. Idempotent."""
	if browser_running():
		return {"ok": True, "already": True}
	exe = _chrome()
	if not exe:
		return {"ok": False, "error": "Chrome not found (set CHROME_EXE)"}
	os.makedirs(PROFILE_DIR, exist_ok=True)
	subprocess.Popen([
		exe,
		f"--remote-debugging-port={DEBUG_PORT}",
		f"--user-data-dir={PROFILE_DIR}",
		# newer Chrome rejects CDP websockets unless the caller's origin is allowlisted
		"--remote-allow-origins=*",
		"--no-first-run", "--no-default-browser-check",
		"--new-window", LOGIN_URL,
	])
	for _ in range(20):  # wait for the debug port to come up
		time.sleep(0.5)
		if browser_running():
			return {"ok": True, "launched": True}
	return {"ok": False, "error": "Chrome launched but debug port never opened"}


_READ_TOKEN_JS = r"""
(() => {
  try {
    for (let i = 0; i < localStorage.length; i++) {
      const k = localStorage.key(i);
      if (k && k.indexOf('auth-token') !== -1) {
        let raw = localStorage.getItem(k);
        try {
          let v = JSON.parse(raw);
          if (v && v.access_token) return v.access_token;
          if (Array.isArray(v) && v[0]) return v[0];        // some sb versions store [access, refresh]
          if (v && v.currentSession && v.currentSession.access_token) return v.currentSession.access_token;
        } catch (e) { if (raw && raw.split('.').length === 3) return raw; }
      }
    }
  } catch (e) {}
  return null;
})()
"""


def read_token() -> str | None:
	"""Read the current (auto-refreshed) Supabase access token from the logged-in Meshy tab."""
	targets = _cdp_targets()
	if not targets:
		return None
	# prefer a page target actually on meshy.ai
	page = None
	for t in targets:
		if t.get("type") == "page" and "meshy.ai" in (t.get("url") or ""):
			page = t
			break
	if not page:
		page = next((t for t in targets if t.get("type") == "page" and t.get("webSocketDebuggerUrl")), None)
	if not page or not page.get("webSocketDebuggerUrl"):
		return None
	try:
		ws = websocket.create_connection(page["webSocketDebuggerUrl"], timeout=8)
		ws.send(json.dumps({"id": 1, "method": "Runtime.evaluate",
		                    "params": {"expression": _READ_TOKEN_JS, "returnByValue": True}}))
		for _ in range(10):
			msg = json.loads(ws.recv())
			if msg.get("id") == 1:
				ws.close()
				val = (((msg.get("result") or {}).get("result")) or {}).get("value")
				return val if isinstance(val, str) and val else None
		ws.close()
	except Exception:  # noqa: BLE001
		return None
	return None


def token_status() -> dict:
	"""Is the debug browser up and is a valid login token readable?"""
	if not browser_running():
		return {"browser": False, "loggedIn": False, "note": "browser not launched"}
	tok = read_token()
	if not tok:
		return {"browser": True, "loggedIn": False, "note": "browser up -- log in at the Meshy tab"}
	return {"browser": True, "loggedIn": True, "tokenPreview": tok[:12] + "..."}


def _web(method: str, path: str, token: str, body: dict | None = None, timeout: float = 30.0):
	data = json.dumps(body).encode() if body is not None else None
	req = urllib.request.Request(WEB_API + path, data=data, method=method, headers={
		"Authorization": f"Bearer {token}",
		"Content-Type": "application/json",
		"Origin": "https://app.meshy.ai",
	})
	with urllib.request.urlopen(req, timeout=timeout) as r:
		raw = r.read().decode()
		return json.loads(raw) if raw else {}


def regenerate(task_id: str) -> dict:
	"""Trigger a FREE regeneration of an image-to-3d task via the web API. Returns
	{ok, task_id (the resulting task to poll -- may be new or the same), raw} or {ok:False,error}.
	Reads a fresh token each call so expiry is a non-issue while the browser stays logged in."""
	tok = read_token()
	if not tok:
		return {"ok": False, "error": "no login token -- launch the browser and log in first"}
	try:
		res = _web("POST", f"/v2/tasks/{task_id}/regenerate", tok, body={})
	except urllib.error.HTTPError as e:  # noqa: PERF203
		detail = e.read().decode(errors="replace")[:300]
		if e.code in (401, 403):
			return {"ok": False, "error": f"login token rejected ({e.code}) -- re-log-in in the browser", "detail": detail}
		return {"ok": False, "error": f"regenerate HTTP {e.code}: {detail}"}
	except Exception as e:  # noqa: BLE001
		return {"ok": False, "error": str(e)}
	# the resulting task id: response may echo a new id, or the same task regenerated in place
	new_id = None
	if isinstance(res, dict):
		new_id = res.get("id") or res.get("result") or (res.get("task") or {}).get("id") \
			or (res.get("data") or {}).get("id")
	return {"ok": True, "task_id": new_id or task_id, "same_task": not new_id, "raw": res}
