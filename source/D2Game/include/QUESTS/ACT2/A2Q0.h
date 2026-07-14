#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

//Jerhyn Gossip
struct Act2Quest0						//sizeof 0x84
{
	QuestGUID tPlayerGUIDs;						//0x00
};

#pragma pack()



//D2Game.0x6FC9F350
void __fastcall ACT2Q0_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FC9F3F0
void __fastcall ACT2Q0_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC9F460
void __fastcall ACT2Q0_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC9F510
bool __fastcall ACT2Q0_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//D2Game.0x6FC9F540
void __fastcall ACT2Q0_Callback10_PlayerLeavesGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC9F560
void __fastcall ACT2Q0_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//
bool __fastcall ACT2Q0_StatusFilterCallback(QuestData* pQuest, UnitAny* pPlayer, BitBuffer* pGlobalFlags, BitBuffer* pFlags, uint8_t* pStatus);
