# MemoryModule (vendored)

Source: https://github.com/fancycode/MemoryModule (v0.0.4)
License: Mozilla Public License 2.0 (MPL-2.0) -- file-level copyleft. These two
files retain their MPL-2.0 headers; the rest of D2MOO stays MIT. Do not strip the
license header. Modifications to MemoryModule.c/.h must be shared under MPL-2.0.

Used by D2Debugger's reimpl-provider hot-reload (WS-1) to load the provider DLL
from an in-memory buffer with a CUSTOM import resolver -- so provider imports of
D2Common are resolved by VERIFIED ADDRESS (D2MOO_ResolveGameFn), never the
scrambled export table. See conformance/GRADUATED_CONFORMANCE_PIPELINE_PLAN.md
detail A2.
