#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

//Countess
struct Act1Quest5						//sizeof 0x120
{
	int nUnitGUIDs1[12];					//0x00 TODO: Which UnitGUIDs?
	short nUnitCount1;						//0x30
	uint8_t pad0x32[2];						//0x32
	int nUnitGUIDs2[12];					//0x34 TODO: Which UnitGUIDs?
	short nUnitCount2;						//0x64
	uint8_t pad0x66[2];						//0x66
	int nObjectGUIDs[8];					//0x68 TODO: Which objects?
	uint16_t nObjectCount;					//0x88
	uint8_t pad0x8A[2];						//0x8A
	QuestGUID tPlayerGUIDs;					//0x8C
	Coord pCoord;						//0x110
	uint8_t bCountessKilled;				//0x118
	uint8_t bDeathMissilesCreated;			//0x119
	uint8_t bTriggerSeqFilter;				//0x11A
	uint8_t bTowerTomeActivated;			//0x11B
	uint8_t unk0x11C;						//0x11C TODO
	uint8_t pad0x11D[3];					//0x11D
};

#pragma pack()



//D2Game.0x6FC9CD00
int32_t __fastcall OBJECTS_OperateFunction06_TowerTome(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC9CDA0
int32_t __fastcall ACT1Q5_UnitIterate_StatusCyclerEx(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9CE00
int32_t __fastcall ACT1Q5_UnitIterate_UpdateQuestStateFlags(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9CED0
bool __fastcall ACT1Q5_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//D2Game.0x6FC9CF20
void __fastcall ACT1Q5_UnitIterate_SetPrimaryGoalDone(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9CFD0
void __fastcall ACT1Q5_SpawnTowerChestMissiles(QuestData* pQuestData, Act1Quest5* pQuestDataEx);
//D2Game.0x6FC9D2E0
void __fastcall ACT1Q5_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FC9D3B0
void __fastcall ACT1Q5_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC9D5D0
int32_t __fastcall ACT1Q5_UpdateUnitGUIDLists(Act1Quest5* pQuestDataEx, int32_t nUnitGUID, int32_t a3);
//D2Game.0x6FC9D650
void __fastcall ACT1Q5_Callback10_PlayerLeavesGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC9D6A0
void __fastcall ACT1Q5_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC9D830
void __fastcall ACT1Q5_Callback03_ChangedLevel(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC9D970
bool __fastcall ACT1Q5_SeqCallback(QuestData* pQuestData);
//D2Game.0x6FC9D9F0
void __fastcall ACT1Q5_Callback08_MonsterKilled(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC9DBE0
int32_t __fastcall ACT1Q5_UnitIterate_SetRewardGranted(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9DC80
int32_t __fastcall ACT1Q5_UnitIterate_AttachCompletionSound(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9DD40
int32_t __fastcall ACT1Q5_UnitIterate_SetCompletionFlag(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9DD90
int32_t __fastcall ACT1Q5_UnitIterate_SetPrimaryGoalDoneForPartyMembers(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9DDE0
bool __fastcall ACT1Q5_Timer_StatusCycler(Game* pGame, QuestData* pQuestData);
//D2Game.0x6FC9DE10
void __fastcall ACT1Q5_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC9DEE0
void __fastcall OBJECTS_InitFunction04_TowerTome(ObjInitFn* pOp);
//D2Game.0x6FC9DF30
void __fastcall OBJECTS_InitFunction47_CountessChest(ObjInitFn* pOp);
