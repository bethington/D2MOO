// d2moo_fingerprint — the D2MOO (game-free) half of the Tier-2 behavioral matcher.
//
// Calls D2MOO's REAL compiled functions (linked from the D2Common target) with a
// fixed set of diverse inputs and emits a JSON "behavioral fingerprint":
//   { "<fn>": [ {"in":[...], "out":[...]}, ... ], ... }
//
// The matching side (future) captures the SAME fingerprint for each candidate
// PD2-S12 export ordinal via the in-process call_function oracle in the running
// game (PD2 DLLs cannot be loaded standalone — DllMain needs the game bootstrap,
// err=1114), then finds the PD2 ordinal whose fingerprint is bit-identical to a
// D2MOO function's. Bit-exact across all inputs ⇒ PROVEN match (see EMULATION_
// CONFORMANCE_PLAN §17, Tier 2).
//
// This MVP covers the coord-family signature `void __stdcall(int* pX, int* pY)`.
// Add more shapes (RNG, 1-int, 2-int→int) as probe tables below.

#include <cstdio>
#include <D2Dungeon.h>

// Diverse inputs — include negatives + large values so signed-shift vs divide,
// overflow, and offset differences all surface (the coord GameToClient bug this
// session hinged on negative deltas).
static const int kInputs[][2] = {
    {0, 0}, {1, 0}, {0, 1}, {5, 3}, {-7, 2}, {10, -4},
    {100, 20}, {-1, -2}, {3, 7}, {-4, -8}, {255, 1}, {-100, 100},
};
static const int kNumInputs = sizeof(kInputs) / sizeof(kInputs[0]);

typedef void(__stdcall* CoordFn)(int*, int*);
struct Probe { const char* name; CoordFn fn; };

static const Probe kCoordProbes[] = {
    {"DUNGEON_GameTileToClientCoords",    DUNGEON_GameTileToClientCoords},
    {"DUNGEON_GameTileToSubtileCoords",   DUNGEON_GameTileToSubtileCoords},
    {"DUNGEON_ClientToGameCoords",        DUNGEON_ClientToGameCoords},
    {"DUNGEON_GameToClientCoords",        DUNGEON_GameToClientCoords},
    {"DUNGEON_GameSubtileToClientCoords", DUNGEON_GameSubtileToClientCoords},
};
static const int kNumCoordProbes = sizeof(kCoordProbes) / sizeof(kCoordProbes[0]);

int main() {
    printf("{\n");
    for (int p = 0; p < kNumCoordProbes; ++p) {
        printf("  \"%s\": [", kCoordProbes[p].name);
        for (int i = 0; i < kNumInputs; ++i) {
            int x = kInputs[i][0], y = kInputs[i][1];
            kCoordProbes[p].fn(&x, &y);
            printf("%s{\"in\":[%d,%d],\"out\":[%d,%d]}",
                   i ? "," : "", kInputs[i][0], kInputs[i][1], x, y);
        }
        printf("]%s\n", p + 1 < kNumCoordProbes ? "," : "");
    }
    printf("}\n");
    return 0;
}
