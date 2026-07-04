# Behavioral conformance oracle: PD2-S12 vs D2MOO

Prove D2MOO's reimplementation is bit-exact with the **real running Project
Diablo 2 (PD2-S12)** binary by calling PD2's own functions in-process (Frida)
with chosen inputs and diffing the outputs against D2MOO.

This is the **in-process** approach. The external ghidra-mcp debugger (:8099)
fought a wall of WOW64 fragility (anti-debug INT3 flood, comtypes COM crashes,
can't-hijack-a-parked-thread); going in-process collapses all of it — no
debugger to detect, run on the game's own threads, call functions directly.

## Proven so far (all bit-exact PD2 == D2MOO)

| Family | Functions | Calls | Notes |
|---|---|---|---|
| **Coord** | 5 × `DUNGEON_*Coords` | 60 | pure `void __stdcall(int*,int*)`, direct call |
| **RNG** | `SEED_GetRandomNumber`, `...Alt`, `...InRange` | 135 | LCG `0x6AC690C5`; 3 different register ABIs via trampoline |

Coord: `pd2_fp.json` == `d2moo_fp.json` (`compare_fp.py`).
RNG: `pd2_rng_family.json`, every call checked vs the D2MOO/`D2Seed.h` formula.

## The toolchain (reusable)

1. **Ghidra bridge (`:8089`)** — get the VERIFIED PD2-S12 address + signature +
   ABI for a function. D2MOO header VAs are *vanilla* and wrong for PD2;
   ordinals are scrambled. `switch_program?program=D2Common.dll`, then
   `get_function_by_address` / `decompile_function` (the decompiler even
   annotates register-passed args). Offsets are `GhidraVA - 0x6FD50000`
   (D2Common loads at its preferred base).
2. **Frida in-process** — `frida.attach("Game.exe")`, resolve `D2Common.dll`
   base at runtime (ASLR-proof), `NativeFunction(base.add(off), ...)`.
3. **Trampoline** for Blizzard register ABIs (args in EAX/ESI/EDI, not
   stack/ECX-EDX) — a tiny raw-machine-code stub sets the exact registers and
   `call`s the verified entry. Preserve callee-saved regs (esi/edi) with
   push/pop. RETAIN stub pointers (Frida GCs `Memory.alloc`). See
   `pd2_frida_rng_family.js`.
4. **Safety gate** — verify the FIRST call vs the reference (or catch a
   Frida-trapped fault) BEFORE capturing. Never call a heuristic/unverified
   entry: a wrong address AVs (Frida traps it, game survives) but a slightly-
   wrong one runs real code mid-function and CRASHES the game.

## Files

- `start-frida-oracle.ps1` — self-elevating launcher (PD2 runs elevated, so
  Frida must too; one UAC). Launches the game + runs a capture.
- Coord: `pd2_frida_agent.js` + `pd2_frida_capture.py` -> `pd2_fp.json`;
  `d2moo_fp.json` from `../../source/D2Common/tests` fingerprint; `compare_fp.py`.
- RNG: `pd2_frida_rng_family.js` + `pd2_frida_rng_family.py` -> `pd2_rng_family.json`
  (the general, current one). `pd2_rng_probe/trampoline/vectors/confirm` are the
  earlier single-function + read-only steps kept for reference.
- `behav_match.py` — match a query fingerprint to candidates by bit-exact output
  (for resolving unknown ordinals; not needed when the name is known).

## Add a function (recipe)

1. Bridge: `get_function_by_address` / `decompile_function` -> verified entry
   offset (`VA - 0x6FD50000`) + exact ABI (watch for register-passed args).
2. Agent: add a `NativeFunction` (direct if stack/ECX-EDX ABI) or a trampoline
   (register ABI). Retain the stub pointer.
3. Driver: add the reference (D2MOO source formula), input vectors, and a
   first-call safety gate; capture + assert bit-exact.
4. Run via `start-frida-oracle.ps1` (elevated) or attach directly.

## Frontier (not yet done)

- **Data-table functions** (`DATATBLS_*`, most `ITEMS_*ByItemId`, `DRLG_*`):
  callable in PD2 (tables loaded live), but the D2MOO side needs the SAME data
  tables loaded to compare — a bounded harness/build task.
- **Combat / drops** (D2Game): state-dependent; needs an in-game session and
  likely a per-frame `Interceptor` hook to run on the game thread.
