#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: QUEST_UpdateQuestMenuCachedState
extern "C" void __fastcall QUEST_UpdateQuestMenuCachedState(int /*ECX*/, int in_EDX)
{
	char* anQuest   = (char*)D2MOO_Resolve("g_anQuestLookupTbl");
	char* abColorR  = (char*)D2MOO_Resolve("g_abQuestTooltipColorR");
	char* abVis     = (char*)D2MOO_Resolve("g_abQuestTooltipEntryVisibility");
	char* dwCached  = (char*)D2MOO_Resolve("g_dwCachedMenuSelection");

	if (!anQuest || !abColorR || !abVis || !dwCached)
		return;

	int nQuestIndex = 0;
	char* pnQuestEntry = anQuest;

	do {
		if (*(int*)pnQuestEntry == in_EDX) {
			if (nQuestIndex != -1) {
				uint8_t vis   = (uint8_t)abVis[nQuestIndex];
				uint8_t color = (uint8_t)abColorR[nQuestIndex];
				*(uint32_t*)(dwCached + (uint32_t)vis * 4u) = (uint32_t)color;
			}
			return;
		}
		pnQuestEntry += 0x10;
		nQuestIndex  = nQuestIndex + 1;
	} while ((int)pnQuestEntry < 0x6fba8420);
}
