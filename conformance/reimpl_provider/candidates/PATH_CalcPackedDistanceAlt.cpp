#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: PATH_CalcPackedDistanceAlt
extern "C" int __stdcall PATH_CalcPackedDistanceAlt(uint32_t dwCoord1, uint32_t dwCoord2)
{
	// Pure arithmetic -- no global state read. The function only extracts X (lo 16)
	// and Y (hi 16) from each packed uint coordinate, takes absolute differences, and
	// returns 2*max(dX,dY) + min(dX,dY) (the octile/diagonal heuristic).
	int nDiffX = (int)((dwCoord1 & 0xFFFFu) - (dwCoord2 & 0xFFFFu));
	if (nDiffX < 0) {
		nDiffX = -nDiffX;
	}
	int nDiffY = (int)((dwCoord1 >> 16) - (dwCoord2 >> 16));
	if (nDiffY < 0) {
		nDiffY = -nDiffY;
	}
	if (nDiffY <= nDiffX) {
		return nDiffY + nDiffX * 2;
	}
	return nDiffX + nDiffY * 2;
}
