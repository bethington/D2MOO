#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

//Hratli Gossip
struct Act3Quest0						//sizeof 0x1C
{
	uint8_t bHratliStartCreated;			//0x00
	uint8_t bHratliEndCreated;				//0x01
	uint8_t bHratliEndObjectInitialized;	//0x02
	uint8_t bHratliStartIsPresent;			//0x03
	int nHratliX;							//0x04
	int nHratliY;							//0x08
	int nHratliGUID;						//0x0C
	uint8_t bHratliMapAIChanged;			//0x10
	uint8_t pad0x11[7];						//0x11
	MapAI* pHratliMapAI;				//0x18
};

#pragma pack()



//D2Game.0x6FCA6B40
void __fastcall ACT3Q0_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FCA6BD0
void __fastcall ACT3Q0_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA6C30
void __fastcall ACT3Q0_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA6CB0
bool __fastcall ACT3Q0_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//D2Game.0x6FCA6CE0
void __fastcall ACT3Q0_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA6D20
void __fastcall OBJECTS_InitFunction49_HratliStart(ObjInitFn* pOp);
//D2Game.0x6FCA6DA0
void __fastcall OBJECTS_InitFunction50_HratliEnd(ObjInitFn* pOp);
//
bool __fastcall ACT3Q0_StatusFilterCallback(QuestData* pQuest, UnitAny* pPlayer, BitBuffer* pGlobalFlags, BitBuffer* pFlags, uint8_t* pStatus);
