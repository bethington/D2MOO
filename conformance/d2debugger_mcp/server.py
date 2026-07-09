"""d2debugger-mcp -- an MCP bridge to the live D2Debugger control surface.

WS-5 of conformance/GRADUATED_CONFORMANCE_PIPELINE_PLAN.md. This is a thin
MCP<->HTTP multiplexer (sibling in spirit to ghidra-mcp) exposing the in-process
HTTP API served by D2Debugger.mcp.cpp inside a running Project Diablo 2 process.

It lets an AI agent DRIVE live shadow-proving with no human at the ImGui panel:
  - read dispatcher/profiler state,
  - flip dispatchers between Original / Reimpl / Shadow,
  - hot-reload the reimpl-provider DLL (WS-1),
  - and -- the keystone -- run the DIRECT-CALL ORACLE: call the raw original and
    the candidate reimpl with CHOSEN input vectors and get a deterministic
    match/mismatch verdict, without waiting for the game to exercise the path.

Talks over stdlib http.client (no third-party HTTP dep), same as ghidra-mcp.

Run (stdio transport):
    python server.py
Point at a non-default endpoint with D2DBG_MCP_URL (default http://127.0.0.1:8790).
"""
from __future__ import annotations

import json
import os
import http.client
from urllib.parse import urlparse

from mcp.server.fastmcp import FastMCP

mcp = FastMCP("d2debugger-mcp")

_BASE = os.environ.get("D2DBG_MCP_URL", "http://127.0.0.1:8790")


def _http(method: str, path: str, body: dict | None = None) -> dict:
    """One request to the D2Debugger control server. Returns parsed JSON, or an
    {"ok": false, "error": ...} envelope on transport/parse failure so tool
    callers always get a dict (the game may simply not be running yet)."""
    u = urlparse(_BASE)
    conn = http.client.HTTPConnection(u.hostname, u.port or 8790, timeout=15)
    payload = json.dumps(body).encode("utf-8") if body is not None else None
    headers = {"Content-Type": "application/json"} if payload is not None else {}
    try:
        conn.request(method, path, body=payload, headers=headers)
        resp = conn.getresponse()
        raw = resp.read().decode("utf-8", "replace")
    except OSError as e:
        return {"ok": False, "error": f"cannot reach D2Debugger at {_BASE} "
                f"({e}). Is PD2 running with D2_DEBUGGER=1?"}
    finally:
        conn.close()
    try:
        return json.loads(raw)
    except json.JSONDecodeError:
        return {"ok": False, "error": f"non-JSON response: {raw[:200]}"}


def _mode_str(mode) -> str:
    """Accept 0/1/2 or original/reimpl/shadow; return the canonical word."""
    table = {0: "original", 1: "reimpl", 2: "shadow",
             "0": "original", "1": "reimpl", "2": "shadow",
             "original": "original", "reimpl": "reimpl", "shadow": "shadow"}
    key = mode.lower() if isinstance(mode, str) else mode
    if key not in table:
        raise ValueError("mode must be one of: original, reimpl, shadow (or 0,1,2)")
    return table[key]


@mcp.tool()
def d2dbg_status() -> dict:
    """Overall health of the live D2Debugger: whether the cross-DLL bridge is up,
    how many dispatchers exist, the reimpl-provider load status, and profiler
    hook counts. Safe to poll. Call this first to confirm the game is reachable."""
    return _http("GET", "/status")


@mcp.tool()
def d2dbg_list_dispatchers() -> dict:
    """List every live dispatcher (the functions with a D2MOO one-for-one
    equivalent) with its index, name, D2Common offset, current mode, live hit
    count, and shadow-divergence count."""
    return _http("GET", "/dispatchers")


@mcp.tool()
def d2dbg_set_mode(index: int, mode: str) -> dict:
    """Set one dispatcher's mode. mode = 'original' (run the game's code),
    'reimpl' (run the D2MOO equivalent), or 'shadow' (run both, compare, use the
    original's result -- the safe proving mode). Returns the updated dispatcher."""
    return _http("POST", f"/dispatcher/{int(index)}/mode", {"mode": _mode_str(mode)})


