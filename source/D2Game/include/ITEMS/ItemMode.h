#pragma once

#include <Units/Units.h>

#include <DataTbls/ItemsTbls.h>
#include "Items.h"
#include <GAME/Event.h>

#pragma pack(push, 1)
struct UnkItemMode
{
	int32_t nHitpoints;
	int32_t nMana;
	int32_t nStamina;
	int32_t nRightSkillId;
	int32_t nLeftSkillId;
	int32_t nRightSkillOwnerGUID;
	int32_t nLeftSkillOwnerGUID;
};

struct UnkItemMode2
{
	int32_t nBodyLoc;
	UnitAny* pItem;
	int32_t nSwitchBodyLoc;
	int32_t nSocketArg;
};

struct ItemModeArg
{
    int32_t bIsBow;
    int32_t bIsCrossBow;
    int32_t bIsBowQuiver;
    int32_t bIsCrossBowQuiver;
    int32_t bIsShield;
    int32_t bIsWeapon;
    int32_t bIsTwoHanded;
    int32_t bCanUse2Weapons;
    int32_t bIsThrowable;
    int32_t bIsRing;
};
#pragma pack(pop)


//D2Game.0x6FC41900
int32_t __fastcall D2GAME_Return1_6FC41900();
//D2Game.0x6FC41910
void __fastcall D2GAME_ITEMMODE_ServerStatlistCallback_6FC41910(Game* pGame, UnitAny* pOwner, UnitAny* pOther, int32_t nLayer_StatId, int32_t nPreviousValue, int32_t nNewValue);
//D2Game.0x6FC42050
void __fastcall sub_6FC42050(UnitAny* pItem, GameClient* pClient);
//D2Game.0x6FC420B0
void __fastcall sub_6FC420B0(UnitAny* pItem, GameClient* pClient);
//D2Game.0x6FC42120
int32_t __fastcall sub_6FC42120(UnitAny* pUnit, UnitAny* pItem, int32_t a3);
//D2Game.0x6FC42310
void __fastcall D2GAME_ITEMS_UpdateItemStatlist_6FC42310(Game* pGame, UnitAny* pItem, UnitAny* pUnit, int32_t a4);
//D2Game.0x6FC424E0
void __fastcall D2GAME_ITEMS_UpdateTransferredProperties_6FC424E0(Game* pGame, UnitAny* pItem, UnitAny* pTarget, int32_t bUpdateStatList, int32_t bUpdateVitals);
//D2Game.0x6FC425F0
int32_t __fastcall sub_6FC425F0(Game* pGame, UnitAny* pUnit, UnitAny* pItem);
//D2Game.0x6FC428F0
int32_t __fastcall sub_6FC428F0(UnitAny* pItem, ItemsTxt* pItemsTxtRecord, UnitAny* pPreviousItem);
//D2Game.0x6FC42B80
void __fastcall D2GAME_PickupItemEx_6FC42B80(Game* pGame, UnitAny* pUnit, int32_t nItemGUID, int32_t* a4);
//D2Game.0x6FC42DD0
void __fastcall D2GAME_PickupGold_6FC42DD0(Game* pGame, UnitAny* pUnit, UnitAny* pGoldPile);
//D2Game.0x6FC42F20
int32_t __fastcall sub_6FC42F20(UnitAny* pUnit, UnitAny* pItem, int32_t* pBodyLoc, int32_t bSkipRequirementCheck);
//D2Game.0x6FC43160
void __fastcall sub_6FC43160(UnitAny* pUnit, UnitAny* pItem, ItemModeArg* pItemModeArg);
//D2Game.0x6FC43280
int32_t __fastcall sub_6FC43280(ItemModeArg* pArg1, ItemModeArg* pArg2);
//D2Game.0x6FC43340
int32_t __fastcall D2GAME_PickupItem_6FC43340(Game* pGame, UnitAny* pUnit, int32_t nItemGUID, int32_t* a4);
//D2Game.0x6FC437F0
int32_t __fastcall sub_6FC437F0(Game* pGame, UnitAny* pUnit, UnitAny* pItem);
//D2Game.0x6FC43AF0
void __fastcall sub_6FC43AF0(UnitAny* pUnit, UnitAny* pItem, int32_t nQuantityBonus);
//D2Game.0x6FC43BF0
int32_t __fastcall sub_6FC43BF0(Game* pGame, UnitAny* pUnit, UnitAny* pItem);
//D2Game.0x6FC43E60
int32_t __fastcall sub_6FC43E60(Game* pGame, UnitAny* pUnit, UnitAny* pItem, int32_t bRemove, int32_t nPage, const char* szFile, int32_t nLine);
//D2Game.0x6FC44030
void __fastcall sub_6FC44030(Game* pGame, UnitAny* pUnit, int32_t nItemGUID);
//D2Game.0x6FC44410
int32_t __fastcall D2GAME_PlaceItem_6FC44410(const char* szFile, int32_t nLine, Game* pGame, UnitAny* pPlayer, int32_t nUnitId, int32_t nX, int32_t nY, int32_t a8, int32_t a9, Inventory* pInventory);
//D2Game.0x6FC446B0
int32_t __fastcall sub_6FC446B0(Game* pGame, UnitAny* pUnit, int32_t nItemGUID, int32_t* a4, int32_t a5, int32_t bSetQuantityFlag, Inventory* pInventoryArg, int32_t a8);
//D2Game.0x6FC44A90
void __fastcall D2GAME_ITEMS_UpdateInventoryItems_6FC44A90(Game* pGame, UnitAny* pUnit, int32_t bRefresh);
//D2Game.0x6FC44F00
void __fastcall sub_6FC44F00(UnitAny* pUnit, UnkItemMode* a2);
//D2Game.0x6FC44FB0
void __fastcall sub_6FC44FB0(UnitAny* pUnit, UnkItemMode* a2);
//D2Game.0x6FC45050
void __fastcall D2GAME_ITEMS_UpdateInventoryItems_6FC45050(Game* pGame, UnitAny* pUnit, int32_t a3, int32_t bRefresh);
//D2Game.0x6FC45060
int32_t __fastcall sub_6FC45060(Game* pGame, UnitAny* pPlayer, int32_t nItemGUID, uint8_t nBodyLoc, int32_t bSkipRequirementCheck, int32_t* a6);
//D2Game.0x6FC45300
void __fastcall sub_6FC45300(UnitAny* pUnit);
//D2Game.0x6FC45550
int32_t __fastcall sub_6FC45550(Game* pGame, UnitAny* pUnit, int32_t nItemGUID, uint8_t nBodyLoc, int32_t bSkipRequirementCheck, int32_t* a6);
//D2Game.0x6FC45930
void __fastcall sub_6FC45930(Game* pGame, UnitAny* pUnit, UnitAny* pItem);
//D2Game.0x6FC45B30
int32_t __fastcall sub_6FC45B30(Game* pGame, UnitAny* pUnit, uint8_t nBodyLoc, int32_t bSetQuantityFlag, int32_t* a5);
//D2Game.0x6FC45E60
int32_t __fastcall sub_6FC45E60(Game* pGame, UnitAny* pUnit, int32_t nItemGUID, uint8_t nBodyLoc, int32_t bSkipRequirementCheck, int32_t* a6);
//D2Game.0x6FC46270
int32_t __fastcall sub_6FC46270(Game* pGame, UnitAny* pUnit, int32_t nItemGUID, uint8_t nBodyLoc, int32_t* a5);
//D2Game.0x6FC46840
int32_t __fastcall sub_6FC46840(Game* pGame, UnitAny* pUnit, int32_t* a3);
//D2Game.0x6FC46D40
int32_t __fastcall sub_6FC46D40(Game* pGame, UnitAny* pUnit, int32_t nCursorItemGUID, int32_t nGridItemGUID, int32_t nX, int32_t nY, int32_t* a7);
//D2Game.0x6FC471F0
void __fastcall D2GAME_RemoveItem_6FC471F0(Game* pGame, UnitAny* pUnit, UnitAny* pItem, int32_t bSendPacket);
//D2Game.0x6FC47380
void __fastcall sub_6FC47380(Game* pGame, UnitAny* pUnit, UnitAny* pItem);
//D2Game.0x6FC47470
int32_t __fastcall sub_6FC47470(Game* pGame, UnitAny* pUnit, int32_t nTargetGUID, int32_t nUseItemGUID, int32_t* a5);
//D2Game.0x6FC47C90
void __fastcall sub_6FC47C90(UnitAny* pUnit, int32_t nSkillId);
//D2Game.0x6FC47D30
int32_t __fastcall sub_6FC47D30(Game* pGame, UnitAny* pUnit, int32_t nItemGUID, int32_t nX, int32_t nY, int32_t* a6);
//D2Game.0x6FC484E0
int32_t __fastcall sub_6FC484E0(Game* pGame, UnitAny* pUnit, int32_t nItemGUID1, int32_t nItemGUID2, int32_t* a5);
//D2Game.0x6FC48930
int32_t __stdcall D2GAME_Return0_6FC48930(int32_t a1, int32_t* a2);
//D2Game.0x6FC48940
int32_t __fastcall sub_6FC48940(Game* pGame, UnitAny* pUnit, int32_t nItemGUID, int32_t nBeltSlot, int32_t bFindFreeSlot, int32_t* a6);
//D2Game.0x6FC48B40
int32_t __fastcall sub_6FC48B40(Game* pGame, UnitAny* pUnit, int32_t nItemGUID, int32_t* a4);
//D2Game.0x6FC48D50
int32_t __fastcall sub_6FC48D50(Game* pGame, UnitAny* pUnit, int32_t nCursorItemGUID, int32_t nBeltItemGUID, int32_t* a5);
//D2Game.0x6FC49090
void __fastcall sub_6FC49090(Game* pGame, UnitAny* pUnit, UnitAny* pItem);
//D2Game.0x6FC49140
void __fastcall sub_6FC49140(Game* pGame, UnitAny* pUnit, int8_t nX);
//D2Game.0x6FC49220
int32_t __fastcall sub_6FC49220(Game* pGame, UnitAny* pUnit, int32_t nItemGUID, int32_t nX, int32_t nY, int32_t* a6, int32_t bUseOnMerc);
//D2Game.0x6FC49670
void __fastcall D2GAME_ITEMS_Identify_6FC49670(Game* pGame, UnitAny* pPlayer, UnitAny* pItem);
//D2Game.0x6FC49760
int32_t __fastcall D2GAME_RemoveItemIfOnCursor_6FC49760(Game* pGame, UnitAny* pPlayer, UnitAny* pItem);
//D2Game.0x6FC497E0
int32_t __fastcall D2GAME_ITEMSOCKET_PlaceItem_6FC497E0(Game* pGame, UnitAny* pUnit, int32_t nSocketFillerGUID, int32_t nItemGUID, int32_t* bPlaced, int32_t bRefresh, int32_t bResetCursorItem, int32_t bCheckMode);
//D2Game.0x6FC49AE0
int32_t __fastcall sub_6FC49AE0(Game* pGame, UnitAny* pUnit, int32_t nScrollGUID, int32_t nBookGUID, int32_t* a5);
//D2Game.0x6FC49DC0
int32_t __fastcall sub_6FC49DC0(Game* pGame, UnitAny* pUnit, int32_t nItemGUID, int32_t nCubeGUID, int32_t* a5);
//D2Game.0x6FC49F80
int32_t __fastcall sub_6FC49F80(Game* pGame, UnitAny* pUnit, int32_t nItemGUID, uint8_t nBodyLoc);
//D2Game.0x6FC4A2E0
void __fastcall sub_6FC4A2E0(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC4A350
int32_t __fastcall sub_6FC4A350(Game* pGame, UnitAny* pItem, int32_t nLengthStatId, int32_t nValueStatId, int32_t nMaxValue);
//D2Game.0x6FC4A450
void __fastcall sub_6FC4A450(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC4A460
void __fastcall D2GAME_Items_EventsHandler_6FC4A460(Game* pGame, UnitAny* pUnit, EventTypes nEventType);
//D2Game.0x6FC4A4B0
int32_t __fastcall D2GAME_DoKeyCheck_6FC4A4B0(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC4A660
int32_t __fastcall D2GAME_Transmogrify_6FC4A660(Game* pGame, UnitAny* pUnit, UnitAny* pItem);
//D2Game.0x6FC4A9B0
int32_t __fastcall sub_6FC4A9B0(Game* pGame, UnitAny* pUnit, UnitAny* pItem, uint8_t nBodyLoc);
//D2Game.0x6FC4AB10
int32_t __fastcall sub_6FC4AB10(UnitAny* pPlayer, UnitAny* pItem, UnitAny** ppItem1, UnitAny** ppItem2, int32_t* pBodyLoc);
//D2Game.0x6FC4AD80
int32_t __fastcall sub_6FC4AD80(Game* pGame, UnitAny* pPlayer, UnitAny* pCorpse);
//D2Game.0x6FC4B240
void __fastcall D2GAME_UpdatePlayerVitals_6FC4B240(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC4B2D0
int32_t __fastcall D2GAME_CheckHasFilledSockets_6FC4B2D0(UnitAny* pItem, const char* szFile, int32_t nLine);
//D2Game.0x6FC4B310
int32_t __fastcall sub_6FC4B310(Game* pGame, UnitAny* pUnit, UnitAny* pItem, UnitAny* pSocketable, int32_t a5);
//D2Game.0x6FC4B430
UnitAny* __fastcall sub_6FC4B430(Game* pGame, UnitAny* pUnit, UnitAny* pItem, int32_t* a4);
//D2Game.0x6FC4B520
void __fastcall sub_6FC4B520(Game* pGame, UnitAny* pUnit, UnitAny* pItem, int32_t nValue);
//D2Game.0x6FC4B580
void __fastcall sub_6FC4B580(Game* pGame, UnitAny* pUnit, UnitAny* pItem);
//D2Game.0x6FC4B630
void __fastcall D2GAME_RepairBrokenItem_6FC4B630(Game* pGame, UnitAny* pPlayer, UnitAny* pItem);
//D2Game.0x6FC4B740
void __fastcall sub_6FC4B740(UnitAny* pUnit, UnitAny* pItem);
//D2Game.0x6FC4B830
void __fastcall sub_6FC4B830(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC4B9D0
int32_t __fastcall sub_6FC4B9D0(Game* pGame, UnitAny* pUnit, UnitAny* pItem);
//D2Game.0x6FC4BA50
void __fastcall sub_6FC4BA50(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC4BB90
void __fastcall D2GAME_UpdatePlayerItems_6FC4BB90(Game* pGame, UnitAny* pUnit, int32_t a3);
//D2Game.0x6FC4BBB0
void __fastcall sub_6FC4BBB0(Game* pGame, UnitAny* pItem, UnitAny* pPlayer);
//D2Game.0x6FC4BC00
void __fastcall D2GAME_ITEMS_UpdateStatList_6FC4BC00(Game* pGame, UnitAny* pItem, UnitAny* pUnit);
//D2Game.0x6FC4BC10
void __fastcall D2GAME_ITEMMODE_Unk_6FC4BC10(Game* pGame, UnitAny* pUnit, UnitAny* pItem);
//D2Game.0x6FC4BD50
int32_t __fastcall D2GAME_RechargeItem_6FC4BD50(Game* pGame, UnitAny* pUnit, UnitAny* pItem);
//D2Game.0x6FC4BE80
void __fastcall sub_6FC4BE80(UnitAny* pUnit, Game* pGame, int32_t nUnitGUID, Room1* pRoom);
//D2Game.0x6FC4BF00
Room1* __fastcall D2GAME_GetFreeSpaceEx_6FC4BF00(Room1* pRoom, Coord* pCoords, Coord* pReturnCoords, int32_t nUnitSize);
//D2Game.0x6FC4BFF0
int32_t __fastcall sub_6FC4BFF0(Game* pGame, Seed* pSeed, int32_t nLevelId, int32_t nItemType, int32_t a5);
//D2Game.0x6FC4C1E0
int32_t __fastcall sub_6FC4C1E0(Game* pGame, Seed* pSeed, int32_t nLevelId, int32_t nItemType, int32_t a5);
//D2Game.0x6FC4C3D0
int32_t __fastcall sub_6FC4C3D0(Game* pGame, Seed* pSeed, int32_t nLevelId, int32_t nItemType, int32_t a5, int32_t a6);
//D2Game.0x6FC4C5F0
int32_t __fastcall sub_6FC4C5F0(Game* pGame, UnitAny** ppUnit, ItemDrop* pItemDrop);
