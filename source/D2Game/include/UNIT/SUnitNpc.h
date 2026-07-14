#pragma once

#include <Units/Units.h>
#include "SUnitProxy.h"

#pragma pack(1)

struct NpcTrade
{
	struct
	{
		bool bVendorInit;		//0x00
		bool bHireInit;			//0x01
		uint8_t nAct;			//0x02
		bool bTrader;			//0x03
		bool bLevelRefresh;		//0x04
		bool bInited;			//0x05
		bool bForceVendor;		//0x06
		bool bRefreshInventory;	//0x07
	};

	uint32_t dwTicks;				//0x08
	UnitProxy pProxy;			//0x0C
	uint32_t dwUnk;					//0x1C
	D2UnitGUID dwNPCGUID;			//0x20
};

struct NpcControl
{
	int32_t nArraySize;							//0x00
	NpcRecord* pFirstRecord;			//0x04
	Seed pSeed;						//0x08
	int32_t unk0x10;							//0x10
};

struct NpcGamble
{
	Inventory* pInventory;			//0x00
	D2UnitGUID dwGUID;							//0x04
	NpcGamble* pNext;					//0x08
};

struct NPCMessageList
{
	uint16_t nMessageIndexes[8];				//0x00
};

struct NpcVendorChain
{
	int dwGUID;
	uint8_t field_4;
	uint8_t unk0x05[3];
	NpcVendorChain* pNext;
};

struct NpcEvent
{
	UnitAny* pUnit;
	int32_t field_4;
	int32_t field_8;
	int32_t field_C;
	NpcEvent* pNext;
};

struct MercData
{
	int16_t nMercName;						//0x00
	int16_t nPad;								//0x02
	uint32_t dwSeed;							//0x04
	BOOL bHired;							//0x08
	BOOL bAvailable;						//0x0C
};

struct NpcRecord
{
	int32_t nNPC;							//0x00
	Inventory* pInventory;			//0x04
	NpcGamble* pGamble;				//0x08
	BOOL bGambleInit;						//0x0C
	MercData* pMercData;				//0x10
	NpcEvent* pEvent;					//0x14
	NpcVendorChain* pVendorChain;		//0x18
	BOOL bTrading;							//0x1C
	NpcTrade npcTrade;				//0x20
};

#pragma pack()

