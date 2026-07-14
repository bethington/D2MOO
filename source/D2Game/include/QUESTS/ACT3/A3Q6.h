#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

//Mephisto/BlackTemple
struct Act3Quest6						//sizeof 0x34
{
	uint8_t bTimerStarted;					//0x00
	uint8_t bHellGatePortalInitialized;		//0x01
	uint8_t bMephistoBridgeInitialized;		//0x02
	uint8_t bOrmusActivated;				//0x03
	int nHellGatePortalGUID;				//0x04
	int nMephistoBridgeGUID;				//0x08
	int nHellGatePortalObjectMode;			//0x0C
	int nMephistoBridgeObjectMode;			//0x10
	int nSoulStonesDropped;					//0x14
	uint8_t bSoulStoneDropped;				//0x18
	uint8_t pad0x19[3];						//0x19
	int nSoulstonesToDrop;					//0x1C
	uint8_t bNatalyaSpawned;				//0x20
	uint8_t pad0x21[3];						//0x21
	MapAI* pNatalyaMapAI;				//0x24
	uint8_t unused0x28[4];					//0x28
	int nNatalyaGUID;						//0x2C
	uint8_t bNatalyaMapAIChanged;			//0x30
	uint8_t pad0x31[3];						//0x31
};

#pragma pack()



//D2Game.0x6FCAC1F0
bool __fastcall ACT3Q6_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//D2Game.0x6FCAC230
void __fastcall ACT3Q6_UnitIterate_SetPrimaryGoalDone(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCAC380
void __fastcall OBJECTS_InitFunction45_MephistoBridge(ObjInitFn* pOp);
//D2Game.0x6FCAC3E0
void __fastcall OBJECTS_InitFunction44_HellGatePortal(ObjInitFn* pOp);
//D2Game.0x6FCAC450
void __fastcall ACT3Q6_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FCAC510
void __fastcall ACT3Q6_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCAC610
void __fastcall ACT3Q6_Callback03_ChangedLevel(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCAC7F0
int32_t __fastcall ACT3Q6_UnitIterate_UpdateQuestStateFlags(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCAC910
int32_t __fastcall ACT3Q6_UnitIterate_StatusCyclerEx(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCAC980
void __fastcall ACT3Q6_Callback02_NpcDeactivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCAC9D0
void __fastcall ACT3Q6_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCACB30
void __fastcall ACT3Q6_Callback08_MonsterKilled(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCACD80
int32_t __fastcall ACT3Q6_UnitIterate_UpdateQuestStateAfterMonsterKill(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCACE30
bool __fastcall ACT3Q6_Timer_StatusCycler(Game* pGame, QuestData* pQuestData);
//D2Game.0x6FCACE60
int32_t __fastcall ACT3Q6_UnitIterate_SetPrimaryGoalDoneForPartyMembers(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCACEB0
int32_t __fastcall ACT3Q6_UnitIterate_SetCompletionFlag(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCACF10
int32_t __fastcall ACT3Q6_UnitIterate_AttachCompletionSound(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCACF50
bool __fastcall ACT3Q6_SeqCallback(QuestData* pQuestData);
//D2Game.0x6FCACF80
void __fastcall ACT3Q6_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCAD0C0
void __fastcall OBJECTS_InitFunction52_NatalyaStart(ObjInitFn* pOp);
//D2Game.0x6FCAD1A0
void __fastcall ACT3Q6_SetObjectModes(Game* pGame);
//
void __fastcall ACT3Q6_Callback10_PlayerLeavesGame(QuestData* pQuestData, QuestArg* pQuestArg);
