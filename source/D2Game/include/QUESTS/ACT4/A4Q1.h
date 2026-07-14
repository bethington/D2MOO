#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

//Izual
struct Act4Quest1				//sizeof 0x1C
{
	int unused0x00;					//0x00
	Coord pCoords;			//0x04
	uint8_t unk0x0C;				//0x0C TODO
	uint8_t bIzualGhostIsSpawning;	//0x0D
	uint8_t unused0x0E;				//0x0E
	uint8_t unk0x0F;				//0x0F TODO
	uint8_t bNeedToSpawnIzualGhost;	//0x10
	uint8_t bTyraelActivated;		//0x11
	uint8_t bIzualGhostActivated;	//0x12
	uint8_t pad0x13;				//0x13
	UnitAny* pTargetUnit;		//0x14
	uint8_t bCloseToIzualGhost;		//0x18
	uint8_t pad0x19[3];				//0x19
};

#pragma pack()



//D2Game.0x6FCAD830
bool __fastcall ACT4Q1_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//D2Game.0x6FCAD8B0
void __fastcall ACT4Q1_UnitIterate_SetPrimaryGoalDone(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCAD930
void __fastcall ACT4Q1_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FCADA00
bool __fastcall ACT4Q1_SeqCallback(QuestData* pQuestData);
//D2Game.0x6FCADA70
void __fastcall ACT4Q1_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCADC80
int32_t __fastcall ACT4Q1_UnitIterate_UpdateQuestStateFlags(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCADD00
int32_t __fastcall ACT4Q1_UnitIterate_StatusCyclerEx(Game* pGame, UnitAny* pPlayer, void* pData);
//D2Game.0x6FCADD70
void __fastcall ACT4Q1_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCADEA0
void __fastcall ACT4Q1_Callback08_MonsterKilled(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCADFC0
int32_t __fastcall ACT4Q1_UnitIterate_SetCompletionFlag(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCAE020
int32_t __fastcall ACT4Q1_UnitIterate_SetPrimaryGoalDoneForPartyMembers(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCAE070
bool __fastcall ACT4Q1_SpawnIzualGhost(Game* pGame, QuestData* pQuest);
//D2Game.0x6FCAE140
int32_t __fastcall ACT4Q1_UnitIterate_SetRewardPending(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCAE250
void __fastcall ACT4Q1_Callback03_ChangedLevel(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCAE2E0
void __fastcall ACT4Q1_Callback02_NpcDeactivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCAE330
void __fastcall ACT4Q1_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCAE3C0
void __fastcall ACT4Q1_OnIzualActivated(Game* pGame);
//D2Game.0x6FCAE400
int32_t __fastcall ACT4Q1_IsAnyPlayerInRangeOfIzualGhost(Game* pGame, UnitAny* pTargetUnit);
//D2Game.0x6FCAE450
void __fastcall ACT4Q1_UnitIterate_CheckDistanceToIzualGhost(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCAE470
void __fastcall ACT4Q1_AttachCompletionSoundToPlayers(Game* pGame);
//D2Game.0x6FCAE480
int32_t __fastcall ACT4Q1_UnitIterate_AttachCompletionSound(Game* pGame, UnitAny* pUnit, void* pData);
//
void __fastcall ACT4Q1_Callback10_PlayerLeavesGame(QuestData* pQuestData, QuestArg* pQuestArg);
