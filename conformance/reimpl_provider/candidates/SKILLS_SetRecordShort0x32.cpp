#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SKILLS_SetRecordShort0x32
extern "C" void __stdcall SKILLS_SetRecordShort0x32(int nSkillId, uint16_t wValue)
{
	void* pMonStatsPtr = D2MOO_Resolve("g_pMonStatsTxt");
	void* pCountPtr = D2MOO_Resolve("g_pLevelTileFastEntriesBuffer");

	if (!pMonStatsPtr || !pCountPtr)
		return; // resolver missing -> obvious mismatch

	char* pMonStats = (char*)(*(void**)pMonStatsPtr);
	int count = (int)(*(void**)pCountPtr);

	if (!pMonStats)
		return; // original would _exit(-1) here; we just no-op

	if (nSkillId > -1 && nSkillId < count)
	{
		// (&g_pMonStatsTxt->wVelocity)[nSkillId * 0xd4] = wValue
		// ushort* indexing: wVelocity is at byte offset 0x32, stride = 0xd4 ushorts = 0x1A8 bytes
		*(uint16_t*)(pMonStats + 0x32 + nSkillId * 0x1A8) = wValue;
	}
}
