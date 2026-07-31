#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetGameSessionInfo

extern "C" void LeaveCriticalSectionValidated(void);
extern "C" void LeaveCriticalSection(void *lpCriticalSection);
extern "C" void *AcquireGameSession(void);
extern "C" void *FindUnitByTypeAndId(int nGame, uint32_t dwTypeId);
extern "C" void CopyUnitInfoToBuffer(void);
extern "C" int IsBadCodePtr(void *lpfn);

extern "C" int __stdcall GetGameSessionInfo(int nUnused, int nCount, uint32_t dwParam3, uint32_t *pUnitTypeIdArray, int nArraySize)
{
	uint32_t *nGame;
	uint32_t nOffset;
	void *pUnit;
	uint32_t fIsBadPtr;
	uint32_t dwIndex;

	// NEEDS GLOBAL: g_pDataSyncClientHashTable
	// NEEDS GLOBAL: PTR_ProcessGameRoutine_b600_6fd2e3d0

	LeaveCriticalSectionValidated();
	// LeaveCriticalSection((void *)((char *)g_pDataSyncClientHashTable + 0xc));

	nGame = (uint32_t *)AcquireGameSession();
	nUnused = 0;
	if (nGame == (uint32_t *)0) {
		return 0;
	}

	dwIndex = 0;
	if (0 < nCount) {
		nOffset = (uint32_t)(nArraySize - (int)pUnitTypeIdArray);
		do {
			pUnit = FindUnitByTypeAndId((int)nGame, *(uint32_t *)(nOffset + (int)pUnitTypeIdArray));
			if (pUnit != (void *)0) {
				CopyUnitInfoToBuffer();
				if (*pUnitTypeIdArray != 0) {
					// fIsBadPtr = IsBadCodePtr((void *)((unsigned int *)PTR_ProcessGameRoutine_b600_6fd2e3d0)[*pUnitTypeIdArray]);
					// if (fIsBadPtr != 0) {
					// 	return 0;
					// }
					// ((void (*)(void))((unsigned int *)PTR_ProcessGameRoutine_b600_6fd2e3d0)[*pUnitTypeIdArray])();
				}
				nUnused = nUnused + 1;
			}
			dwIndex = dwIndex + 1;
			pUnitTypeIdArray = pUnitTypeIdArray + 1;
		} while ((int)dwIndex < nCount);
	}

	if ((void *)nGame[6] != (void *)0) {
		LeaveCriticalSection((void *)nGame[6]);
		return nUnused;
	}

	return 0;
}
