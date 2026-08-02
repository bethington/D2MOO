#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: FindMenuIndexByState
extern "C" int __stdcall FindMenuIndexByState(void)
{
	char* pnMenuClassId = (char*)D2MOO_Resolve("g_anNpcMenuClassIds");
	char* pCountBase   = (char*)D2MOO_Resolve("g_dwNpcMenuItemCount");
	char* pTargetBase  = (char*)D2MOO_Resolve("g_dwLastError_6fbc9725");
	if (!pnMenuClassId || !pCountBase || !pTargetBase)
		return 0;

	int nMenuIndex = 0;
	bool fFound = false;
	uint32_t dwCount  = *(uint32_t*)pCountBase;
	uint32_t dwTarget = *(uint32_t*)pTargetBase;

	if (0 < (int)dwCount) {
		do {
			if (fFound) break;
			if (*(uint32_t*)pnMenuClassId == dwTarget) {
				fFound = true;
			} else {
				nMenuIndex = nMenuIndex + 1;
				pnMenuClassId = pnMenuClassId + 0x16;
			}
		} while (nMenuIndex < (int)dwCount);
		if (fFound) {
			return nMenuIndex;
		}
	}
	return 0;
}
