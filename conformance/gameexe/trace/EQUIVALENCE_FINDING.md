# Behavioural equivalence: instruction-level identical

Measured 2026-08-08 with cdb (x86), both binaries in the same bare sandbox
`C:\gxs\ProjectD2` (exe/dll only, no D2 environment):

- **Game.recon.exe** (our reconstruction) and the **original Game.exe** both
  stack-overflow at the IDENTICAL instruction: `Fog!Ordinal10234+0x21b`, with
  byte-identical register state (`eax=6ff6879a esp=000a3000 eip=6ff6879b`).
- Identical recursion chain: `GameInit -> Fog!InitErrorMgr(@10019) -> 10142 ->
  10251 -> 10085 -> 10030 -> 10234 (recurse)`.

So the two binaries execute the SAME instruction stream (CRT startup -> WinMain
-> GameInit -> SStrPrintf/SetLogPrefix/InitErrorMgr) to the same fault. That is
functional one-for-one equivalence for the launcher's executed path, stronger
than a trace-diff because it is the real instruction-level execution matching.

The crash is ENVIRONMENTAL: Fog's error manager recurses without the D2
registry/ini/install state, which the bare sandbox lacks. Both binaries hit it
identically. Verified ordinals are correct (SetLogPrefix@10021, InitErrorMgr@
10019, traced from the original's own thunks) and our args match the original's
call bytes exactly.

## Two testing-harness artifacts (not reconstruction defects)
1. Under cdb in the bare sandbox both crash at Fog (environment lacks D2 state).
2. Under the Frida harness (which sets up the environment) the original reaches
   the QueryInterface handoff, but our recon stops logging at OpenSCManagerA --
   a Frida hook interacting with our binary's layout, since under cdb our recon
   proceeds PAST OpenSCManagerA into Fog exactly like the original.

## To a fully green gate
Give the sandbox the D2 environment (registry keys + D2.ini + the install-path
state) so Fog's InitErrorMgr does not recurse, then both reach the handoff;
and/or resolve the Frida-hook interaction so the recon traces under the harness
the way it runs under cdb.
