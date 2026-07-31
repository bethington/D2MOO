#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DRLG_ProcessTileCoords
// NEEDS GLOBAL: g_anTileCoord4
// NEEDS GLOBAL: g_anTileCoord5
// NEEDS GLOBAL: g_anTileCoord2
// NEEDS GLOBAL: g_anTileCoord0
// NEEDS GLOBAL: g_dwTileCoord3Offset
// NEEDS GLOBAL: g_dwTileCoord2Offset
// NEEDS GLOBAL: g_dwTileCoord0Offset
// NEEDS GLOBAL: g_pTileCoordTbl
// NEEDS GLOBAL: g_dwTile3Dst
// NEEDS GLOBAL: g_dwTile2Dst
// NEEDS GLOBAL: g_dwTile1Dst
// NEEDS GLOBAL: g_dwTile0Dst

extern "C" void __stdcall DRLG_ProcessTileCoords(void* this, uint32_t nIteration, int nCount)
{
	(void)this; // unused in body

	// Pointer variables (deref once to get the pointer value)
	char* g_anTileCoord4 = *(char**)D2MOO_Resolve("g_anTileCoord4");
	char* g_anTileCoord5 = *(char**)D2MOO_Resolve("g_anTileCoord5");
	char* g_anTileCoord2 = *(char**)D2MOO_Resolve("g_anTileCoord2");
	char* g_anTileCoord0 = *(char**)D2MOO_Resolve("g_anTileCoord0");

	// Scalar dword targets (write through)
	uint32_t* g_dwTileCoord3Offset = (uint32_t*)D2MOO_Resolve("g_dwTileCoord3Offset");
	uint32_t* g_dwTileCoord2Offset = (uint32_t*)D2MOO_Resolve("g_dwTileCoord2Offset");
	uint32_t* g_dwTileCoord0Offset = (uint32_t*)D2MOO_Resolve("g_dwTileCoord0Offset");

	// Array bases (address of the first element; indexed by byte/dword)
	char* g_pTileCoordTbl = (char*)D2MOO_Resolve("g_pTileCoordTbl");
	uint32_t* g_dwTile3Dst = (uint32_t*)D2MOO_Resolve("g_dwTile3Dst");
	uint32_t* g_dwTile2Dst = (uint32_t*)D2MOO_Resolve("g_dwTile2Dst");
	uint32_t* g_dwTile1Dst = (uint32_t*)D2MOO_Resolve("g_dwTile1Dst");
	uint32_t* g_dwTile0Dst = (uint32_t*)D2MOO_Resolve("g_dwTile0Dst");

	if (!g_anTileCoord4 || !g_anTileCoord5 || !g_anTileCoord2 || !g_anTileCoord0 ||
		!g_dwTileCoord3Offset || !g_dwTileCoord2Offset || !g_dwTileCoord0Offset ||
		!g_pTileCoordTbl || !g_dwTile3Dst || !g_dwTile2Dst || !g_dwTile1Dst || !g_dwTile0Dst)
		return;

	for (; nCount != 0; nCount = nCount + -1) {
		*g_dwTileCoord3Offset = g_dwTile3Dst[*(uint8_t*)g_anTileCoord4];
		*g_dwTileCoord2Offset =
			g_dwTile2Dst[*(uint8_t*)g_anTileCoord4] + g_dwTile1Dst[*(uint8_t*)g_anTileCoord5];
		*g_dwTileCoord0Offset = g_dwTile0Dst[*(uint8_t*)g_anTileCoord5];

		uint8_t bTileIdx = *(uint8_t*)g_anTileCoord2;
		g_anTileCoord2 = g_anTileCoord2 + 1;

		uint32_t* pTileCoords = *(uint32_t**)(g_pTileCoordTbl + bTileIdx * 4);

		*(char*)g_anTileCoord0 = (char)pTileCoords[*g_dwTileCoord3Offset];
		*(char*)(g_anTileCoord0 + 1) = (char)pTileCoords[*g_dwTileCoord2Offset];
		*(char*)(g_anTileCoord0 + 2) = (char)pTileCoords[*g_dwTileCoord0Offset];
		*(char*)(g_anTileCoord0 + 3) = (char)pTileCoords[*g_dwTileCoord3Offset];
		*(char*)(g_anTileCoord0 + 4) = (char)pTileCoords[*g_dwTileCoord2Offset];
		*(char*)(g_anTileCoord0 + 5) = (char)pTileCoords[*g_dwTileCoord0Offset];

		g_anTileCoord0 = g_anTileCoord0 + 6;

		nIteration = nIteration + 1;
		if ((nIteration & 1) == 0) {
			g_anTileCoord4 = g_anTileCoord4 + 1;
			g_anTileCoord5 = g_anTileCoord5 + 1;
		}
	}
}
