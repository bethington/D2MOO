#pragma once

#include <Units/Units.h>
#include "Objects.h"
#include "ObjMode.h"

//D2Game.0x6FC7A3A0
int32_t __fastcall OBJRGN_AllocObjectControl(Game* pGame);
//D2Game.0x6FC7A5C0
void __fastcall OBJRGN_FreeObjectControl(Game* pGame);
//D2Game.0x6FC7A6B0
ObjectRegion* __fastcall OBJRGN_GetObjectRegionForLevel(Game* pGame, int32_t nLevelId);
//D2Game.0x6FC7A6C0
ObjectControl* __fastcall OBJRGN_GetObjectControlFromGame(Game* pGame);
//D2Game.0x6FC7A6D0
ShrineData* __fastcall OBJRGN_GetShrineDataFromGame(Game* pGame);
//D2Game.0x6FC7A6E0
void __fastcall OBJRGN_AllocObjectRoomCoords(Game* pGame, UnitAny* pUnit, Room1* pRoom);
//D2Game.0x6FC7A780
void __fastcall OBJECTS_InitFunction17_Waypoint(ObjInitFn* pOp);
//D2Game.0x6FC7A860
bool __fastcall OBJRGN_CanNotSpawnMoreWells(Game* pGame, int32_t nLevelId);
//D2Game.0x6FC7A890
bool __fastcall OBJRGN_ShouldSpawnHealingShrineOrWell(Game* pGame, int32_t nLevelId);
//D2Game.0x6FC7A8C0
bool __fastcall OBJRGN_CanNotSpawnMoreShrines(Game* pGame, int32_t nLevelId);
//D2Game.0x6FC7A8F0
void __fastcall OBJRGN_IncreaseHealingShrineCount(Game* pGame, int32_t nLevelId);
//D2Game.0x6FC7A900
bool __fastcall OBJRGN_CanSpawnWell(Game* pGame, int32_t nLevelId, int32_t nX, int32_t nY);
//D2Game.0x6FC7A960
void __fastcall OBJRGN_SetWellCoordinates(Game* pGame, int32_t nLevelId, int32_t nX, int32_t nY);
//D2Game.0x6FC7A990
bool __fastcall OBJRGN_CanSpawnShrine(Game* pGame, int32_t nLevelId, int32_t nX, int32_t nY);
//D2Game.0x6FC7A9F0
void __fastcall OBJRGN_SetShrineCoordinates(Game* pGame, int32_t nLevelId, int32_t nX, int32_t nY);
//D2Game.0x6FC7AA20
int32_t __fastcall OBJRGN_GetTrapMonsterId(ObjOperateFn* pOp);
