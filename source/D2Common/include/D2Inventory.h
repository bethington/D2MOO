#pragma once

#include "D2CommonDefinitions.h"


struct UnitAny;
struct InventoryGridInfo;

#pragma pack(1)

struct InvRect
{
	int32_t nLeft;		//0x00
	int32_t nRight;		//0x04
	int32_t nTop;		//0x08
	int32_t nBottom;	//0x0C
};

enum PlayerBodyLocs
{
	BODYLOC_NONE,		//Not Equipped
	BODYLOC_HEAD,		//Helm
	BODYLOC_NECK,		//Amulet
	BODYLOC_TORSO,		//Body Armor
	BODYLOC_RARM,		//Right-Hand
	BODYLOC_LARM,		//Left-Hand
	BODYLOC_RRIN,		//Right Ring
	BODYLOC_LRIN,		//Left Ring
	BODYLOC_BELT,		//Belt
	BODYLOC_FEET,		//Boots
	BODYLOC_GLOVES,		//Gloves
	BODYLOC_SWRARM,		//Right-Hand on Switch
	BODYLOC_SWLARM,		//Left-Hand on Switch
	NUM_BODYLOC
};

#define InventoryHeader 0x1020304

enum ItemInvPage
{
	INVPAGE_INVENTORY = 0,
	INVPAGE_EQUIP = 1,
	INVPAGE_TRADE = 2,
	INVPAGE_CUBE = 3,
	INVPAGE_STASH = 4,
	INVPAGE_BELT = 5,
	INVPAGE_NULL = 255
};

enum InventoryRecords
{
	// 640x420
	INVENTORYRECORD_AMAZON,
	INVENTORYRECORD_SORCERESS,
	INVENTORYRECORD_NECROMANCER,
	INVENTORYRECORD_PALADIN,
	INVENTORYRECORD_BARBARIAN,
	INVENTORYRECORD_MONSTER,
	INVENTORYRECORD_TRADE_PAGE_1,
	INVENTORYRECORD_TRADE_PAGE_2,
	INVENTORYRECORD_BANK_PAGE_1,
	INVENTORYRECORD_TRANSMOGRIFY_BOX_PAGE_1,
	INVENTORYRECORD_GUILD_VAULT_PAGE_1,
	INVENTORYRECORD_TROPHY_CASE_PAGE_1,
	INVENTORYRECORD_BIG_BANK_PAGE_1,
	INVENTORYRECORD_HIRELING,
	INVENTORYRECORD_DRUID,
	INVENTORYRECORD_ASSASSIN,
	NUM_INVENTORY_PAGE_STATS,

	// 800x600
	INVENTORYRECORD_AMAZON2 = NUM_INVENTORY_PAGE_STATS,
	INVENTORYRECORD_SORCERESS2,
	INVENTORYRECORD_NECROMANCER2,
	INVENTORYRECORD_PALADIN2,
	INVENTORYRECORD_BARBARIAN2,
	INVENTORYRECORD_MONSTER2,
	INVENTORYRECORD_TRADE_PAGE_1_2,
	INVENTORYRECORD_TRADE_PAGE_2_2,
	INVENTORYRECORD_BANK_PAGE2,
	INVENTORYRECORD_TRANSMOGRIFY_BOX2,
	INVENTORYRECORD_GUILD_VAULT_PAGE2,
	INVENTORYRECORD_TROPHY_CASE_PAGE2,
	INVENTORYRECORD_BIG_BANK_PAGE2,
	INVENTORYRECORD_HIRELING2,
	INVENTORYRECORD_DRUID2,
	INVENTORYRECORD_ASSASSIN2
};

enum NodePages
{
	NODEPAGE_STORAGE = 1,
	NODEPAGE_BELTSLOTS = 2,
	NODEPAGE_EQUIP = 3
};

enum InventoryGrids
{
	INVGRID_BODYLOC,
	INVGRID_BELT,
	INVGRID_INVENTORY,
};

enum TradeStates
{
	TRADESTATE_OTHERNOROOM,
	TRADESTATE_SELFNOROOM,
};