@mcp.tool()
def d2dbg_set_all_modes(mode: str) -> dict:
    """Set ALL dispatchers to the same mode at once (e.g. 'shadow' to begin a
    proving pass, 'original' to return to a pure passthrough)."""
    return _http("POST", "/dispatchers/mode", {"mode": _mode_str(mode)})


@mcp.tool()
def d2dbg_reload_provider() -> dict:
    """Hot-reload the reimpl-provider DLL (WS-1) from disk into the running game
    -- no restart. Use after rebuilding an equivalent so the new code binds live.
    Loads in-memory via MemoryModule with the verified-address import resolver;
    modes are preserved across the reload. Returns the provider bind status."""
    return _http("POST", "/reimpl/reload")


@mcp.tool()
def d2dbg_call_oracle(index: int, vectors: list[dict] | None = None,
                      x: int | None = None, y: int | None = None) -> dict:
    """DIRECT-CALL ORACLE (the keystone). Call dispatcher `index`'s raw original
    AND its currently-bound reimpl with chosen inputs, and compare -- a
    deterministic proof independent of game activity or current mode.

    Provide EITHER a single point (x=, y=) OR a batch (vectors=[{"x":..,"y":..},
    ...], up to 4096). Returns per-vector {in, original, reimpl, match} plus a
    rollup {count, matches, mismatches, allMatch}. allMatch==true over a good
    vector set is the live proof the equivalent matches the game.

    ABI note: current dispatchers are the coord family (void __stdcall(int*,int*))
    -- inputs/outputs are the (x, y) pair."""
    if vectors is None:
        if x is None or y is None:
            return {"ok": False, "error": "provide vectors=[...] or both x= and y="}
        body = {"x": int(x), "y": int(y)}
    else:
        body = {"vectors": [{"x": int(v["x"]), "y": int(v["y"])} for v in vectors]}
    return _http("POST", f"/call/{int(index)}", body)


@mcp.tool()
def d2dbg_prove_spec(spec: dict) -> dict:
    """GENERAL arbitrary-ABI oracle (design detail B) -- prove ANY D2MOO reimpl
    live, not just the coord family. `spec` describes the function's ABI and the
    input vectors; the oracle calls the raw game original AND the provider's
    reimpl of the same name and compares the requested channels.

    spec = {
      "name": "SOME_Function",                       # must exist in the game AND
                                                      # be exported by the provider
      "callconv": "stdcall"|"cdecl"|"fastcall"|"thiscall",
      "ret": "void"|"i32"|"u32"|"ptr",
      "args": [ {"id":"a","kind":"i32"},             # scalar 32-bit arg
                {"id":"x","kind":"buf","bytes":4} ], # pointer to in/out scratch
      "compare": ["ret","x","y"],                    # channels to compare
      "vectors": [ {"a":5,"x":100,"y":40}, ... ]     # per-arg input values (<=4096)
    }
    Returns per-vector {match, ret{o,r}, bufs{...}} plus {count, matches,
    mismatches, allMatch}. allMatch==true is the live proof the reimpl matches
    the game across the vector set. Args are 32-bit slots (int/uint/pointer);
    float/double/int64/struct-by-value are not yet supported."""
    return _http("POST", "/oracle", spec)


@mcp.tool()
def d2dbg_list_subsystems() -> dict:
    """List all D2Common subsystems seen by the profiler with per-subsystem
    function count, aggregate live hit count, and whether count-hooks are
    installed. Use to decide what to instrument next."""
    return _http("GET", "/profiler/subsystems")


@mcp.tool()
def d2dbg_instrument(subsystem: str) -> dict:
    """Install count-only profiler hooks for ONE subsystem (Original passthrough,
    no behavior change) so its functions start reporting live hit counts. Install
    one subsystem at a time -- hooking everything at once destabilizes the game."""
    return _http("POST", "/profiler/instrument", {"subsystem": subsystem})


@mcp.tool()
def d2dbg_instrument_all() -> dict:
    """Instrument EVERY subsystem at once (the "Instrument All" action) so all
    ~1,172 D2Common functions start reporting live hit counts. Safe: installs one
    transaction PER subsystem with address-dedup + a data-export gate + graceful
    skip (the two bugs that made a blind mass-hook crash PD2 are fixed). Idempotent
    -- already-instrumented subsystems are left as-is. Returns hooked/skipped
    totals. Use before a play session to capture a whole-engine hit profile."""
    return _http("POST", "/profiler/instrument_all", {})


