# d2debugger-mcp

An MCP bridge to the **live** D2Debugger control surface inside a running
Project Diablo 2 process. WS-5 of
[../GRADUATED_CONFORMANCE_PIPELINE_PLAN.md](../GRADUATED_CONFORMANCE_PIPELINE_PLAN.md) —
the keystone that lets an AI agent drive live shadow-proving with **no human at
the ImGui panel**.

It is a thin MCP↔HTTP multiplexer (sibling in spirit to `ghidra-mcp`) over the
in-process HTTP server that `D2Debugger.mcp.cpp` runs on `127.0.0.1:8790`.

## How it fits together

```
 agent (MCP client)                    game process (elevated)
 ─────────────────                     ───────────────────────
 d2debugger-mcp  ──HTTP─▶  D2Debugger.mcp.cpp (127.0.0.1:8790)
 (server.py)               └─▶ D2Mcp_HandleRequest (D2Debugger.LiveDispatch.cpp)
                                 ├─ live dispatchers (mode / hits / divergences)
                                 ├─ reimpl-provider hot-reload (WS-1, MemoryModule)
                                 ├─ whole-engine profiler (per-subsystem hooks)
                                 └─ DIRECT-CALL ORACLE (call original + reimpl,
                                    compare on chosen input vectors)
```

A non-elevated MCP client can reach the elevated game's **loopback** socket —
TCP loopback is not gated by elevation — so no special privileges are needed.

## Prerequisites

- PD2 running with the D2MOO patch DLLs staged and `D2_DEBUGGER=1` (e.g. via
  `conformance/behavioral/run-phase4-panel-check.ps1`). Confirm with:
  `curl http://127.0.0.1:8790/status`
- Python ≥ 3.10 with the `mcp` package. The repo's `ghidra-mcp` venv already has
  it: `C:\Users\benam\source\mcp\ghidra-mcp\.venv`.

## Run standalone (stdio transport)

```sh
python server.py
```

Override the endpoint with `D2DBG_MCP_URL` (default `http://127.0.0.1:8790`).

## Register in an MCP host (Claude Code)

```sh
claude mcp add d2debugger -- \
  C:/Users/benam/source/mcp/ghidra-mcp/.venv/Scripts/python.exe \
  C:/Users/benam/source/cpp/D2MOO/conformance/d2debugger_mcp/server.py
```

Or add to the host's MCP config JSON:

```json
{
  "mcpServers": {
    "d2debugger": {
      "command": "C:/Users/benam/source/mcp/ghidra-mcp/.venv/Scripts/python.exe",
      "args": ["C:/Users/benam/source/cpp/D2MOO/conformance/d2debugger_mcp/server.py"],
      "env": { "D2DBG_MCP_URL": "http://127.0.0.1:8790" }
    }
  }
}
```

## Tools

| Tool | What it does |
|------|--------------|
| `d2dbg_status` | Bridge/dispatcher/provider/profiler health. Call first. |
| `d2dbg_list_dispatchers` | Every dispatcher: index, name, offset, mode, hits, divergences. |
| `d2dbg_set_mode(index, mode)` | Set one dispatcher: `original` / `reimpl` / `shadow`. |
| `d2dbg_set_all_modes(mode)` | Set all dispatchers at once. |
| `d2dbg_reload_provider()` | Hot-reload the reimpl-provider DLL (WS-1), modes preserved. |
| `d2dbg_call_oracle(index, vectors=… \| x=,y=)` | Coord-family oracle — call raw original + bound reimpl on chosen inputs, compare. |
| `d2dbg_prove_spec(spec)` | **General arbitrary-ABI oracle** (detail B) — prove ANY reimpl from an ABI spec (name/callconv/args/compare/vectors). |
| `d2dbg_list_subsystems()` | Profiler subsystems: counts, hits, installed flag. |
| `d2dbg_instrument(subsystem)` | Install count-hooks for one subsystem. |
| `d2dbg_list_functions(subsystem)` | Functions in a subsystem: ordinal/name/offset/hits/hooked/hasEquiv. |

## The autonomous proving loop (proven 2026-07-06)

1. Build a candidate reimpl into the provider DLL.
2. `d2dbg_reload_provider()` — binds it live, no game restart.
3. `d2dbg_call_oracle(index, vectors=[…])` — deterministic per-vector verdict.
4. `allMatch: true` over a good vector set ⇒ the equivalent matches the game.

Verified end-to-end: a correct provider returns `6/6 allMatch`; a deliberately
`+1`-buggy provider is flagged `0/4` on the buggy function while an untouched
function stays `4/4` — the oracle is discriminating, not trivially green.

### Headless one-shot: `prove_candidate.py`

The same loop as a single non-interactive command (for CI or a fun-doc worker) —
exits `0` iff every vector matched:

```sh
# rebuild + stage + hot-reload the provider, then prove with an auto vector spread
python ../tools/prove_candidate.py --dispatcher DUNGEON_GameToClientCoords --build --vectors auto
```

See [../tools/prove_candidate.py](../tools/prove_candidate.py). Flags: `--dispatcher`
(index or name), `--vectors auto|<file.json>`, `--build`, `--no-reload`, `--url`,
`--json`.

> **ABI note:** the current dispatchers are the coord family
> (`void __stdcall(int*, int*)`), so oracle inputs/outputs are the `(x, y)` pair.
> Arbitrary-ABI marshalling is design detail B in the plan (deferred).
