#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

//LamEsen
struct Act3Quest1						//sizeof 0xA0
{
	uint8_t bCanGetReward;					//0x00
	uint8_t bTomeDropped;					//0x01
	uint8_t bTomeActive;					//0x02
	uint8_t unused0x03;						//0x03
	uint8_t bTomeAcquired;					//0x04
	uint8_t bPlayerWithTomeDropped;			//0x05
	uint8_t pad0x06[2];						//0x06
	int nTomesInGame;						//0x08
	uint8_t bTomeBroughtToAlkor;			//0x0C
	uint8_t pad0x0D[3];						//0x0D
	int nPlayerWithTomeGUID;				//0x10
	int nTomeGUID;							//0x14
	int nTomeObjectMode;					//0x18
	QuestGUID tPlayerGUIDs;					//0x1C
};

#pragma pack()



//D2Game.0x6FCA6E80
bool __fastcall ACT3Q1_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//D2Game.0x6FCA6F10
int32_t __fastcall OBJECTS_OperateFunction28_LamEsenTome(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FCA7040
int32_t __fastcall ACT3Q1_UnitIterate_StatusCyclerEx(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCA70B0
void __fastcall ACT3Q1_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA7190
void __fastcall ACT3Q1_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA73B0
int32_t __fastcall ACT3Q1_UnitIterate_UpdateQuestStateFlags(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCA7420
int32_t __fastcall ACT3Q1_UnitIterate_SetPrimaryGoalDone(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCA74A0
int32_t __fastcall ACT3Q1_UnitIterate_AddStatPointReward(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCA74F0
int32_t __fastcall ACT3Q1_UnitIterate_SetCompletionFlag(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCA7530
void __fastcall ACT3Q1_Callback02_NpcDeactivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA7580
void __fastcall ACT3Q1_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FCA7670
void __fastcall ACT3Q1_Callback03_ChangedLevel(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA7740
void __fastcall ACT3Q1_Callback04_ItemPickedUp(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA77D0
void __fastcall ACT3Q1_Callback05_ItemDropped(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA7830
void __fastcall ACT3Q1_Callback09_PlayerDroppedWithQuestItem(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA7870
void __fastcall ACT3Q1_Callback14_PlayerJoinedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA78D0
void __fastcall ACT3Q1_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA79A0
bool __fastcall ACT3Q1_SeqCallback(QuestData* pQuestData);
//D2Game.0x6FCA7A10
bool __fastcall ACT3Q1_StatusFilterCallback(QuestData* pQuest, UnitAny* pPlayer, BitBuffer* pGlobalFlags, BitBuffer* pFlags, uint8_t* pStatus);
//D2Game.0x6FCA7B40
void __fastcall ACT3Q1_UnitIterate_CheckForTomeItem(Game* pGame, UnitAny* pUnit, void* pData);
//
void __fastcall ACT3Q1_Callback10_PlayerLeavesGame(QuestData* pQuestData, QuestArg* pQuestArg);
