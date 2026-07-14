#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)


struct PrisonOfIceReward						//sizeof 0x08
{
	const uint32_t* dwItemCodes;					//0x00
	int nItemCount;									//0x04
};

//PrisonOfIce/Anya
struct Act5Quest3								//sizeof 0x114
{
	QuestGUID tPlayerGUIDs;							//0x00
	int unk0x84;									//0x84 TODO
	int unk0x88;									//0x88 TODO
	int nTimerInvocations;							//0x8C
	uint8_t bDrehyaIcedMonsterSpawned;				//0x90
	uint8_t bDrehyaMonsterInTownSpawned;			//0x91
	uint8_t bNilathakOutsideTownSpawned;			//0x92
	uint8_t bNihlathakMonsterInTownSpawned;			//0x93
	int nDrehyaMonsterInTownGUID;					//0x94
	int nDrehyaIcedMonsterGUID;						//0x98
	int NihlathakMonsterInTownGUID;					//0x9C
	int nNihlathakOutsideTownGUID;					//0xA0
	uint8_t bFrozenAnyaObjectSpawned;				//0xA4
	uint8_t pad0xA5[3];								//0xA5
	int nFrozenDrehyaObjectGUID;					//0xA8
	uint8_t bMalahIntroActivated;					//0xAC
	uint8_t bChangeToSpecialObjectModeOutsideTown;	//0xAD
	uint8_t bChangeToSpecialObjectModeInTown;		//0xAE
	uint8_t pad0xAF;								//0xAF
	uint8_t unused0xB0;								//0xB0
	uint8_t bDrehyaPortalOutsideTownSpawned;		//0xB1
	uint8_t bDrehyaPortalInTownSpawned;				//0xB2
	uint8_t unused0xB3;								//0xB3
	int nDrehyaPortalInTownGUID;					//0xB4
	uint8_t unused0xB8[8];							//0xB8
	int nDrehyaPortalOutsideTownGUID;				//0xC0
	Coord pDrehyaPortalCoords;				//0xC4
	uint8_t unused0xCC[12];							//0xCC
	int nDefrostPotionsInGame;						//0xD8
	int nObjectUpdateInvocations;					//0xDC
	uint8_t bDrehyaPortalCoordsInitialized;			//0xE0
	uint8_t bDrehyaInTownInitialized;				//0xE1
	uint8_t bDefrostPotionAcquired;					//0xE2
	uint8_t bDrehyaIcedRemoved;						//0xE3
	int nDrehyaObjectInTownGUID;					//0xE4
	Coord pDrehyaObjectInTownCoords;			//0xE8
	Coord pDrehyaObjectOutsideTownCoords;		//0xF0
	int nDrehyaObjectOutsideTownGUID;				//0xF8
	int nFrozenAnyaObjectGUID;						//0xFC
	uint8_t bTimerActive;							//0x100
	uint8_t bMalahActivated;						//0x101
	uint8_t bRewarded;								//0x102
	uint8_t pad0x103;								//0x103
	MapAI* pDrehyaMapAI;						//0x104
	MapAI* pNilathakMapAI;					//0x108
	uint8_t bDrehyaMapAIChanged;					//0x10C
	uint8_t bNilathakMapAIChanged;					//0x10D
	uint8_t pad0x10E[2];							//0x10E
	int nNihlathakObjectInTownGUID;					//0x110
};

#pragma pack()

