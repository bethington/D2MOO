#pragma once

#include <Units/Units.h>
#include <AI/AiStates.h>

#pragma pack(1)

struct MinionList
{
	D2UnitGUID dwMinionGUID;						//0x00
	MinionList* pNext;				//0x04
};

struct InactiveItemNode
{
	InactiveItemNode* pNext;			//0x00
	int32_t nFrame;								//0x04
	int32_t nOwnerId;							//0x08
	uint16_t nBitstreamSize;					//0x0C
	uint8_t pBitstream;						//0x0E - dynamic size dependent on item serialization
};

struct InactiveMonsterNode
{
	int32_t nX;									//0x00
	int32_t nY;									//0x04
	int32_t nClassId;							//0x08
	int32_t nUnitId;							//0x0C
	int32_t nUnitFlags;							//0x10
	int32_t nUnitFlagsEx;						//0x14
	int32_t nTypeFlags;							//0x18
	int32_t unk0x1C;							//0x1C
	MinionList* pMinionList;				//0x20
	MapAI* pMapAI;						//0x24
	AiSpecialState nAiSpecialState;			//0x28
	int32_t nLevelId;							//0x2C
	uint16_t nNameSeed;							//0x30
	uint8_t nMonUMods[9];						//0x32
	uint8_t unk0x3B;							//0x3B
	uint16_t nBossHcIdx;						//0x3C
	uint16_t unk0x3E;							//0x3E
	int32_t nExperience;						//0x40
	int32_t nMaxHitpoints;						//0x44
	int32_t nHitpoints;							//0x48
	int32_t nCmdParam1;							//0x4C
	int32_t nCmdParam2;							//0x50
	int32_t nGameFrame;							//0x54
	InactiveMonsterNode* pNext;			//0x58
};

struct InactiveUnitNode
{
	int32_t nX;									//0x00
	int32_t nY;									//0x04
	int32_t nUnitType;							//0x08
	int32_t nClassId;							//0x0C
	int32_t nAnimMode;							//0x10
	int32_t nGameFrame;							//0x14
	int32_t nUnitFlags;							//0x18
	int32_t nUnitFlagsEx;						//0x1C
	int32_t nUnitId;							//0x20
	int32_t nFrame;								//0x24
	uint8_t nInteractType;						//0x28
	uint8_t unk0x29;							//0x29
	uint16_t unk0x2A;							//0x2A
	int32_t nDropItemCode;						//0x2C
	InactiveUnitNode* pNext;			//0x30
};

struct InactiveUnitList
{
	int32_t nX;											//0x00
	int32_t nY;											//0x04
	InactiveItemNode* pInactiveItems;			//0x08
	InactiveMonsterNode* pInactiveMonsters;	//0x0C
	InactiveUnitNode* pInactiveUnits;			//0x10
	InactiveUnitList* pNext;					//0x14
};

#pragma pack()

//D2Game.0x6FCC3850
void __fastcall SUNITINACTIVE_RestoreInactiveUnits(Game* pGame, Room1* pRoom);
//D2Game.0x6FCC40D0
AiControl* __fastcall AIGENERAL_GetAiControlFromUnit(UnitAny* pUnit);
//D2Game.0x6FCC40F0
void __fastcall SUNITINACTIVE_FreeInactiveMonsterNode(Game* pGame, InactiveMonsterNode* pInactiveMonsterNode);
//D2Game.0x6FCC4120
UnitAny* __fastcall SUNITINACTIVE_RestoreInactiveItem(Game* pGame, Room1* pRoom, InactiveItemNode* pInactiveItemNode);
//D2Game.0x6FCC4270
void __fastcall SUNITINACTIVE_FreeInactiveUnitLists(Game* pGame);
//D2Game.0x6FCC4370
void __fastcall SUNITINACTIVE_CompressUnitIfNeeded(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCC4650
void __fastcall SUNITINACTIVE_CompressInactiveUnit(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCC4C90
void __fastcall SUNITINACTIVE_DeleteSingleListNode(Game* pGame, uint16_t nUnitType, uint16_t nClassId, uint8_t nAct);
//D2Game.0x6FCC4DC0
InactiveUnitList* __fastcall SUNITINACTIVE_GetListNodeFromActAndCoordinates(Game* pGame, int32_t nAct, int32_t nX, int32_t nY, int32_t bAllocNewNode);
//D2Game.0x6FCC4E80
InactiveUnitList* __fastcall SUNITINACTIVE_GetListNodeFromRoom(Game* pGame, Room1* pRoom, int32_t bAllocNewNode);
//D2Game.0x6FCC4ED0
void __fastcall SUNITINACTIVE_RestoreSpecialMonsterParameters(Game* pGame, UnitAny* pUnit, InactiveMonsterNode* pInactiveMonsterNode);
//D2Game.0x6FCC50B0
void __fastcall SUNITINACTIVE_SaveSpecialMonsterParameters(Game* pGame, UnitAny* pUnit, InactiveMonsterNode* pInactiveMonsterNode);
//D2Game.0x6FCC52C0
int32_t __fastcall SUNITINACTIVE_IsUnitInsideRoom(Game* pGame, Room1* pRoomNear, int32_t nGameX, int32_t nGameY, int32_t nClassId);
//D2Game.0x6FCC5490
void __fastcall SUNITINACTIVE_DeleteExpiredItemNodes(Game* pGame, int32_t nAct);
//D2Game.0x6FCC54F0
void __fastcall SUNITINACTIVE_SetUnitFlagEx(UnitAny* pUnit, uint32_t nFlag, int32_t bSet);
