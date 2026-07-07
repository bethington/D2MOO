# Reimpl-provider candidates (WS-6a)

Drop a drafted D2MOO reimplementation here as a `.cpp` file and it is
automatically compiled into `D2MOO_ReimplProvider.dll` and exported by name —
no CMake edit, no `.def` edit.

## Contract

- Define each equivalent as an `extern "C"` function with the **same name** as
  the game function (so the oracle's `GetProcAddress(provider, "Name")` and the
  live original resolve to the same symbol), and the correct calling convention
  (`__stdcall` / `__fastcall` / `__cdecl` / `__thiscall`).
- Mark every exported function with a marker line the build reads:

  ```cpp
  // D2MOO_REIMPL_EXPORT: SOME_Function
  int __fastcall SOME_Function(int a) { ... }
  ```

- The provider CMake target globs `candidates/*.cpp` with `CONFIGURE_DEPENDS`
  and regenerates the module `.def` from every `D2MOO_REIMPL_EXPORT:` marker, so
  adding/removing a file re-globs on the next build.

## Prove it live

```sh
# rebuild + stage + hot-reload the provider, then prove against the live game
python ../../tools/prove_candidate.py --spec ../../vectors/<your>.spec.json --build
```

The spec (`name`, `callconv`, `args`, `compare`, `vectors`) is exactly what a
fun-doc port worker already emits from its `param_layout` / `input_sets`.