//D2Game.0x6FCC67D0
void __fastcall D2GAME_NPC_FirstFn_6FCC67D0(Game* pGame, int32_t nVendorId, NpcRecord* pNpcRecord);
//D2Game.0x6FCC6970
void __fastcall D2GAME_NPC_RepairItem_6FCC6970(Game* pGame, UnitAny* pItem, UnitAny* pPlayer);
//D2Game.0x6FCC6A60
UnitAny* __fastcall D2GAME_NPC_GenerateStoreItem_6FCC6A60(UnitAny* pNpc, int32_t szCode, Game* pGame, int32_t a4, int32_t nQuality, int32_t nItemLevel, int32_t nPlayerLevel);
//D2Game.0x6FCC6F10
void __fastcall sub_6FCC6F10(UnitAny* pNpc, UnitAny* pItem, Game* pGame, int32_t bInit);
//D2Game.0x6FCC6FF0
void __fastcall D2GAME_NPC_BuildHirelingList_6FCC6FF0(Game* pGame, GameClient* pClient, UnitAny* pUnit, int32_t a4);
//D2Game.0x6FCC7100
void __fastcall D2GAME_NPC_FillStoreInventory_6FCC7100(Game* pGame, UnitAny* pPlayer, UnitAny* pNpc, NpcTrade* pTrade);
//D2Game.0x6FCC74F0
void __fastcall D2GAME_STORES_CreateVendorCache_6FCC74F0(Game* pGame, UnitAny* pPlayer, UnitAny* pNPC, int32_t a4, int32_t a5);
//D2Game.0x6FCC7680
int32_t __fastcall D2GAME_STORES_SellItem_6FCC7680(Game* pGame, UnitAny* pPlayer, int32_t nNpcGUID, int32_t nItemGUID, int16_t nItemMode, int32_t a6);
//D2Game.0x6FCC7E20
void __fastcall sub_6FCC7E20(Game* pGame, UnitAny* pNpc, UnitAny* pItem, UnitAny* pUnit, int32_t a5);
//D2Game.0x6FCC7FA0
void __fastcall sub_6FCC7FA0(Game* pGame, UnitAny* pPlayer, UnitAny* pNpc, uint16_t wName);
//D2Game.0x6FCC8430
void __fastcall sub_6FCC8430(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCC84D0
void __fastcall sub_6FCC84D0(Game* pGame, UnitAny* pPlayer, UnitAny* pPet);
//D2Game.0x6FCC8630
UnitAny* __fastcall D2GAME_MERCS_Create_6FCC8630(Game* pGame, UnitAny* pPlayer, uint16_t wName, uint32_t nSeed, int16_t wVersion, int32_t nBaseMonster, int32_t bDead);
//D2Game.0x6FCC87C0
UnitAny* __fastcall sub_6FCC87C0(Game* pPlayer, UnitAny* pUnit, UnitAny* pItem, int32_t* a4);
//D2Game.0x6FCC88B0
int32_t __fastcall sub_6FCC88B0(Game* pGame, UnitAny* pPlayer, UnitAny* pNpc, int32_t nItemGUID, int32_t nItemMode, uint16_t nTransactionType, int32_t nCost, int32_t bMultibuy);
//D2Game.0x6FCC92A0
int32_t __fastcall D2GAME_NPC_BuyItemHandler_6FCC92A0(Game* pGame, UnitAny* pPlayer, int32_t nNpcUnitId, int32_t nItemId, int32_t nItemMode, uint16_t nTransactionType, int32_t nCost, int32_t bMultibuy);
//D2Game.0x6FCC9350
void __fastcall D2GAME_NPC_ResurrectMerc_6FCC9350(Game* pGame, UnitAny* pPlayer, int32_t nNpcUnitId);
//D2Game.0x6FCC9540
void __fastcall D2GAME_NPC_HireMerc_6FCC9540(Game* pGame, UnitAny* pPlayer, int32_t nNpcUnitId, uint16_t a4);
//D2Game.0x6FCC95B0
int32_t __fastcall D2GAME_NPC_Repair_6FCC95B0(Game* pGame, UnitAny* pUnit, int32_t nNpcGUID, int32_t nItemGUID, int32_t nUnused, int32_t a6);
//D2Game.0x6FCC9C90
void __fastcall D2GAME_NPC_IdentifyAllItems_6FCC9C90(Game* pGame, UnitAny* pPlayer, int32_t nNpcGUID);
//D2Game.0x6FCC9F40
int32_t __fastcall NPC_HandleDialogMessage(Game* pGame, UnitAny* pPlayer, int32_t nType, int32_t nNpcGUID, int32_t nItemGUID);
//D2Game.0x6FCCA990
void __fastcall D2GAME_NPC_IdentifyBoughtItem_6FCCA990(Game* pGame, UnitAny* pPlayer, int32_t nItemGUID);
//D2Game.0x6FCCA9F0
void __fastcall D2GAME_STORES_FillGamble_6FCCA9F0(Game* pGame, UnitAny* pNpc, UnitAny* pUnit, NpcRecord* pNpcRecord);
//D2Game.0x6FCCAE20
void __fastcall D2GAME_STORES_CreateVendorCache_6FCCAE20(Game* pGame, UnitAny* pPlayer, UnitAny* pNpc, int32_t bGamble);
//D2Game.0x6FCCAF30
int32_t __fastcall D2GAME_NPC_RemoveStates_6FCCAF30(UnitAny* pUnit);
//D2Game.0x6FCCAFA0
void __fastcall SUNITNPC_PetIterate_Heal(Game* pGame, UnitAny* a2, UnitAny* pUnit, void* a4);
//D2Game.0x6FCCB080
void __fastcall D2GAME_NPC_HealPlayer_6FCCB080(Game* pGame, UnitAny* pUnit, UnitAny* pNpc);
//D2Game.0x6FCCB220
void __fastcall D2GAME_NPC_Heal_6FCCB220(Game* pGame, UnitAny* pUnit, UnitAny* pNpc);
//D2Game.0x6FCCB280
void __fastcall D2GAME_NPC_PurchaseHeal_6FCCB280(Game* pGame, UnitAny* pUnit, int32_t nNpcGUID);
//D2Game.0x6FCCB4D0
void __fastcall D2GAME_NPC_ResetInteract_6FCCB4D0(Game* pGame, UnitAny* pPlayer, UnitAny* pNpc);
//D2Game.0x6FCCB520
void __fastcall D2GAME_NPC_AssignMercenary_6FCCB520(Game* pGame, UnitAny* pPlayer, int32_t nMonster);
//D2Game.0x6FCCB7D0
int32_t __fastcall D2GAME_NPC_IsItemInNpcInventory_6FCCB7D0(Game* pGame, UnitAny* pPlayer, UnitAny* pNpc, UnitAny* pItem, int32_t a4);
