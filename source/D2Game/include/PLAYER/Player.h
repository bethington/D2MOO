#pragma once

#include <Units/Units.h>
#include <D2DataTbls.h>


//D2Game.0x6FC7B550
void __fastcall PLAYER_Create(Game* pGame, UnitAny* pPlayer, int32_t nPlayerGUID);
//D2Game.0x6FC7B630
void __fastcall PLAYER_Destroy(Game* pGame, UnitAny* pPlayer);
//D2Game.0x6FC7B750
void __fastcall PLAYER_RemoveAllPlayers(Game* pGame);
//D2Game.0x6FC7B7A0
void __fastcall sub_6FC7B7A0(Game* pGame, UnitAny* pUnit, int32_t nX, int32_t nY);
//D2Game.0x6FC7B800
void __fastcall PLAYER_CreateStartItem(Game* pGame, UnitAny* pPlayer, CharStatsTxt* pCharStatsTxt, int32_t nCount, CharItem* pCharItem);
//D2Game.0x6FC7BB50
void __fastcall PLAYER_PlaceItemInInventory(UnitAny* pItem, UnitAny* pPlayer, Game* pGame, int32_t a8);
//D2Game.0x6FC7BC30
void __fastcall PLAYER_CreateStartItemsFromCharStatsTxt(UnitAny* pPlayer, Game* pGame, void* pUnused);
//D2Game.0x6FC7BC90
void __fastcall PLAYER_SynchronizeItemsToClient(UnitAny* pUnit, GameClient* pClient);
//D2Game.0x6FC7BD50
int32_t __fastcall PLAYER_IsBusy(UnitAny* pUnit);
//D2Game.0x6FC7BDB0
void __fastcall PLAYER_ResetBusyState(UnitAny* pUnit);
//D2Game.0x6FC7BDF0
void __fastcall PLAYER_StopInteractions(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC7BEC0
void __fastcall sub_6FC7BEC0(Game* pGame, UnitAny* pUnit);
//1.10f: D2Game.0x6FC7BFC0
//1.13c: D2Game.0x6FC57B10
void __fastcall sub_6FC7BFC0(Game* pGame, Room1* pRoom, int32_t nPlayerGUID, Coord* pCoord);
//D2Game.0x6FC7C0C0
void __fastcall PLAYER_SetUniqueIdInPlayerData(UnitAny* pPlayer, int32_t nUnitGUID);
//D2Game.0x6FC7C120
int32_t __fastcall PLAYER_GetUniqueIdFromPlayerData(UnitAny* pPlayer);
//D2Game.0x6FC7C170
void __fastcall sub_6FC7C170(Game* pGame, UnitAny* pPlayer);
//D2Game.0x6FC7C260
int32_t __fastcall sub_6FC7C260(Game* pGame, UnitAny* pUnit, int32_t nUnitGUID, uint32_t nValue);
//D2Game.0x6FC7C3A0
int32_t __fastcall sub_6FC7C3A0(Game* pGame, UnitAny* pPlayer, UnitAny* a3);
//D2Game.0x6FC7C450
int32_t __fastcall sub_6FC7C450(Game* pGame, UnitAny* pPlayer, int32_t nHotkeyId, int16_t nSkillId, char a4, int32_t nFlags);
//D2Game.0x6FC7C490 (#10059)
void __fastcall PLAYER_SetVirtualPlayerCount(int32_t nPlayers);
//D2Game.0x6FC7C4A0
int32_t __fastcall PLAYER_GetPlayerCount(Game* pGame);
//D2Game.0x6FC7C4E0
void __fastcall PLAYER_CountLivingPlayers(Game* pGame, UnitAny* pUnit, void* pLivingPlayers);
//D2Game.0x6FC7C500
void __fastcall PLAYER_ApplyDeathPenalty(Game* pGame, UnitAny* pDefender, UnitAny* pAttacker);
//D2Game.0x6FC7C750
int32_t __fastcall D2GAME_IteratePlayers_6FC7C750(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC7C790
void __fastcall sub_6FC7C790(Game* pGame, UnitAny* pUnit, void* pArgs);
//D2Game.0x6FC7C7B0
UnitAny* __fastcall sub_6FC7C7B0(UnitAny* pUnit);
//D2Game.0x6FC7C900
void __fastcall sub_6FC7C900(UnitAny* pAttacker, int32_t a2);
//D2Game.0x6FC7CA10
void __fastcall sub_6FC7CA10(UnitAny* pTarget, int32_t a2);
//D2Game.0x6FC7CA70
void __fastcall D2GAME_SetStatOrResetGold_6FC7CA70(UnitAny* pUnit, int32_t nStat, int32_t nValue);
