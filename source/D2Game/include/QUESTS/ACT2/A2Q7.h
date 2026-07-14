#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

//Unused Gossip
struct Act2Quest7						//sizeof 0x01
{
	uint8_t unk0x00;								//0x00
};

#pragma pack()



//D2Game.0x6FCA6420
void __fastcall ACT2Q7_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FCA64B0
void __fastcall ACT2Q7_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA6510
void __fastcall ACT2Q7_Callback02_NpcDeactivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA6540
void __fastcall ACT2Q7_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA6660
void __fastcall OBJECTS_InitFunction05_Door(ObjInitFn* pOp);
//D2Game.0x6FCA6680
bool __fastcall ACT2Q7_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//
bool __fastcall ACT2Q7_StatusFilterCallback(QuestData* pQuest, UnitAny* pPlayer, BitBuffer* pGlobalFlags, BitBuffer* pFlags, uint8_t* pStatus);
