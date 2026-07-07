// coord_provider.cpp -- WS-1 hot-reloadable reimpl provider (spike).
//
// A STANDALONE DLL exporting the coord-family equivalents BY NAME (matching the
// bridge's D2MOO_LiveDispatch_GetName). D2Debugger LoadLibrary's this, resolves
// each dispatcher's reimpl, and calls D2MOO_LiveDispatch_SetReimpl -- so an
// equivalent can be added/replaced and shadow-proven WITHOUT restarting PD2
// (GRADUATED_CONFORMANCE_PIPELINE_PLAN.md WS-1).
//
// Functions are __stdcall (matching LiveDispatch::ReimplFn) and exported
// UNDECORATED via coord_provider.def so GetProcAddress(dll, "DUNGEON_...") works.
// These implementations are the PROVEN-correct coord formulas (== the PD2-S12
// vectors), so loading this provider yields ZERO shadow divergence -- the proof
// that the swap works. To demo a live hot-reload changing behavior, introduce a
// deliberate bug in one, rebuild JUST this DLL, and reload from D2Debugger.

// WS-6a: each exported reimpl is marked with a `D2MOO_REIMPL_EXPORT: <name>`
// line. The provider CMake target globs candidates/*.cpp and auto-generates the
// module .def from these markers -- a drafted equivalent drops in with no
// hand-editing of the .def. (See conformance/reimpl_provider/candidates/.)

extern "C" {

// D2MOO_REIMPL_EXPORT: DUNGEON_GameTileToClientCoords
// (x - y) * 80 ; ((x + y) * 80) >> 1
void __stdcall DUNGEON_GameTileToClientCoords(int* pX, int* pY)
{
	const int x = *pX, y = *pY;
	*pX = (x - y) * 80;
	*pY = ((x + y) * 80) >> 1;
}

// D2MOO_REIMPL_EXPORT: DUNGEON_GameTileToSubtileCoords
// x * 5 ; y * 5
void __stdcall DUNGEON_GameTileToSubtileCoords(int* pX, int* pY)
{
	*pX = *pX * 5;
	*pY = *pY * 5;
}

// D2MOO_REIMPL_EXPORT: DUNGEON_ClientToGameCoords
// 2y + x ; 2y - x
void __stdcall DUNGEON_ClientToGameCoords(int* pX, int* pY)
{
	const int x = *pX, y = *pY;
	*pX = 2 * y + x;
	*pY = 2 * y - x;
}

// D2MOO_REIMPL_EXPORT: DUNGEON_GameToClientCoords
// (x - y) >> 1 ; (x + y) >> 2   (arith shift -- matches PD2's SAR, incl. negatives)
void __stdcall DUNGEON_GameToClientCoords(int* pX, int* pY)
{
	const int x = *pX, y = *pY;
	*pX = (x - y) >> 1;
	*pY = (x + y) >> 2;
}

// D2MOO_REIMPL_EXPORT: DUNGEON_GameSubtileToClientCoords
// (x - y) * 16 ; (x + y) * 8
void __stdcall DUNGEON_GameSubtileToClientCoords(int* pX, int* pY)
{
	const int x = *pX, y = *pY;
	*pX = (x - y) * 16;
	*pY = (x + y) * 8;
}

}