struct InventoryGrid
{
	UnitAny* pItem;						//0x00
	UnitAny* pLastItem;					//0x04
	uint8_t nGridWidth;						//0x08
	uint8_t nGridHeight;					//0x09
	uint16_t pad0x0A;						//0x0A
	UnitAny** ppItems;					//0x0C
};

struct Corpse
{
	uint32_t unk0x00;						//0x00
	uint32_t dwUnitId;						//0x04
	uint32_t unk0x08;						//0x08
	Corpse* pNextCorpse;				//0x0C
};

struct InventoryNode
{
	int32_t nItemId;						//0x00
	InventoryNode* pNext;				//0x04
};

struct Inventory
{
	uint32_t dwSignature;					//0x00
	void* pMemPool;							//0x04
	UnitAny* pOwner;						//0x08
	UnitAny* pFirstItem;					//0x0C
	UnitAny* pLastItem;					//0x10
	InventoryGrid* pGrids;			//0x14
	int32_t nGridCount;						//0x18
	D2UnitGUID dwLeftItemGUID;				//0x1C
	UnitAny* pCursorItem;				//0x20
	D2UnitGUID dwOwnerGuid;					//0x24
	uint32_t dwItemCount;					//0x28
	InventoryNode* pFirstNode;		//0x2C
	InventoryNode* pLastNode;			//0x30
	Corpse* pFirstCorpse;				//0x34
	Corpse* pLastCorpse;				//0x38
	int32_t nCorpseCount;					//0x3C
};

struct ItemExtraData
{
	Inventory* pParentInv;			//0x00
	UnitAny* pPreviousItem;				//0x04
	UnitAny* pNextItem;					//0x08
	char nNodePos;							//0x0C
	char nNodePosOther;						//0x0D
	uint16_t unk0x0E;						//0x0E
	UnitAny* unk0x10;					//0x10
	UnitAny* unk0x14;					//0x14
};
#pragma pack()

// Helper functions
// Check if the header signature is correct. Assumes non null ptr.
inline bool INVENTORY_CheckSignature(Inventory* pInventory) { return pInventory->dwSignature == InventoryHeader; }
// Check if ptr is non null and if header signature is correct.
inline Inventory* INVENTORY_GetPtrIfValid(Inventory* pInventory) { return (pInventory && INVENTORY_CheckSignature(pInventory)) ? pInventory : nullptr; }
// Return true if matches a valid body location
inline bool INVENTORY_ValidateBodyLoc(int nBodyLoc) { return nBodyLoc >= 0 && nBodyLoc < NUM_BODYLOC; }

