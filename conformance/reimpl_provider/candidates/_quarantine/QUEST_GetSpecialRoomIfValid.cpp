#include "../provider_runtime.h"

// QUEST_FindSpecialRoom is a free function defined in D2Game.dll; declare it as extern.
extern "C" uint32_t QUEST_FindSpecialRoom(void);

// D2MOO_REIMPL_EXPORT: QUEST_GetSpecialRoomIfValid
extern "C" void* QUEST_GetSpecialRoomIfValid(void* pThis, int nQuestType)
{
	// Decompiled algorithm:
	//   if (in_EAX == 0x49) return QUEST_FindSpecialRoom();
	//   return 0;
	// in_EAX is the quest type, passed in EAX on entry (per plate comment).
	if (nQuestType == 0x49) {
		return (void*)QUEST_FindSpecialRoom();
	}
	return (void*)0;
}
