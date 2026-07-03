# D2MOO conformance harness (vs PD2-S12)

Replays golden vectors minted from the **real Project Diablo 2 S12** (1.13c-base)
binary against D2MOO's reimplementation, asserting a **bit-exact** match. This is
the piece D2MOO otherwise lacks: automated conformance against a *specific target
binary*.

## Why

D2MOO's logic is 1.10f-derived. For 1.13c / PD2-S12, individual functions may
differ, and PD2's own changes aren't present. Each `vectors/*.json` file is ground
truth captured from the **live PD2-S12 process** via the ghidra-mcp in-process
`call_function` oracle (call the real D2Common function with chosen inputs, record
the output). A **passing** function is a proven-conformant drop-in candidate — flip
its `D2.Detours` patch to `FunctionReplaceOriginalByPatch`. A **failing** function
is a real 1.10f→1.13c divergence to fix.

## Run

```sh
./build.sh                    # builds (g++) + runs vectors/rng.json
./build.sh vectors/foo.json   # a specific vector file
```

On MSVC (D2MOO's native toolchain) compile `d2moo_conform.cpp` against
`source/D2Common/include` directly — the MSVC `i64` literals compile natively (the
`build.sh` normalization is only for g++/clang).

## Adding a function (the burn-down loop)

1. Mint PD2-S12 vectors for the target ordinal via the ghidra-mcp `call_function`
   oracle → `vectors/NAME.json`, an array of `{ "fn": "...", "in": {...}, "out": {...} }`.
2. Add a dispatch case in `d2moo_conform.cpp` that calls D2MOO's function.
3. Run. Progress burns down D2Common's 1,211 ordinals toward a proven drop-in.

## Status

- **RNG: 20/20** — D2MOO's `SEED_RollLimitedRandomNumber` (`D2Common/include/D2Seed.h`)
  and the `nMin + SEED_RollLimitedRandomNumber(range)` range idiom are bit-exact
  with the PD2-S12 `D2Seed_Random` / `D2Seed_RandomInRange` vectors.
