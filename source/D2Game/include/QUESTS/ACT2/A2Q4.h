#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

//HorazonTome
struct Act2Quest4							//sizeof 0x48
{
	uint8_t unused0x00[4];						//0x00
	uint8_t bDrognanActivated;					//0x04
	uint8_t pad0x05[3];							//0x05
	Room1* pRoom;							//0x08
	uint8_t bJerhynStartInitialized;			//0x0C
	uint8_t bJerhynPalaceInitialized;			//0x0D
	uint8_t bPalaceOpen;						//0x0E
	uint8_t unk0x0F;							//0x0F TODO
	uint8_t unk0x10;							//0x10 TODO
	uint8_t bHaremBlockerInitialized;			//0x11
	uint8_t unk0x12;							//0x12 TODO
	uint8_t unk0x13;							//0x13 TODO
	uint8_t unk0x14;							//0x14 TODO
	uint8_t bJerhynCoordinatesStored;			//0x15
	uint8_t bPortalToCanyonOpen;				//0x16
	uint8_t unused0x17;							//0x17
	uint8_t unk0x18;							//0x18 TODO
	uint8_t unk0x19;							//0x19 TODO
	uint8_t bPlayerCloseToHaremBlocker;			//0x1A
	uint8_t pad0x1B;							//0x1B
	int nPlayerCloseToHaremBlockerGUID;			//0x1C
	Coord pHaremBlockerCoords;			//0x20
	Coord pJerhynCoords;					//0x28
	Coord pGuardCoords;					//0x30
	int nHaremBlockerGUID;						//0x38
	int nJerhynGUID;							//0x3C
	uint16_t nHaremBlockerObjectMode;			//0x40
	uint16_t nPortalModeInSanctuary;			//0x42
	uint16_t nPortalModeInCellar;				//0x44
	uint8_t bIsHaremBlockerNeutral;				//0x46
	uint8_t pad0x47;							//0x47
};

#pragma pack()



//D2Game.0x6FCA25C0
bool __fastcall ACT2Q4_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//D2Game.0x6FCA2660
int32_t __fastcall ACT2Q4_UnitIterate_StatusCyclerEx(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCA26D0
int32_t __fastcall ACT2Q4_UnitIterate_UpdateQuestStateFlags(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCA2780
int32_t __fastcall ACT2Q4_IsHaremBlockerObjectOpened(Game* pGame);
//D2Game.0x6FCA27B0
void __fastcall ACT2Q4_UnitIterate_SetPrimaryGoalDone(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCA2840
void __fastcall ACT2Q4_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FCA2920
void __fastcall ACT2Q4_Callback02_NpcDeactivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA2980
void __fastcall ACT2Q4_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA2C50
void __fastcall ACT2Q4_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA2D90
void __fastcall ACT2Q4_Callback03_ChangedLevel(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA2F60
void __fastcall ACT2Q4_InitializeJerhynMonster(QuestData* pQuestData, UnitAny* pUnit, Room1* pRoom, Coord* pCoord);
//D2Game.0x6FCA3160
bool __fastcall ACT2Q4_SeqCallback(QuestData* pQuestData);
//D2Game.0x6FCA31E0
void __fastcall ACT2Q4_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA3320
void __fastcall ACT2Q4_InitializeJerhynStartObject(QuestData* pQuestData, ObjInitFn* pOp);
//D2Game.0x6FCA33F0
void __fastcall ACT2Q4_InitializeJerhynPalaceObject(QuestData* pQuestData, ObjInitFn* pOp);
//D2Game.0x6FCA34D0
int32_t __fastcall ACT2Q4_IsGuardMoving(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCA3500
void __fastcall ACT2Q4_InitializeHaremBlockerObject(QuestData* pQuestData, UnitAny* pUnit);
//D2Game.0x6FCA35D0
void __fastcall OBJECTS_InitFunction30_HaremBlocker(ObjInitFn* pOp);
//D2Game.0x6FCA3620
int32_t __fastcall ACT2Q4_IsJerhynPalaceActivated(Game* pGame);
//D2Game.0x6FCA36A0
int32_t __fastcall ACT2Q4_HasGuardMovedToEndPosition(Game* pGame);
//D2Game.0x6FCA36E0
int32_t __fastcall ACT2Q4_GetGuardCoordinates(Game* pGame, Coord* pCoord);
//D2Game.0x6FCA3740
void __fastcall ACT2Q4_GetAndUpdatePalaceNpcState(Game* pGame, UnitAny* pUnit, int32_t* a3, int32_t* pIdle);
//D2Game.0x6FCA3A10
int32_t __fastcall ACT2Q4_UnitIterate_CheckDistanceToHaremBlocker(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCA3AA0
int32_t __fastcall OBJECTS_OperateFunction42_SanctuaryTome(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FCA3B80
int32_t __fastcall ACT2Q4_UnitIterate_SetCompletionFlag(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCA3BC0
int32_t __fastcall ACT2Q4_UnitIterate_SetPrimaryGoalDoneForPartyMembers(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCA3C70
int32_t __fastcall ACT2Q4_UnitIterate_UselessGoalCheck(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCA3CA0
void __fastcall OBJECTS_InitFunction29_ArcaneSanctuaryPortal(ObjInitFn* pOp);
//D2Game.0x6FCA3D60
void __fastcall ACT2Q4_SetPortalMode(Game* pGame, int32_t nLevelId);
//
void __fastcall ACT2Q4_Callback10_PlayerLeavesGame(QuestData* pQuestData, QuestArg* pQuestArg);
