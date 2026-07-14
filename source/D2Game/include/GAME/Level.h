#pragma once

#include <Units/Units.h>


//D2Game.0x6FC3BBA0
void __fastcall LEVEL_UpdateUnitsInAdjacentRooms(Game* pGame, Room1* pRoom, GameClient* pClient);
//D2Game.0x6FC3BD10
void __fastcall LEVEL_RemoveUnitsExceptClientPlayer(Room1* pRoom, GameClient* pClient);
//D2Game.0x6FC3BDE0
void __fastcall LEVEL_FreeDrlgDeletes(Game* pGame);
//D2Game.0x6FC3BE40
void __fastcall LEVEL_AddClient(Game* pGame, Room1* pRoom, GameClient* pClient);
//D2Game.0x6FC3BF00
void __fastcall LEVEL_RemoveClient(Game* pGame, Room1* pRoom, GameClient* pClient);
//D2Game.0x6FC3BFB0
void __fastcall LEVEL_RemoveClientFromAdjacentRooms(Room1* pRoom, GameClient* pClient);
//D2Game.0x6FC3C010
void __fastcall LEVEL_SynchronizeDayNightCycleWithClient(Game* pGame, GameClient* pClient);
//D2Game.0x6FC3C0B0
void __fastcall LEVEL_ChangeAct(Game* pGame, GameClient* pClient, int32_t nDestinationLevelId, int32_t nTileCalc);
//D2Game.0x6FC3C410
void __fastcall LEVEL_WarpUnit(Game* pGame, UnitAny* pPlayer, int32_t nDestinationLevelId, int32_t nTileCalc);
//D2Game.0x6FC3C510
void __fastcall LEVEL_LoadAct(Game* pGame, uint8_t nAct);
//D2Game.0x6FC3C580
void __fastcall LEVEL_RemoveAllUnits(Game* pGame);
//D2Game.0x6FC3C5B0
void __fastcall LEVEL_UpdateQueuedUnitsInAllActs(Game* pGame);
