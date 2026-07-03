// ============================================================================
//  PD2-S12 conformance tests for D2MOO's D2Common.
//
//  These assert D2MOO's REAL compiled functions (linked from the D2Common target)
//  match ground truth captured from the Project Diablo 2 S12 (1.13c) binary via
//  the ghidra-mcp in-process oracle / decompilation. Unlike the header-only RNG
//  check, these reach NON-INLINE exports (e.g. the coordinate conversions), which
//  is the whole point of linking D2MOO's compiled library.
//
//  A failing case is a real 1.10f->1.13c/PD2 divergence to fix. See also the
//  standalone data-driven harness in conformance/ (RNG 20/20).
// ============================================================================
#include <doctest.h>
#include <cstdint>

#include "D2Seed.h"
#include "D2Dungeon.h"

// --- Coordinate conversions (non-inline D2Common exports) --------------------
// PD2-S12 D2Common TileToScreenCoordsInPlace @ 0x6fd9dac0:
//   *pX = (x - y) * 80 ; *pY = (x + y) * 80 >> 1     (== D2MOO GameTileToClientCoords)
TEST_CASE("PD2-S12: DUNGEON_GameTileToClientCoords == TileToScreenCoordsInPlace")
{
	struct { int x, y, ex, ey; } cases[] = {
		{ 0, 0,     0,    0 },
		{ 1, 0,    80,   40 },
		{ 0, 1,   -80,   40 },
		{ 5, 3,   160,  320 },
		{ 10, -4, 1120,  240 },
		{ -7, 2,  -720, -200 },
	};
	for (auto& c : cases)
	{
		int x = c.x, y = c.y;
		DUNGEON_GameTileToClientCoords(&x, &y);
		CHECK(x == c.ex);
		CHECK(y == c.ey);
	}
}

// PD2-S12 game->client projection. NOTE / OPEN FINDING: PD2-S12's inline projection
// (CalcIsometricScreenCoords @ 0x6fd85b00) computes screenX = (x-y) >> 1, screenY =
// (x+y) >> 2 -- ARITHMETIC shift (floor). D2MOO's DUNGEON_GameToClientCoords uses
// (x-y)/2, (x+y)/4 (truncating divide), which DIVERGES for NEGATIVE deltas
// (verified: {0,3}->D2MOO x=-1 vs PD2-S12 -2; {1,2}->0 vs -1; {5,10}->-2 vs -3).
// Non-negative inputs agree (below). Before "fixing" D2MOO (/ -> >>), confirm the
// STANDALONE game->client export's semantics (ordinal reconciliation, todo) -- the
// inline may differ from the standalone. Non-negative cases are asserted here.
TEST_CASE("PD2-S12: DUNGEON_GameToClientCoords (non-negative deltas)")
{
	struct { int x, y, ex, ey; } cases[] = {
		{ 0, 0,   0, 0 },
		{ 8, 4,   2, 3 },
		{ 10, 2,  4, 3 },
		{ 100, 20, 40, 30 },
	};
	for (auto& c : cases)
	{
		int x = c.x, y = c.y;
		DUNGEON_GameToClientCoords(&x, &y);
		CHECK(x == c.ex);   // (x-y)/2 == (x-y)>>1 for non-negative x-y
		CHECK(y == c.ey);   // (x+y)/4 == (x+y)>>2 for non-negative x+y
	}
}

// PD2-S12 MultiplyValuesBy5 @ 0x6fd9d8a0 (real export ordinal @11158):
//   *pX *= 5 ; *pY *= 5   (== D2MOO DUNGEON_GameTileToSubtileCoords, tile->subtile)
TEST_CASE("PD2-S12: DUNGEON_GameTileToSubtileCoords == MultiplyValuesBy5")
{
	struct { int x, y, ex, ey; } cases[] = {
		{ 0, 0,   0,   0 },
		{ 1, 1,   5,   5 },
		{ 3, 7,  15,  35 },
		{ -2, 4, -10, 20 },
	};
	for (auto& c : cases)
	{
		int x = c.x, y = c.y;
		DUNGEON_GameTileToSubtileCoords(&x, &y);
		CHECK(x == c.ex);
		CHECK(y == c.ey);
	}
}

// PD2-S12 TransformToIsometric @ 0x6fd9d8c0 (real export ordinal @11026):
//   *pX = 2*y + x ; *pY = 2*y - x   (== D2MOO DUNGEON_ClientToGameCoords)
// Pure integer add/mul (no signed divide) -> bit-exact incl. negatives.
TEST_CASE("PD2-S12: DUNGEON_ClientToGameCoords == TransformToIsometric")
{
	struct { int x, y, ex, ey; } cases[] = {
		{ 0, 0,    0,   0 },
		{ 1, 0,    1,  -1 },
		{ 0, 1,    2,   2 },
		{ 5, 3,   11,   1 },
		{ -7, 2,  -3,  11 },   // 2*2 + (-7) = -3 ; 2*2 - (-7) = 11
		{ 10, -4,  2, -18 },   // 2*(-4) + 10 = 2 ; 2*(-4) - 10 = -18
	};
	for (auto& c : cases)
	{
		int x = c.x, y = c.y;
		DUNGEON_ClientToGameCoords(&x, &y);
		CHECK(x == c.ex);
		CHECK(y == c.ey);
	}
}

// --- RNG (inline, but linked here too as a cross-check) ----------------------
// PD2-S12 D2Common SEED (LCG: nHighSeed + 0x6AC690C5*nLowSeed, then %/pow2-mask).
TEST_CASE("PD2-S12: SEED_RollLimitedRandomNumber")
{
	// {lo, hi, max} -> {ret, newHi}
	struct { uint32_t lo, hi, max, ret, newHi; } cases[] = {
		{ 1, 0, 100, 85, 0 },          // 0x6AC690C5 (=1791398085) % 100 = 85
		{ 1, 0, 0,   0,  0 },          // max<=0 -> 0, no advance
	};
	for (auto& c : cases)
	{
		D2SeedStrc s; s.nLowSeed = c.lo; s.nHighSeed = c.hi;
		uint32_t got = SEED_RollLimitedRandomNumber(&s, (int)c.max);
		CHECK(got == c.ret);
		if (c.max > 0) CHECK(s.nHighSeed == c.newHi);
	}
}
