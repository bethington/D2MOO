#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

//Greiz Gossip
struct Act2Quest8						//sizeof 0x02
{
	uint8_t unk0x00[2];						//0x00
};

#pragma pack()



//D2Game.0x6FCA66F0
void __fastcall ACT2Q8_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FCA6770
void __fastcall ACT2Q8_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA67B0
void __fastcall ACT2Q8_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA6850
bool __fastcall ACT2Q8_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//
bool __fastcall ACT2Q8_StatusFilterCallback(QuestData* pQuest, UnitAny* pPlayer, BitBuffer* pGlobalFlags, BitBuffer* pFlags, uint8_t* pStatus);
