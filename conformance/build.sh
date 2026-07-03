#!/usr/bin/env bash
# Quick conformance check on g++/clang (non-MSVC). D2MOO's D2Seed.h uses the MSVC
# i64 literal suffix and pulls in D2BasicTypes.h/D2Common.h; this normalizes the
# suffix + stubs those two headers, then builds and runs. On MSVC (D2MOO's native
# toolchain) just compile d2moo_conform.cpp against source/D2Common/include.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
INC="$HERE/../source/D2Common/include"
B="$HERE/.build"; mkdir -p "$B"
printf '#pragma once\n#include <cstdint>\n' > "$B/D2BasicTypes.h"
printf '#pragma once\n#ifndef D2COMMON_DLL_DECL\n#define D2COMMON_DLL_DECL\n#endif\n' > "$B/D2Common.h"
sed -E 's/([0-9A-Fa-fx]+)i64/\1LL/g' "$INC/D2Seed.h" > "$B/D2Seed.h"
g++ -std=c++17 -I"$B" "$HERE/d2moo_conform.cpp" -o "$B/d2moo_conform.exe"
( cd "$HERE" && "$B/d2moo_conform.exe" "$@" )
