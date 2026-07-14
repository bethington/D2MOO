#pragma once

#include <Units/Units.h>

#include <DataTbls/ItemsTbls.h>
#include <DataTbls/MonsterTbls.h>
#include <D2Items.h>

#pragma pack(1)

struct ItemDrop
{
	UnitAny* pUnit;						//0x00
	Seed* pSeed;						//0x04
	Game* pGame;						//0x08
	int32_t nItemLvl;						//0x0C
	uint32_t unk0x10;						//0x10
	int32_t nId;							//0x14
	int32_t nSpawnType;						//0x18 [3 for ground spawn, 4 for inv spawn]
	int32_t nX;								//0x1C
	int32_t nY;								//0x20
	Room1* pRoom;						//0x24
	uint16_t wUnitInitFlags;				//0x28
	uint16_t wItemFormat;					//0x2A [ptGame0x0x78]
	BOOL bForce;							//0x2C
	int32_t nQuality;						//0x30
	int32_t nQuantity;						//0x34
	int32_t nMinDur;						//0x38
	int32_t nMaxDur;						//0x3C
	int32_t nItemIndex;						//0x40
	uint32_t dwFlags1;						//0x44 - itemflag override (used when force is true)
	uint32_t dwSeed;						//0x48 - overrides the seed, used when force is true
	uint32_t dwItemSeed;					//0x4C - overrides the item seed when force is true
	int32_t eEarLvl;						//0x50
	int32_t nQtyOverride;					//0x54
	char szName[16];						//0x58
	int32_t nPrefix[3];						//0x68
	int32_t nSuffix[3];						//0x74
	uint32_t dwFlags2;						//0x80
};

#pragma pack()

