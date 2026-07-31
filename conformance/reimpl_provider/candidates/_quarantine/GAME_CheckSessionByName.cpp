#include "../provider_runtime.h"

// Helper functions (provided by provider)
extern "C" void* AcquireGameSession();
extern "C" int CompareStringsIgnoreCase(const char* s1, const char* s2);

// D2MOO_REIMPL_EXPORT: GAME_CheckSessionByName
// NEEDS GLOBAL: g_pDataSyncClientHashTable
extern "C" uint32_t __stdcall GAME_CheckSessionByName(const char* lpszSessionName)
{
	// g_pDataSyncClientHashTable is a pointer variable (g_p prefix) => deref once.
	char* pBase = (char*)*(void**)D2MOO_Resolve("g_pDataSyncClientHashTable");
	if (!pBase) return 1; // sentinel: not found

	int* pSlot = (int*)(pBase + 0x14);

	// Resolve LeaveCriticalSection function pointer from global.
	typedef void (*LeaveCriticalSectionFn)(void*);
	LeaveCriticalSectionFn pLeaveCS = (LeaveCriticalSectionFn)*(void**)D2MOO_Resolve("g_pfnLeaveCritSec");
	if (!pLeaveCS) return 1;

	for (;;) {
		int slotVal = *pSlot;
		if (slotVal != 0 && slotVal != -1) {
			uint32_t* puVar1 = (uint32_t*)AcquireGameSession();
			if (puVar1 != NULL) {
				const char* sessionName = (const char*)((char*)puVar1 + 0x2a);
				int nResult = CompareStringsIgnoreCase(lpszSessionName, sessionName);
				void* lpCriticalSection = (void*)puVar1[6];
				if (nResult == 0) {
					if (lpCriticalSection != NULL) {
						pLeaveCS(lpCriticalSection);
						return 0;
					}
					return 0; // abort path: CS missing for matching session
				}
				if (lpCriticalSection == NULL) {
					return 0; // abort path: CS missing for non-matching session
				}
				pLeaveCS(lpCriticalSection);
			}
		}
		pSlot++;
		if ((int)pSlot > 0x6FD31C07) {
			return 1;
		}
	}
}
