#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

#pragma pack()



//D2Game.0x6FC9EF40
void __fastcall ACT1Q7_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FC9EFA0
void __fastcall ACT1Q7_Callback00_NpcActivate(QuestData* pQuest, QuestArg* pQuestArg);
//D2Game.0x6FC9F0A0
bool __fastcall ACT1Q7_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//
bool __fastcall ACT1Q7_StatusFilterCallback(QuestData* pQuest, UnitAny* pPlayer, BitBuffer* pGlobalFlags, BitBuffer* pFlags, uint8_t* pStatus);
