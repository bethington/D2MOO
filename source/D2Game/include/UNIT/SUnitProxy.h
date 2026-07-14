#pragma once

#include <Units/Units.h>
#include "GAME/Game.h"

#pragma pack(1)

struct NpcRecord;
struct NpcVendorChain;

struct ItemCache
{
	char nMin;								//0x00
	char nMax;								//0x01
	char nMagicMin;							//0x02
	char nMagicMax;							//0x03
	uint32_t dwCode;							//0x04
	int32_t nMagicLevel;						//0x08
};

struct UnitProxy
{
	ItemCache* pItemCache;			//0x00
	int32_t nItems;								//0x04
	uint32_t* pPermCache;						//0x08
	int32_t nPerms;								//0x0C
};

#pragma pack()


//D2Game.0x6FCCB8A0
NpcRecord* __fastcall SUNITPROXY_GetNpcRecordFromClassId(Game* pGame, int32_t nNpcClassId, int32_t* pIndex);
//D2Game.0x6FCCB910
NpcRecord* __fastcall SUNITPROXY_GetNpcRecordFromUnit(Game* pGame, UnitAny* pNpc, int32_t* pIndex);
//D2Game.0x6FCCB980
void __fastcall SUNITPROXY_AllocNpcEvent(UnitAny* pNpc, Game* pGame, UnitAny* pPlayer, int32_t a4);
//D2Game.0x6FCCBA30
void __fastcall SUNITPROXY_InitializeNpcControl(Game* pGame);
//D2Game.0x6FCCBF50
void __fastcall SUNITPROXY_InitializeItemCache(Game* pGame, UnitProxy* pUnitProxy, void* pUnused, int32_t nNpcInventoryId);
//D2Game.0x6FCCC030
void __fastcall SUNITPROXY_FreeNpcControl(Game* pGame);
//D2Game.0x6FCCC140
void __fastcall SUNITPROXY_ClearNpcRecordData(Game* pGame, NpcRecord* pNpcRecord);
//D2Game.0x6FCCC2E0
void __fastcall SUNITPROXY_UpdateNpcsOnActChange(Game* pGame, UnitAny* pUnit, int32_t nCurrentLevelId, int32_t nDestLevelId);
//D2Game.0x6FCCC540
void __fastcall SUNITPROXY_UpdateVendorInventory(Game* pGame, UnitAny* pUnit, uint8_t nAct, int32_t bNoMorePlayersInLevel);
//D2Game.0x6FCCC690
void __fastcall SUNITPROXY_CountPlayersInLevel(Game* pGame, UnitAny* pUnit, void* pArg);
//D2Game.0x6FCCC6B0
void __fastcall SUNITPROXY_OnClientRemovedFromGame(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCCC7C0
void __fastcall SUNITPROXY_InitializeNpcEventChain(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCCC860
Inventory* __fastcall SUNITPROXY_GetNpcInventory(Game* pGame, int32_t nNpc);
//D2Game.0x6FCCC8B0
Seed* __fastcall SUNITPROXY_GetSeedFromNpcControl(Game* pGame);
//D2Game.0x6FCCC8C0
void __fastcall SUNITPROXY_FreeVendorChain(Game* pGame, UnitAny* pPlayer, UnitAny* pNpc);
//D2Game.0x6FCCCA70
void __fastcall SUNITPROXY_UpdateGambleInventory(Game* pGame, UnitAny* pNpc, UnitAny* pPlayer, GameClient* pClient);
//D2Game.0x6FCCCB20
Inventory* __fastcall SUNITPROXY_GetGambleInventory(Game* pGame, UnitAny* pPlayer, UnitAny* pNpc);
//D2Game.0x6FCCCBB0
NpcVendorChain* __fastcall SUNITPROXY_GetVendorChain(Game* pGame, NpcRecord* pNpcRecord, UnitAny* pNpc);
//D2Game.0x6FCCCC00
void __fastcall SUNITPROXY_AllocNpcInventory(Game* pGame, NpcRecord* pNpcRecord, UnitAny* pNPC);
//D2Game.0x6FCCCC40
void __fastcall SUNITPROXY_FillGlobalItemCache();
//D2Game.0x6FCCCED0
void __fastcall SUNITPROXY_FillIGlobaltemCacheRecordForNpc(int32_t nNpcId);
//D2Game.0x6FCCD120
void __fastcall SUNITPROXY_ClearGlobalItemCache();
//D2Game.0x6FCCD190
void __fastcall SUNITPROXY_FreeNpcGamble(Game* pGame, UnitAny* pNpc, UnitAny* pPlayer);