@mcp.tool()
def d2dbg_list_functions(subsystem: str) -> dict:
    """List the functions in one subsystem with ordinal, name, offset, live hit
    count, whether each is hooked, and whether it has a one-for-one equivalent
    (a dispatcher). Use to find hot, high-value functions to port next."""
    return _http("GET", f"/profiler/functions/{subsystem}")


@mcp.tool()
def d2dbg_main_menu_singleplayer(confirm: bool = False) -> dict:
    """Click "Single Player" on Diablo II's main/title screen -- calls D2Launch's
    StartSinglePlayerMode() on the game thread, transitioning the title screen to
    the character-select screen. MUTATES game UI state; set confirm=True.
    Must be at the title/main menu (the D2Win menu pump must be firing)."""
    if not confirm:
        return {"ok": False, "error": "pass confirm=True (this drives the live game UI)"}
    return _http("POST", "/action/main-menu-singleplayer", {"confirm": True})


@mcp.tool()
def d2dbg_launch_character(difficulty: int = 0, confirm: bool = False) -> dict:
    """Launch the CURRENTLY-HIGHLIGHTED single-player character at a chosen
    DIFFICULTY -- calls SelectCharacterByIndex(difficulty) on the game thread.
    difficulty: 0=Normal, 1=Nightmare, 2=Hell (must be UNLOCKED on the char).
    MUTATES game state; set confirm=True. Precondition: be at character-select
    with a character HIGHLIGHTED (d2dbg_status: charSelectReady==true)."""
    if not confirm:
        return {"ok": False, "error": "pass confirm=True (this launches a live game)"}
    return _http("POST", "/action/launch-character",
                 {"difficulty": int(difficulty), "confirm": True})


@mcp.tool()
def d2dbg_list_characters() -> dict:
    """List the single-player characters on the character-select screen:
    [{index, name, class}, ...]. Read-only. Use to discover available characters
    and the exact name string to pass to d2dbg_load_character. Must be at the
    character-select screen (use d2dbg_main_menu_singleplayer first if at title)."""
    return _http("GET", "/action/list-characters")


@mcp.tool()
def d2dbg_load_character(name: str, difficulty: int = 0, confirm: bool = False) -> dict:
    """Load a single-player character BY NAME at a chosen DIFFICULTY -- finds the
    named character, highlights it, and launches it (SelectCharacterByIndex on the
    game thread; the launch-mode arg IS the difficulty). difficulty: 0=Normal,
    1=Nightmare, 2=Hell (live-confirmed). The character must have that difficulty
    UNLOCKED (the real difficulty popup only offers unlocked ones). MUST be at the
    character-select screen (use d2dbg_main_menu_singleplayer first). MUTATES game
    state; set confirm=True."""
    if not confirm:
        return {"ok": False, "error": "pass confirm=True (this launches a live game)"}
    return _http("POST", "/action/load-character",
                 {"name": name, "difficulty": int(difficulty), "confirm": True})


@mcp.tool()
def d2dbg_exit_to_menu(confirm: bool = False) -> dict:
    """In-game "Save and Exit Game" -> return to the character-select menu. Calls
    D2Client's HandleSaveAndExitDialogConfirm() on the game thread (saves the
    character via the server, tears down the game). MUST be IN a game (the
    in-world pump must be firing); set confirm=True."""
    if not confirm:
        return {"ok": False, "error": "pass confirm=True (saves + leaves the game)"}
    return _http("POST", "/action/exit-to-menu", {"confirm": True})


@mcp.tool()
def d2dbg_exit_game(confirm: bool = False) -> dict:
    """Exit Diablo II -- sets D2Win's message-loop quit levers on the game thread
    so the loop exits and does not restart (the game's own clean quit). CLOSES the
    game; set confirm=True. Best invoked from a menu screen (the menu pump must be
    firing to execute the queued call)."""
    if not confirm:
        return {"ok": False, "error": "pass confirm=True (this closes the game)"}
    return _http("POST", "/action/exit-game", {"confirm": True})


if __name__ == "__main__":
    mcp.run()
