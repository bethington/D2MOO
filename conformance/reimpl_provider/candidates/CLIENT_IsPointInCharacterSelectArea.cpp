#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_IsPointInCharacterSelectArea
extern "C" int __fastcall CLIENT_IsPointInCharacterSelectArea(
	int nArgEdx, int nArgEsi, uint32_t dwMouseX, uint32_t dwMouseY)
{
	char* pScreenWidth = (char*)D2MOO_Resolve("g_dwScreenWidth");
	char* pBitMaskTable = (char*)D2MOO_Resolve("g_dwBitMaskTable");
	if (!pScreenWidth || !pBitMaskTable)
		return -0x7FFFFFFF; // resolver missing - obvious wrong-value sentinel

	// dwBound = (int)(g_dwScreenWidth - 0x26c) / 2
	uint32_t dwBound = (int)(*(uint32_t*)pScreenWidth - 0x26c) / 2;

	// Check X bounds: dwBound + 0x12 <= dwMouseX <= dwBound + ((g_dwScreenWidth + dwBound*-2 + -0x31)/3 + 6)*3
	if (((int)(dwBound + 0x12) <= (int)dwMouseX) &&
		((int)dwMouseX <= (int)(dwBound + ((int)(*(uint32_t*)pScreenWidth + dwBound * -2 + -0x31) / 3 + 6) * 3))) {
		// nMenuYBase = (int)(g_dwBitMaskTable[0x12] - 0x1a4) / 2
		int nMenuYBase = (int)(*(int*)((char*)pBitMaskTable + 0x48) - 0x1a4) / 2;
		// Check Y bounds: nMenuYBase + 0x28 <= dwMouseY <= nMenuYBase + 0x136
		if ((nMenuYBase + 0x28 <= (int)dwMouseY) && ((int)dwMouseY <= nMenuYBase + 0x136)) {
			return 1;
		}
	}
	return 0;
}
