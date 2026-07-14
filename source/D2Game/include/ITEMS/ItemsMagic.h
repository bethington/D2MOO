#pragma once

#include <Units/Units.h>

#include <DataTbls/ItemsTbls.h>
#include "Items.h"

//D2Game.0x6FC52920
int16_t __fastcall ITEMS_RollMagicAffixes(UnitAny* pItem, int32_t bRequireSpawnableAffix, int32_t bForceAffixRoll, int32_t bAssignProperties, int32_t bPrefixes, int32_t nPreferredAffixIndex);
//D2Game.0x6FC52980
int16_t __fastcall ITEMS_RollMagicAffixesOld(UnitAny* pItem, int32_t bRequireSpawnableAffix, int32_t bForceAffixRoll, int32_t bAssignProperties, int32_t bPrefixes, int32_t nPreferredAffixIndex);
//D2Game.0x6FC52C00
int16_t __fastcall ITEMS_RollMagicAffixesNew(UnitAny* pItem, int32_t bRequireSpawnableAffix, int32_t bForceAffixRoll, int32_t bAssignProperties, int32_t bPrefixes, int32_t nPreferredAffixIndex, int32_t nAutoMagicGroup);
//D2Game.0x6FC53080
int16_t __fastcall ITEMS_RollMagicAffixes(UnitAny* pItem, int32_t bRequireSpawnableAffix, int32_t bForceAffixRoll, int32_t bAssignProperties, int32_t bPrefixes, int32_t nPreferredAffixIndex, int32_t nAutoMagicGroup);
//D2Game.0x6FC530E0
int16_t __fastcall ITEMS_RollTemperedItemAffix(UnitAny* pItem, int32_t bPrefix);
//D2Game.0x6FC53360
int32_t __fastcall D2GAME_RollRareItem_6FC53360(UnitAny* pItem, ItemDrop* pItemDrop);
//D2Game.0x6FC53610
int16_t __fastcall D2GAME_RollRareAffix_6FC53610(UnitAny* pItem, int32_t bPrefix);
//D2Game.0x6FC53760
int32_t __fastcall sub_6FC53760(UnitAny* pItem, ItemDrop* pItemDrop);
//D2Game.0x6FC53CD0
int32_t __fastcall sub_6FC53CD0(UnitAny* pItem, ItemDrop* pItemDrop);
//D2Game.0x6FC54240
int32_t __fastcall sub_6FC54240(UnitAny* pItem, int32_t bScroll);
//D2Game.0x6FC542C0
int32_t __fastcall sub_6FC542C0(UnitAny* pItem, ItemDrop* pItemDrop);
//D2Game.0x6FC544A0
int32_t __fastcall sub_6FC544A0(UnitAny* pItem);
//D2Game.0x6FC54690
int32_t __fastcall sub_6FC54690(UnitAny* pUnit, ItemDrop* pItemDrop);
//D2Game.0x6FC549F0
int32_t __fastcall sub_6FC549F0(UnitAny* pItem, ItemDrop* pItemDrop);
//D2Game.0x6FC54D00
int32_t __fastcall sub_6FC54D00(UnitAny* pItem, ItemDrop* pItemDrop);
//D2Game.0x6FC55060
void __fastcall sub_6FC55060(UnitAny* pUnit, int32_t nItemLevel, int32_t nClassFirstSkillId, int32_t a4);
//D2Game.0x6FC55270
int32_t __fastcall sub_6FC55270(UnitAny* pItem, ItemDrop* pItemDrop);