//D2Common.0x6FD8E210
BOOL __fastcall INVENTORY_RemoveItem(UnitAny* pItem);
//D2Common.0x6FD8E4A0
ItemExtraData* __fastcall INVENTORY_GetItemExtraDataFromItem(UnitAny* pItem);
//D2Common.0x6FD8E4C0 (#10240)
D2COMMON_DLL_DECL Inventory* __stdcall INVENTORY_AllocInventory(void* pMemPool, UnitAny* pOwner);
//D2Common.0x6FD8E520 (#10241)
D2COMMON_DLL_DECL void __stdcall INVENTORY_FreeInventory(Inventory* pInventory);
//D2Common.0x6FD8E620 (#10244)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_CompareWithItemsParentInventory(Inventory* pInventory, UnitAny* pItem);
//D2Common.0x6FD8E660 (#10243)
D2COMMON_DLL_DECL UnitAny* __stdcall INVENTORY_RemoveItemFromInventory(Inventory* pInventory, UnitAny* pItem);
//D2Common.0x6FD8E6A0 (#10242)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_PlaceItemInSocket(Inventory* pInventory, UnitAny* pItem, int nUnused);
//D2Common.0x6FD8E7A0 (#10277)
D2COMMON_DLL_DECL UnitAny* __stdcall INVENTORY_GetFirstItem(Inventory* pInventory);
//D2Common.0x6FD8E7C0 (#10278)
D2COMMON_DLL_DECL UnitAny* __stdcall INVENTORY_GetLastItem(Inventory* pInventory);
//D2Common.0x6FD8E7E0 (#10245)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_GetFreePosition(Inventory* pInventory, UnitAny* pItem, int nInventoryRecordId, int* pFreeX, int* pFreeY, uint8_t nPage);
//D2Common.0x6FD8EAF0
InventoryGrid* __fastcall INVENTORY_GetGrid(Inventory* pInventory, int nInventoryGrid, InventoryGridInfo* pInventoryGridInfo);
//D2Common.0x6FD8EC70
BOOL __fastcall INVENTORY_CanItemBePlacedAtPos(InventoryGrid* pInventoryGrid, int nX, int nY, uint8_t nItemWidth, uint8_t nItemHeight);
//D2Common.0x6FD8ECF0
BOOL __fastcall INVENTORY_FindFreePositionBottomRightToTopLeftWithWeight(InventoryGrid* pInventoryGrid, int* pFreeX, int* pFreeY, uint8_t nItemWidth, uint8_t nItemHeight);
//D2Common.0x6FD8EE20
uint8_t __fastcall INVENTORY_GetPlacementWeight(InventoryGrid* pInventoryGrid, int nXPos, int nYPos, uint8_t nItemWidth, uint8_t nItemHeight);
//D2Common.0x6FD8EFB0
BOOL __fastcall INVENTORY_FindFreePositionTopLeftToBottomRightWithWeight(InventoryGrid* pInventoryGrid, int* pFreeX, int* pFreeY, uint8_t nItemWidth, uint8_t nItemHeight);
//D2Common.0x6FD8F0E0
BOOL __fastcall INVENTORY_FindFreePositionTopLeftToBottomRight(InventoryGrid* pInventoryGrid, int* pFreeX, int* pFreeY, uint8_t nItemWidth, uint8_t nItemHeight);
//D2Common.0x6FD8F1E0 (#10246)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_PlaceItemAtFreePosition(Inventory* pInventory, UnitAny* pItem, int nInventoryRecordId, BOOL bUnused, uint8_t nPage, const char* szFile, int nLine);
//D2Common.0x6FD8F250
BOOL __fastcall INVENTORY_PlaceItemInGrid(Inventory* pInventory, UnitAny* pItem, int nXPos, int nYPos, int nInventoryGrid, int nInventoryRecordId, BOOL bUnused);
//D2Common.0x6FD8F600 (#10247)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_CanItemBePlaced(Inventory* pInventory, UnitAny* pItem, int nXPos, int nYPos, int nInventoryRecordId, UnitAny** ppExchangeItem, unsigned int* pHoveredItems, uint8_t nPage);
//D2Common.0x6FD8F780 (#10248)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_CanItemsBeExchanged(Inventory* pInventory, UnitAny* pItem, int nXPos, int nYPos, int nInventoryRecordId, UnitAny** ppExchangeItem, uint8_t nPage, BOOL bCheckIfCube);
//D2Common.0x6FD8F930 (#10249)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_PlaceItemAtInventoryPage(Inventory* pInventory, UnitAny* pItem, int nXPos, int nYPos, int nInventoryRecordId, BOOL bUnused, uint8_t nPage);
//D2Common.0x6FD8F970 (#10250)
D2COMMON_DLL_DECL void __stdcall INVENTORY_Return(const char* szFile, int nLine, Inventory* pInventory, int nX, int nY, int nInventoryRecordId, BOOL bClient, uint8_t nPage);
//D2Common.0x6FD8F980 (#10252)
D2COMMON_DLL_DECL UnitAny* __stdcall INVENTORY_GetItemFromInventoryPage(Inventory* pInventory, int nGridX, int nGridY, int* pX, int* pY, int nInventoryRecordId, uint8_t nPage);
//D2Common.0x6FD8FAB0 (#10253)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_PlaceItemInBodyLoc(Inventory* pInventory, UnitAny* pItem, int nBodyLoc);
//D2Common.0x6FD8FAE0 (#10257)
D2COMMON_DLL_DECL UnitAny* __stdcall INVENTORY_GetItemFromBodyLoc(Inventory* pInventory, int nBodyLoc);
//D2Common.0x6FD8FB20 (#10255)
D2COMMON_DLL_DECL void __stdcall INVENTORY_GetSecondWieldingWeapon(UnitAny* pPlayer, Inventory* pInventory, UnitAny** ppItem, int nBodyLoc);
//D2Common.0x6FD8FBB0 (#10256)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_CheckEquipmentForWeaponByClass(Inventory* pInventory, int nWeaponClass);
//D2Common.0x6FD8FC60 (#10258)
D2COMMON_DLL_DECL UnitAny* __stdcall INVENTORY_GetLeftHandWeapon(Inventory* pInventory);
//D2Common.0x6FD8FD10 (#11301)
D2COMMON_DLL_DECL UnitAny* __stdcall INVENTORY_GetSecondaryWeapon(Inventory* pInventory);
//D2Common.0x6FD8FDD0 (#10259)
D2COMMON_DLL_DECL UnitAny* __stdcall INVENTORY_GetCompositItem(Inventory* pInventory, int nComponent);
//D2Common.0x6FD8FE80 (#10260)
D2COMMON_DLL_DECL int __stdcall INVENTORY_GetBodyLocFromEquippedItem(Inventory* pInventory, UnitAny* pItem);
//D2Common.0x6FD8FED0 (#11278)
D2COMMON_DLL_DECL int __stdcall INVENTORY_GetItemsXPosition(Inventory* pInventory, UnitAny* pItem);
//D2Common.0x6FD8FF20 (#10261)
D2COMMON_DLL_DECL void __stdcall INVENTORY_SetCursorItem(Inventory* pInventory, UnitAny* pItem);
//D2Common.0x6FD8FF80 (#10262)
D2COMMON_DLL_DECL UnitAny* __stdcall INVENTORY_GetCursorItem(Inventory* pInventory);
//D2Common.0x6FD8FFA0 (#10263)
D2COMMON_DLL_DECL UnitAny* __stdcall INVENTORY_FindBackPackItemForStack(Inventory* pInventory, UnitAny* pStackable, UnitAny* pCheckItem);
//D2Common.0x6FD90080 (#10264)
D2COMMON_DLL_DECL UnitAny* __stdcall INVENTORY_FindEquippedItemForStack(Inventory* pInventory, UnitAny* pStackable, UnitAny* pCheckItem);
//D2Common.0x6FD90130 (#10265)
D2COMMON_DLL_DECL UnitAny* __stdcall INVENTORY_FindFillableBook(Inventory* pInventory, UnitAny* pScrolls, UnitAny* pCheckItem);
//D2Common.0x6FD90230 (#10266)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_PlaceItemInBeltSlot(Inventory* pInventory, UnitAny* pItem, int nSlot);
//D2Common.0x6FD902B0 (#10268)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_HasSimilarPotionInBelt(Inventory* pInventory, UnitAny* pPotion);
//D2Common.0x6FD90340 (#10269)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_GetFreeBeltSlot(Inventory* pInventory, UnitAny* pItem, int* pFreeSlotId);
//D2Common.0x6FD904F0 (#10270)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_PlaceItemInFreeBeltSlot(Inventory* pInventory, UnitAny* pItem);
//D2Common.0x6FD90550 (#10271)
D2COMMON_DLL_DECL UnitAny* __stdcall INVENTORY_GetItemFromBeltSlot(Inventory* pInventory, int nSlotId);
//D2Common.0x6FD90590 (#10272)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_GetUseableItemFromBeltSlot(Inventory* pInventory, UnitAny* pItem, int nSlotId, UnitAny** ppItem);
//D2Common.0x6FD90690 (#10273)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_GetEquippedShield(Inventory* pInventory, UnitAny** ppItem);
//D2Common.0x6FD90760 (#10274)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_GetEquippedWeapon(Inventory* pInventory, UnitAny** ppItem, int* pBodyLoc, BOOL* pIsLeftHandItem);
//D2Common.0x6FD90850 (#10275)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_HasBodyArmorEquipped(Inventory* pInventory);
//D2Common.0x6FD908A0 (#10276)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_IsItemBodyLocFree(Inventory* pInventory, UnitAny* pItem, int nBodyLoc, int nInventoryRecordId);
//D2Common.0x6FD90910 (#10279)
D2COMMON_DLL_DECL void __stdcall INVENTORY_RemoveInventoryItems(Inventory* pInventory);
//D2Common.0x6FD90940 (#10280)
D2COMMON_DLL_DECL InventoryNode* __stdcall INVENTORY_GetTradeInventory(Inventory* pInventory);
//D2Common.0x6FD90960 (#10281)
D2COMMON_DLL_DECL void __stdcall INVENTORY_FreeTradeInventory(Inventory* pInventory);
//D2Common.0x6FD909B0 (#10282)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_CheckForItemInTradeInventory(Inventory* pInventory, int nItemId);
//D2Common.0x6FD909F0 (#10283)
D2COMMON_DLL_DECL void __stdcall INVENTORY_AddItemToTradeInventory(Inventory* pInventory, UnitAny* pItem);
//D2Common.0x6FD90AB0 (#10316)
D2COMMON_DLL_DECL int __stdcall D2Common_10316(Corpse* pCorpse);
//D2Common.0x6FD90AC0 (#10284)
D2COMMON_DLL_DECL int __stdcall INVENTORY_GetItemCount(Inventory* pInventory);
//D2Common.0x6FD90AE0 (#10285)
D2COMMON_DLL_DECL UnitAny* __stdcall INVENTORY_GetBackPackItemByType(Inventory* pInventory, int nItemType, UnitAny* pCheckItem);
//D2Common.0x6FD90BC0 (#10286)
D2COMMON_DLL_DECL UnitAny* __stdcall INVENTORY_GetEquippedItemByType(Inventory* pInventory, int nItemType, UnitAny* pCheckItem);
//D2Common.0x6FD90C80 (#10287)
D2COMMON_DLL_DECL UnitAny* __stdcall INVENTORY_GetEquippedItemByCode(Inventory* pInventory, int nItemCode, UnitAny* pCheckItem);
//D2Common.0x6FD90D50 (#11306)
D2COMMON_DLL_DECL UnitAny* __stdcall INVENTORY_GetBackPackItemByCode(Inventory* pInventory, int nItemCode, UnitAny* pCheckItem);
//D2Common.0x6FD90E20 (#10288)
D2COMMON_DLL_DECL int __stdcall INVENTORY_GetSetItemEquipCountByFileIndex(Inventory* pInventory, int nItemFileIndex);
//D2Common.0x6FD90ED0 (#10289)
D2COMMON_DLL_DECL void __stdcall INVENTORY_UpdateWeaponGUIDOnInsert(Inventory* pInventory, UnitAny* pItem);
//D2Common.0x6FD90F80 (#10290)
D2COMMON_DLL_DECL void __stdcall INVENTORY_UpdateWeaponGUIDOnRemoval(Inventory* pInventory, UnitAny* pItem);
//D2Common.0x6FD91050 (#10291)
D2COMMON_DLL_DECL int __stdcall INVENTORY_GetWieldType(UnitAny* pPlayer, Inventory* pInventory);
//D2Common.0x6FD91140 (#10292)
D2COMMON_DLL_DECL void __stdcall INVENTORY_SetOwnerId(Inventory* pInventory, D2UnitGUID nOwnerGuid);
//D2Common.0x6FD91160 (#10293)
D2COMMON_DLL_DECL int __stdcall INVENTORY_GetOwnerId(Inventory* pInventory);
//D2Common.0x6FD91190 (#10294)
D2COMMON_DLL_DECL void __stdcall INVENTORY_CreateCorpseForPlayer(Inventory* pInventory, int nUnitId, int a3, int a4);
//D2Common.0x6FD91210 (#10295)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_FreeCorpse(Inventory* pInventory, int nUnitId, int a3);
//D2Common.0x6FD91290 (#10296)
D2COMMON_DLL_DECL Corpse* __stdcall INVENTORY_GetFirstCorpse(Inventory* pInventory);
//D2Common.0x6FD912B0 (#10297)
D2COMMON_DLL_DECL int __stdcall INVENTORY_GetCorpseCount(Inventory* pInventory);
//D2Common.0x6FD912D0 (#10313)
D2COMMON_DLL_DECL Corpse* __stdcall INVENTORY_GetNextCorpse(Corpse* pCorpse);
//D2Common.0x6FDAFEA0 (#10314)
D2COMMON_DLL_DECL D2UnitGUID __stdcall INVENTORY_GetUnitGUIDFromCorpse(Corpse* pCorpse);
//D2Common.0x6FDB18D0 (#10315)
D2COMMON_DLL_DECL int __stdcall D2Common_10315(Corpse* pCorpse);
//D2Common.0x6FD912F0 (#10298)
D2COMMON_DLL_DECL void __stdcall INVENTORY_GetItemSaveGfxInfo(UnitAny* pPlayer, uint8_t* pComponents, uint8_t* pColor);
//D2Common.0x6FD915C0
void __fastcall INVENTORY_InitializeComponentArray();
//D2Common.0x6FD917B0
void __fastcall sub_6FD917B0(UnitAny* pUnit, uint8_t* a2, uint8_t* pColor, UnitAny* pItem);
//D2Common.0x6FD91B60 (#10299)
D2COMMON_DLL_DECL int __stdcall D2Common_10299(UnitAny* pUnit, int nBodyLoc, UnitAny* pItem, BOOL bDontCheckReqs);
//D2Common.0x6FD91D50
int __fastcall sub_6FD91D50(UnitAny* pPlayer, int a2, int nBodyLoc, UnitAny* a3, UnitAny* a4, UnitAny* pItem, int nUnused);
//D2Common.0x6FD91E80
BOOL __fastcall sub_6FD91E80(UnitAny* pUnit, UnitAny* pItem1, UnitAny* pItem2);
//D2Common.0x6FD92080 (#10304)
D2COMMON_DLL_DECL UnitAny* __stdcall INVENTORY_GetNextItem(UnitAny* pItem);
//Inlined at various places
UnitAny* __stdcall INVENTORY_GetUnknownItem(UnitAny* pItem);
//D2Common.0x6FD920C0 (#10305)
D2COMMON_DLL_DECL UnitAny* __stdcall INVENTORY_UnitIsItem(UnitAny* pItem);
//D2Common.0x6FD920E0 (#10306)
D2COMMON_DLL_DECL D2UnitGUID __stdcall INVENTORY_GetItemGUID(UnitAny* pItem);
//D2Common.0x6FD92100 (#10307)
D2COMMON_DLL_DECL int __stdcall INVENTORY_GetItemNodePage(UnitAny* pItem);
//D2Common.0x6FD92140 (#10310)
D2COMMON_DLL_DECL UnitAny* __stdcall INVENTORY_IsItemInInventory(Inventory* pInventory, UnitAny* pItem);
//D2Common.0x6FDAFEA0 (#10311)
D2COMMON_DLL_DECL InventoryNode* __stdcall INVENTORY_GetNextNode(InventoryNode* pNode);
//D2Common.0x6FD90AB0 (#10312)
D2COMMON_DLL_DECL D2UnitGUID __stdcall INVENTORY_GetItemGUIDFromNode(InventoryNode* pNode);
//D2Common.0x6FD92180 (#10300)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_RemoveAllItems(Inventory* pInventory);
//D2Common.0x6FD921D0 (#10302)
D2COMMON_DLL_DECL BOOL __stdcall INVENTORY_CanItemsBeTraded(void* pMemPool, UnitAny* pPlayer1, UnitAny* pPlayer2, TradeStates* pTradeState);
//D2Common.0x6FD923C0
BOOL __fastcall INVENTORY_CopyUnitItemsToTradeInventory(Inventory* pTradeInventory, UnitAny* pUnit);
//D2Common.0x6FD92490
BOOL __fastcall INVENTORY_CanItemBePlacedInInventory(UnitAny* pPlayer, UnitAny* pItem, Inventory* pInventory);
//D2Common.0x6FD925E0
int __fastcall UNITS_GetXPosition(UnitAny* pUnit);
//D2Common.0x6FD92610
int __fastcall UNITS_GetYPosition(UnitAny* pUnit);
