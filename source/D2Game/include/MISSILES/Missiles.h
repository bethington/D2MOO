#pragma once

#include <Units/Units.h>


//D2Game.0x6FC552A0
void __fastcall MISSILES_RemoveAll(Game* pGame);
//D2Game.0x6FC552F0
void __fastcall MISSILES_Initialize(Game* pGame, UnitAny* pMissile, int32_t nUnitGUID);
//D2Game.0x6FC55340
void __fastcall MISSILES_Free(Game* pGame, UnitAny* pMissile);
//D2Game.0x6FC55360
UnitAny* __fastcall MISSILES_CreateMissileFromParams(Game* pGame, Missile* missileParams);
//D2Game.0x6FC55B70
void __fastcall MISSILES_SyncToClient(GameClient* pClient, Game* pGame, UnitAny* pMissile, int32_t nVelocity);
