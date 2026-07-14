#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

#pragma pack()



//D2Game.0x6FC9F120
void __fastcall ACT2Intro_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FC9F180
void __fastcall ACT2Intro_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC9F260
void __fastcall ACT2Intro_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//
bool __fastcall ACT2Intro_StatusFilterCallback(QuestData* pQuest, UnitAny* pPlayer, BitBuffer* pGlobalFlags, BitBuffer* pFlags, uint8_t* pStatus);
//
bool __fastcall ACT2Intro_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
