#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

//Council
struct Act3Quest5						//sizeof 0x50
{
	int nLastKilledMonsterGUID;				//0x00
	uint8_t bOrmusActivated;				//0x04
	uint8_t bMonstersSpawned;				//0x05
	uint8_t pad0x06[2];						//0x06
	int bQuest1RewardGranted;				//0x08
	uint8_t bOrbSmashed;					//0x0C
	uint8_t bFlailDropped;					//0x0D
	uint8_t bCubeDropped;					//0x0E
	uint8_t pad0x0F;						//0x0F
	int nMonsterGUIDs[6];					//0x10
	uint8_t bCompellingOrbSpawned;			//0x28
	uint8_t pad0x29[3];						//0x29
	int nCompellingOrbGUID;					//0x2C
	int nSpawnCount;						//0x30
	int nMonstersLeftToKill;				//0x34
	int nHits;								//0x38
	int nFlailsMissing;						//0x3C
	int nHoradricCubesMissing;				//0x40
	uint8_t unused0x44[12];					//0x44
};

#pragma pack()



//D2Game.0x6FCAAFA0
bool __fastcall ACT3Q5_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//D2Game.0x6FCAB000
void __fastcall ACT3Q5_SpawnCouncil(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCAB0F0
int32_t __fastcall ACT3Q5_UnitIterate_UpdateQuestStateFlags(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCAB160
int32_t __fastcall ACT3Q5_UnitIterate_StatusCyclerEx(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCAB1D0
void __fastcall ACT3Q5_UnitIterate_SetPrimaryGoalDone(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCAB250
void __fastcall ACT3Q5_UnitIterate_DeleteKhalimItems(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCAB380
int32_t __fastcall OBJECTS_OperateFunction53_CompellingOrb(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FCAB580
void __fastcall OBJECTS_InitFunction53_StairsR(ObjInitFn* pOp);
//D2Game.0x6FCAB5B0
void __fastcall OBJECTS_InitFunction60_CompellingOrb(ObjInitFn* pOp);

//D2Game.0x6FCAB620
void __fastcall ACT3Q5_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FCAB6F0
void __fastcall ACT3Q5_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCAB800
void __fastcall ACT3Q5_Callback03_ChangedLevel(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCAB900
void __fastcall ACT3Q5_Callback02_NpcDeactivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCAB960
void __fastcall ACT3Q5_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCABB50
bool __fastcall ACT3Q5_SeqCallback(QuestData* pQuestData);
//D2Game.0x6FCABBC0
void __fastcall ACT3Q5_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCABCB0
void __fastcall ACT3Q5_Callback08_MonsterKilled(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCABED0
int32_t __fastcall ACT3Q5_UnitIterate_UpdateQuestStateAfterMonsterKill(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCAC020
int32_t __fastcall ACT3Q5_UnitIterate_SetPrimaryGoalDoneForPartyMembers(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCAC070
int32_t __fastcall ACT3Q5_UnitIterate_SetCompletionFlag(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCAC0D0
int32_t __fastcall ACT3Q5_UnitIterate_AttachCompletionSound(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCAC110
int32_t __fastcall ACT3Q5_UnitIterate_DetermineFlailAndCubeDropCounts(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCAC1B0
int32_t __fastcall ACT3Q5_IsDuranceOfHateClosed(Game* pGame, UnitAny* pUnit);
//
void __fastcall ACT3Q5_Callback10_PlayerLeavesGame(QuestData* pQuestData, QuestArg* pQuestArg);