//D2Game.0x6FCB3530
void __fastcall ACT5Q3_UnitIterate_SetPrimaryGoalDone(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB35A0
void __fastcall ACT5Q3_RemoveNihlathakFromTown(Game* pGame);
//D2Game.0x6FCB3670
bool __fastcall ACT5Q3_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//D2Game.0x6FCB3760
void __fastcall ACT5Q3_UpdateResistances(Game* pGame, UnitAny* pUnit, int32_t nValue);
//D2Game.0x6FCB3810
void __fastcall ACT5Q3_ApplyResistanceReward(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCB3880
void __fastcall ACT5Q3_UnitIterate_CheckForDefrostPotion(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB38B0
void __fastcall ACT5Q3_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FCB39A0
void __fastcall ACT5Q3_Callback02_NpcDeactivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB3A20
int32_t __fastcall ACT5Q3_UnitIterate_StatusCyclerEx(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB3A90
bool __fastcall ACT5Q3_SeqCallback(QuestData* pQuestData);
//D2Game.0x6FCB3B00
void __fastcall ACT5Q3_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB4040
int32_t __fastcall ACT5Q3_UnitIterate_UpdateQuestStateFlags(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB40B0
int32_t __fastcall ACT5Q3_RemoveDrehyaIced(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB4150
void __fastcall ACT5Q3_SendPacket0x5DToClient(UnitAny* pUnit);
//D2Game.0x6FCB4190
void __fastcall ACT5Q3_SpawnDrehyaInTown(QuestData* pQuestData, Act5Quest3* pQuestDataEx);
//D2Game.0x6FCB4400
//TODO: Find a name
void __fastcall sub_6FCB4400(QuestData* pQuestData);
//D2Game.0x6FCB45E0
void __fastcall ACT5Q3_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB4810
void __fastcall ACT5Q3_Callback10_PlayerLeavesGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB4870
void __fastcall ACT5Q3_Callback03_ChangedLevel(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB4A10
int32_t __fastcall ACT5Q3_UnitIterate_SetCompletionFlag(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB4A80
void __fastcall ACT5Q3_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB4B50
void __fastcall ACT5Q3_Callback14_PlayerJoinedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB4BF0
bool __fastcall ACT5Q3_StatusFilterCallback(QuestData* pQuest, UnitAny* pPlayer, BitBuffer* pGlobalFlags, BitBuffer* pFlags, uint8_t* pStatus);
//D2Game.0x6FCB4D30
void __fastcall OBJECTS_InitFunction66_DrehyaStartInTown(ObjInitFn* pOp);
//D2Game.0x6FCB4EA0
void __fastcall ACT5Q3_SpawnFrozenDrehya(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCB4FB0
void __fastcall OBJECTS_InitFunction67_DrehyaStartOutsideTown(ObjInitFn* pOp);
//D2Game.0x6FCB5010
void __fastcall OBJECTS_InitFunction68_NihlathakStartInTown(ObjInitFn* pOp);
//D2Game.0x6FCB50C0
void __fastcall OBJECTS_InitFunction69_NihlathakStartOutsideTown(ObjInitFn* pOp);
//D2Game.0x6FCB5130
void __fastcall ACT5Q3_UpdateDrehyaPortalMode(QuestData* pQuestData, UnitAny* pUnit);
//D2Game.0x6FCB51C0
int32_t __fastcall ACT5Q3_SpawnDrehyaPortalOutsideTown(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCB5320
int32_t __fastcall ACT5Q3_GetDrehyaPortalCoordinates(Game* pGame, UnitAny* pUnit, Coord* pCoord);
//D2Game.0x6FCB5390
int32_t __fastcall ACT5Q3_InitializeDrehyaPortalCoordinates(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCB53D0
//TODO: Find a name
void __fastcall sub_6FCB53D0(Game* pGame);
//D2Game.0x6FCB5400
//TODO: Find a name
int32_t __fastcall sub_6FCB5400(Game* pGame);
//D2Game.0x6FCB5430
//TODO: Find a name
void __fastcall sub_6FCB5430(Game* pGame);
//D2Game.0x6FCB5470
void __fastcall OBJECTS_InitFunction74_FrozenAnya(ObjInitFn* pOp);
//D2Game.0x6FCB54D0
int32_t __fastcall OBJECTS_OperateFunction67_FrozenAnya(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FCB56D0
int32_t __fastcall ACT5Q3_UnitIterate_SetPrimaryGoalDoneForPartyMembers(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB5720
bool __fastcall ACT5Q3_SpawnDrehyaIcedMonsterOutsideTown(Game* pGame, QuestData* pQuestData);
//D2Game.0x6FCB5890
void __fastcall ACT5Q3_ChangeDrehyaMapAI(Game* pGame, UnitAny* pUnit, MapAI* pMapAi);
//D2Game.0x6FCB5920
void __fastcall ACT5Q3_ChangeNihlathakMapAI(Game* pGame, UnitAny* pUnit, MapAI* pMapAi);
