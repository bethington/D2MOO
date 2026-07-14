#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

#pragma pack()



//D2Game.0x6FC977D0
void __fastcall ACT1Q0_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FC97830
void __fastcall ACT1Q0_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC97870
void __fastcall ACT1Q0_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC978F0
bool __fastcall ACT1Q0_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//
bool __fastcall ACT1Q0_StatusFilterCallback(QuestData* pQuest, UnitAny* pPlayer, BitBuffer* pGlobalFlags, BitBuffer* pFlags, uint8_t* pStatus);
