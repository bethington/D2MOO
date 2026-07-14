#pragma once

#include <Units/Units.h>
#include <Drlg/D2DrlgDrlg.h>


#pragma pack(push, 1)
struct UnkMonCreate
{
	Game* pGame;						//0x00
	Room1* pRoom;						//0x04
	RoomCoordList* pRoomCoordList;	//0x08
	int32_t nMonsterId;						//0x0C
	int32_t nAnimMode;						//0x10
	int32_t nUnitGUID;						//0x14
	int32_t nX;								//0x18
	int32_t nY;								//0x1C
	int32_t field_20;						//0x20
	int16_t nFlags;							//0x24
};

struct UnkMonCreate3
{
	int32_t nXOffset;						//0x00
	int32_t nYOffset;						//0x04
	int32_t nSpawnType;						//0x08
};

struct UnkMonCreate2
{
	int32_t nRecordCount;					//0x00
	int32_t unk0x04;						//0x04
	uint8_t unk0x08;						//0x08
	uint8_t pad0x09[3];						//0x09
	UnkMonCreate3 records[4];			//0x0C - Could be variable length, but at both locations where it's used, it's 4
};
#pragma pack(pop)


//D2Game.0x6FC68350
int32_t __fastcall sub_6FC68350(int32_t nMonsterId, Room1* pRoom, int32_t nX, int32_t nY, int32_t a5);
//D2Game.0x6FC68630
int32_t __fastcall sub_6FC68630(Game* pGame, UnitAny* pUnit, int32_t nSkillId, UnitAny* pTarget, int32_t nX, int32_t nY);
//D2Game.0x6FC68CC0
int32_t __fastcall MONSTERSPAWN_GetResurrectMode(UnitAny* pUnit, int32_t a2);
//D2Game.0x6FC68D70
UnitAny* __fastcall sub_6FC68D70(Game* pGame, UnitAny* pUnit, int32_t nMonsterId, int32_t nAnimMode, int32_t a5, int16_t nFlags);
//D2Game.0x6FC68E30
UnitAny* __fastcall D2GAME_SpawnNormalMonster_6FC68E30(UnkMonCreate* pMonCreate);



//D2Game.0x6FC69B60
void __fastcall MONSTERSPAWN_EquipAncientBarbarians(UnitAny* pUnit);
//D2Game.0x6FC69C00
void __fastcall sub_6FC69C00(int32_t a1, UnitAny* a2);
//D2Game.0x6FC69F10
UnitAny* __fastcall D2GAME_SpawnMonster_6FC69F10(Game* pGame, Room1* pRoom, int32_t nX, int32_t nY, int32_t nMonsterId, int32_t nAnimMode, int32_t a7, int16_t nFlags);
//D2Game.0x6FC69F70
UnitAny* __fastcall sub_6FC69F70(Game* pGame, RoomCoordList* pRoomCoordList, UnitAny* pUnit, int32_t nMonsterId, int32_t nAnimMode, int32_t a6, int16_t nFlags);
//D2Game.0x6FC6A030
UnitAny* __fastcall sub_6FC6A030(Game* pGame, Room1* pRoom, RoomCoordList* pRoomCoordList, int32_t nX, int32_t nY, int32_t nMonsterId, int32_t nAnimMode, int32_t a8, int16_t nFlags);
//D2Game.0x6FC6A090
UnitAny* __fastcall sub_6FC6A090(Game* pGame, Room1* pRoom, int32_t nX, int32_t nY, int32_t nMonsterId, int32_t nAnimMode, int16_t nFlags);
//D2Game.0x6FC6A0F0
UnitAny* __fastcall sub_6FC6A0F0(Game* pGame, Room1* pRoom, int32_t nX, int32_t nY, int32_t nMonsterId, int32_t nMode, int32_t nUnitId, int32_t a8, int16_t nFlags);
//D2Game.0x6FC6A150
UnitAny* __fastcall sub_6FC6A150(Game* pGame, UnitAny* pUnit, int32_t nMonsterId, int32_t nAnimMode, int32_t a5, int16_t nFlags);
//D2Game.0x6FC6A230
int32_t __fastcall sub_6FC6A230(Game* pGame, UnitAny* pOwner, int32_t nMonsterId, int32_t nAnimMode, int32_t a5, int32_t nCount, int16_t nFlags);
//D2Game.0x6FC6A350
int32_t __fastcall sub_6FC6A350(Game* pGame, Room1* pRoom, int32_t nX, int32_t nY, UnitAny* pOwner, int32_t nClassId, int32_t nAnimMode, UnkMonCreate2* a8, int16_t nFlags);
//D2Game.0x6FC6A810
int32_t __fastcall sub_6FC6A810(Game* pGame, Room1* pRoom, int32_t a3, int32_t a4, UnitAny* pTargetUnit, int32_t a6, int16_t a7);
//D2Game.0x6FC6A8C0
int32_t __fastcall sub_6FC6A8C0(Game* pGame, UnitAny* pUnit, int32_t nMonsterId, int32_t nAnimMode, int32_t nCount, int32_t a6, int16_t nFlags);
//D2Game.0x6FC6AA70
int32_t __fastcall MONSTERSPAWN_SpawnRandomMonsterForLevel(Game* pGame, Room1* pRoom, int32_t nX, int32_t nY);