//D2Game.0x6FC4D470
UniqueItemsTxt* __fastcall ITEMS_GetUniqueItemsTxtRecord(int32_t nUniqueItemId);
//D2Game.0x6FC4D4A0
uint32_t __fastcall ITEMS_HasUniqueBeenDroppedAlready(Game* pGame, int32_t nUniqueItemId);
//D2Game.0x6FC4D4E0
int32_t __fastcall ITEMS_CanUniqueItemBeDropped(Game* pGame, UnitAny* pItem);
//D2Game.0x6FC4D5E0
int32_t __fastcall sub_6FC4D5E0(UnitAny** ppItem, ItemDrop* pItemDrop);
//D2Game.0x6FC4D6B0
void __fastcall sub_6FC4D6B0(Game* pGame, UnitAny* pItem, ItemDrop* pItemDrop);
//D2Game.0x6FC4D800
void __fastcall ITEMS_MakeEthereal(UnitAny* pItem, ItemDrop* pItemDrop);
//D2Game.0x6FC4D900
void __fastcall sub_6FC4D900(Game* pGame, UnitAny* pItem, ItemDrop* pItemDrop);
//D2Game.0x6FC4DA10
int32_t __fastcall ITEMS_AssignCharmAffixes(Game* pGame, UnitAny* pItem, ItemDrop* pItemDrop);
//D2Game.0x6FC4DB60
void __fastcall D2GAME_ITEMS_AssignSpecial_6FC4DB60(Game* pGame, UnitAny* pItem, ItemDrop* pItemDrop);
//D2Game.0x6FC4DC20
void __fastcall sub_6FC4DC20(Game* pGame, UnitAny* pItem, ItemDrop* pItemDrop);
//D2Game.0x6FC4DE00
int32_t __fastcall ITEMS_RollItemQuality(UnitAny* pItem, ItemDrop* pItemDrop);
//D2Game.0x6FC4E1A0
void __fastcall sub_6FC4E1A0(UnitAny** ppItem);
//D2Game.0x6FC4E1F0
void __fastcall sub_6FC4E1F0(Game* pGame, ItemDrop* pItemDrop, UnitAny** ppItem, int32_t nLowSeed);
//D2Game.0x6FC4E430
int32_t __fastcall sub_6FC4E430(Game* pGame, ItemDrop* pItemDrop, UnitAny** ppItem, uint32_t* pLowSeed);
//D2Game.0x6FC4E4D0
uint32_t __fastcall ITEMS_RollLimitedRandomNumber(Seed* pSeed, int32_t nMax);
//D2Game.0x6FC4E520
int32_t __fastcall D2GAME_InitItemStats_6FC4E520(Game* pGame, UnitAny** ppUnit, ItemDrop* pItemDrop, int32_t bQuestItem);
//D2Game.0x6FC4EBF0
uint32_t __fastcall ITEMS_RollRandomNumber(Seed* pSeed);
//D2Game.0x6FC4EC10
UnitAny* __fastcall sub_6FC4EC10(Game* pGame, Room1* pRoom, BYTE* pBitstream, uint32_t nBufferSize, int32_t bCheckForHeader, ItemSave* pItemSave, uint32_t* pSize, uint32_t dwVersion);
//D2Game.0x6FC4ED80
UnitAny* __fastcall D2GAME_CreateItemEx_6FC4ED80(Game* pGame, ItemDrop* pItemDrop, int32_t a3);
//D2Game.0x6FC4F290
UnitAny* __fastcall sub_6FC4F290(Game* pGame, Room1* pRoom, Coord* pCoords, ItemDrop* pItemDrop);
//D2Game.0x6FC4F4A0
int32_t __fastcall sub_6FC4F4A0(Game* pGame, Seed* pSeed, int32_t nLevelId, int32_t a4, int32_t a5, int32_t a6);
//D2Game.0x6FC4F640
void __fastcall sub_6FC4F640(Game* pGame, Room1* pRoom, Coord* pCoord);
//D2Game.0x6FC4F830
UnitAny* __fastcall D2GAME_DropArmor_6FC4F830(Game* pGame, Room1* pRoom, Coord* pCoords, int32_t a4, int32_t a5, UnitAny* pUnit);
//D2Game.0x6FC4FA50
UnitAny* __fastcall D2GAME_DropWeapon_6FC4FA50(Game* pGame, Room1* pRoom, Coord* pCoords, int32_t a4, int32_t a5, UnitAny* pUnit);
//D2Game.0x6FC4FCA0
UnitAny* __fastcall sub_6FC4FCA0(Game* pGame, Room1* pRoom, Coord* pCoord, int32_t a4, int32_t a5, UnitAny* pUnit);
//D2Game.0x6FC4FEC0
UnitAny* __fastcall D2GAME_DropItemAtUnit_6FC4FEC0(Game* pGame, UnitAny* pUnit, int32_t nQuality, int32_t* pItemLevel, ItemDrop* pItemDrop, int32_t a6, int32_t a7);
//D2Game.0x6FC501A0
UnitAny* __fastcall D2GAME_CreateItemUnit_6FC501A0(UnitAny* pPlayer, int32_t nItemId, Game* pGame, int32_t nSpawnTarget, int32_t nQuality, int32_t us1, int32_t alw1, int32_t nItemLevel, int32_t us0, int32_t a1, int32_t alw0);
//D2Game.0x6FC502B0
void __fastcall sub_6FC502B0(Game* pGame, UnitAny* pItem);
//D2Game.0x6FC502E0
void __fastcall ITEMS_RemoveAll(Game* pGame);
//D2Game.0x6FC50320
void __fastcall ITEMS_RemoveFromAllPlayers(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC50340
void __fastcall ITEMS_PlayerIterateCallback_RemoveItem(Game* pGame, UnitAny* pUnit, void* pArg);
//D2Game.0x6FC503A0
int32_t __fastcall ITEMS_IsInPlayersInventory(UnitAny* pPlayer, UnitAny* pItem, Inventory* pInventory);
//D2Game.0x6FC50410
UnitAny* __fastcall ITEMS_FindQuestItem(Game* pGame, UnitAny* pUnit, uint32_t dwItemCode);
//D2Game.0x6FC504F0
int32_t __fastcall ITEMS_GetItemLevelForNewItem(UnitAny* pUnit, int32_t nLevelId);
//D2Game.0x6FC50560
void __fastcall ITEMS_UpdateDurability(Game* pGame, UnitAny* pUnit, UnitAny* pItem);
//D2Game.0x6FC50820
void __fastcall ITEMS_FillItemDrop(Game* pGame, ItemDrop* pItemDrop, UnitAny* pItem);
//D2Game.0x6FC509F0
void __fastcall ITEMS_DropGoldPile(Game* pGame, UnitAny* pUnit, uint32_t nValue);
//D2Game.0x6FC50C50
void __fastcall ITEMS_HandleGoldTransaction(Game* pGame, UnitAny* pUnit, uint32_t nValue);
//D2Game.0x6FC50D80
void __fastcall ITEMS_DropPlayerEar(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC50F70
int32_t __fastcall ITEMS_GetHealthPotionDropCode(UnitAny* pUnit);
//D2Game.0x6FC50FF0
int32_t __fastcall ITEMS_GetManaPotionDropCode(UnitAny* pUnit);
//D2Game.0x6FC51070
UnitAny* __fastcall ITEMS_Duplicate(Game* pGame, UnitAny* pItem, UnitAny* pOwner, int32_t bDuplicateSocketFillers);
//D2Game.0x6FC512C0
void __fastcall sub_6FC512C0(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC51310
void __fastcall sub_6FC51310(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC51360
void __fastcall D2GAME_DropTC_6FC51360(Game* pGame, UnitAny* pMonster, UnitAny* pPlayer, TCExShort* pTCTxtRecord, int32_t nQuality, int32_t nLvl, int32_t a7, UnitAny** ppItems, int32_t* pnItemsDropped, int32_t nMaxItems);
//D2Game.0x6FC52070
Room1* __fastcall D2GAME_GetRoom_6FC52070(Room1* pRoom, int32_t nSubtileX, int32_t nSubtileY);
//D2Game.0x6FC52110
void __fastcall sub_6FC52110(Game* pGame, UnitAny* pMonster, UnitAny* pPlayer, int32_t nTCId, int32_t nQuality, int32_t nItemLevel, int32_t a7, UnitAny** ppItems, int32_t* pnItemsDropped, int32_t nMaxItems);
//D2Game.0x6FC521D0
int32_t __fastcall ITEMS_GetGroundRemovalTime(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC52260
void __fastcall D2GAME_DropItem_6FC52260(Game* pGame, UnitAny* pUnit, UnitAny* pItem, Room1* pRoom, int32_t nX, int32_t nY);
//D2Game.0x6FC52300
void __fastcall ITEMS_DeleteInactiveItems(Game* pGame, int32_t nAct);
//D2Game.0x6FC523B0
void __fastcall ITEMS_DestroyRunewordStatList(Game* pGame, UnitAny* pItem);
//D2Game.0x6FC52410
void __fastcall sub_6FC52410(UnitAny* pUnit, ItemDrop* pItemDrop);
//D2Game.0x6FC52650
void __fastcall sub_6FC52650(UnitAny* pUnit, int32_t nItemLvl, int32_t nClassFirstSkillId, int32_t nSkillCount, int32_t nItemDropLvl);
