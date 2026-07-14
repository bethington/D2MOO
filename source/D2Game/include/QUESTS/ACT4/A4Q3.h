#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

//Hellforge
struct Act4Quest3						//sizeof 0x20
{
	short nHellforgeObjectMode;				//0x00
	uint8_t bCainActivated;					//0x02
	uint8_t bRewardDropsPending;			//0x03
	uint8_t bSoulstoneSmashed;				//0x04
	uint8_t pad0x05[3];						//0x05
	int nGemDropTier;						//0x08
	int nHits;								//0x0C
	int nHellforgeHammersInGame;			//0x10
	int nPlayersInAct;						//0x14
	int unused0x18;							//0x18
	uint8_t bSoulstoneAcquired;				//0x1C
	uint8_t pad0x1D[3];						//0x1D
};

#pragma pack()



//D2Game.0x6FCAFF50
void __fastcall OBJECTS_InitFunction48_HellForge(ObjInitFn* pOp);
//D2Game.0x6FCAFFC0
int32_t __fastcall ACT4Q3_UnitIterate_StatusCyclerEx(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB0030
void __fastcall ACT4Q3_UnitIterate_SetPrimaryGoalDone(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB00E0
int32_t __fastcall OBJECTS_OperateFunction49_HellForge(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FCB03A0
int32_t __fastcall ACT4Q3_UnitIterate_SetCompletionFlag(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB0400
bool __fastcall ACT4Q3_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//D2Game.0x6FCB0460
void __fastcall ACT4Q3_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FCB0550
void __fastcall ACT4Q3_Callback04_ItemPickedUp(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB05C0
bool __fastcall ACT4Q3_SeqCallback(QuestData* pQuestData);
//D2Game.0x6FCB0650
void __fastcall ACT4Q3_Callback03_ChangedLevel(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB06D0
int32_t __fastcall ACT4Q3_UnitIterate_UpdateQuestStateFlags(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB0760
void __fastcall ACT4Q3_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB0920
void __fastcall ACT4Q3_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB0A80
void __fastcall ACT4Q3_Callback02_NpcDeactivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB0AE0
void __fastcall ACT4Q3_Callback09_PlayerDroppedWithQuestItem(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB0B00
void __fastcall ACT4Q3_Callback14_PlayerJoinedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB0B20
void __fastcall ACT4Q3_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB0C00
void __fastcall ACT4Q3_Callback08_MonsterKilled(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB0C40
void __fastcall ACT4Q3_CreateReward(QuestData* pQuestData, UnitAny* pUnit);
//
void __fastcall ACT4Q3_Callback10_PlayerLeavesGame(QuestData* pQuestData, QuestArg* pQuestArg);
