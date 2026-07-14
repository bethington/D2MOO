#pragma once

#include "D2CommonDefinitions.h"
#include <Units/Missile.h>
#include <DataTbls/ItemsTbls.h>

struct StatList;

#pragma pack(1)


typedef BOOL(__fastcall* PROPERTYASSIGN)(int32_t, UnitAny*, UnitAny*, const Property*, int32_t, int32_t, int32_t, int32_t, UnitAny*);

typedef int32_t(__fastcall* PROPERTYASSIGNFN)(int32_t, UnitAny*, UnitAny*, const Property*, int32_t, int16_t, int32_t, int32_t, int32_t, int32_t, UnitAny*);

struct PropertyAssign
{
	PROPERTYASSIGN pfAssign;			//0x00
	int32_t nStatId;					//0x04
};


struct ItemCalc
{
	UnitAny* pUnit;					//0x00
	UnitAny* pItem;					//0x04
};

#pragma pack()

//D2Common.0x6FD92640 (#10844)
D2COMMON_DLL_DECL void __stdcall D2Common_10844_ITEMMODS_First(int nDataBits, int* pLayer, int* pValue);
//D2Common.0x6FD92670 (#10846)
D2COMMON_DLL_DECL void __stdcall D2Common_10846(int nDataBits, int* a2, int* a3, int* a4, int* a5);
//D2Common.0x6FD926C0 (#11293)
D2COMMON_DLL_DECL BOOL __stdcall ITEMMODS_GetItemCharges(UnitAny* pItem, int nSkillId, int nSkillLevel, int* pValue, StatList** ppStatList);
//D2Common.0x6FD927D0 (#10847)
D2COMMON_DLL_DECL BOOL __stdcall ITEMMODS_UpdateItemWithSkillCharges(UnitAny* pItem, int nSkillId, int nSkillLevel, int a4);
//D2Common.0x6FD928D0 (#10843)
D2COMMON_DLL_DECL int __stdcall ITEMMODS_GetByTimeAdjustment(int nAmount, int nPeriodOfDay, int nBaseTime, int* pItemModPeriodOfDay, int* pItemModMin, int* pItemModMax);
//D2Common.0x6FD929A0 (#10849)
D2COMMON_DLL_DECL int __stdcall D2Common_10849(int a1, int a2);
//D2Common.0x6FD929B0 (#10845)
D2COMMON_DLL_DECL void __stdcall D2Common_10845(int nDataBits, int* a2, int* a3, int* a4);
//D2Common.0x6FD929E0 (#10850)
D2COMMON_DLL_DECL int __stdcall D2Common_10850(int a1, int a2, int a3);
//D2Common.0x6FD92A00 (#10848)
D2COMMON_DLL_DECL void __stdcall D2Common_10848(int nDataBits, int* pClass, int* pTab, int* pLevel);
//D2Common.0x6FD92A60 (#10851)
D2COMMON_DLL_DECL int __stdcall D2Common_10851(int a1, int a2, int a3);
//D2Common.0x6FD92A80
BOOL __fastcall sub_6FD92A80(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD92C40
StatList* __fastcall ITEMMODS_GetOrCreateStatList(UnitAny* pUnit, UnitAny* pItem, int nState, int fFilter);
//D2Common.0x6FD92CF0
void __fastcall sub_6FD92CF0(UnitAny* pItem, int nStatId);
//D2Common.0x6FD92E80
BOOL __fastcall sub_6FD92E80(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD92EB0
BOOL __fastcall sub_6FD92EB0(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int a7, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD93170
BOOL __fastcall sub_6FD93170(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD931C0
BOOL __fastcall sub_6FD931C0(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD93200
BOOL __fastcall sub_6FD93200(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD93230
BOOL __fastcall sub_6FD93230(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD93410
BOOL __fastcall sub_6FD93410(int nType, UnitAny* pUnit, UnitAny* pItem, int a4, int nStatId, int nApplyType, int a7, int nState, int fStatList, UnitAny* a10);
//D2Common.0x6FD935B0
BOOL __fastcall sub_6FD935B0(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD93790
BOOL __fastcall sub_6FD93790(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD93A20
BOOL __fastcall sub_6FD93A20(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD93CB0
BOOL __fastcall sub_6FD93CB0(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD94060
void __fastcall sub_6FD94060(int nStatId, int* pValue);
//D2Common.0x6FD94160
BOOL __fastcall sub_6FD94160(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD94190
BOOL __fastcall sub_6FD94190(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD943C0
BOOL __fastcall sub_6FD943C0(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD944E0
BOOL __fastcall sub_6FD944E0(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD94AB0
BOOL __fastcall sub_6FD94AB0(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD94E80
BOOL __fastcall sub_6FD94E80(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD94F70
BOOL __fastcall sub_6FD94F70(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD95050
BOOL __fastcall sub_6FD95050(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD95200
BOOL __fastcall sub_6FD95200(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD95210
BOOL __fastcall sub_6FD95210(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nStatId, int nApplyType, int nState, int fStatList, UnitAny* a9);
//D2Common.0x6FD95430 (#10855)
D2COMMON_DLL_DECL void __stdcall ITEMMODS_AssignProperty(int nType, UnitAny* a2, UnitAny* pItem, const void* pMods, int nPropSet, int nApplyType);
//D2Common.0x6FD95810
void __fastcall sub_6FD95810(int nType, UnitAny* pUnit, UnitAny* pItem, const void* pMods, int nIndex, int nPropSet, int nApplyType, const Property* pProperty, int nState, int fStatlist, UnitAny* a11);
//D2Common.0x6FD958D0 (#10865)
D2COMMON_DLL_DECL void __stdcall ITEMMODS_ApplyEthereality(UnitAny* pItem);
//D2Common.0x6FD959F0 (#10867)
D2COMMON_DLL_DECL BOOL __stdcall ITEMMODS_UpdateRuneword(UnitAny* pUnit, UnitAny* pItem, int nUnused);
//D2Common.0x6FD95A70
void __fastcall ITEMMODS_UpdateFullSetBoni(UnitAny* pUnit, UnitAny* pItem, int nState);
//D2Common.0x6FD95BE0 (#10859)
D2COMMON_DLL_DECL BOOL __stdcall ITEMMODS_CanItemHaveMagicAffix(UnitAny* pItem, const MagicAffixTxt* pMagicAffixTxtRecord);
//D2Common.0x6FD95CC0 (#10860)
D2COMMON_DLL_DECL BOOL __stdcall ITEMMODS_CanItemHaveRareAffix(UnitAny* pItem, RareAffixTxt* pRareAffixTxtRecord);
//D2Common.0x6FD95D60 (#10861)
D2COMMON_DLL_DECL BOOL __stdcall ITEMMODS_CanItemBeHighQuality(UnitAny* pItem, QualityItemsTxt* pQualityItemsTxtRecord);
//D2Common.0x6FD95E90 (#10862)
D2COMMON_DLL_DECL void __stdcall ITEMMODS_SetRandomElixirFileIndex(UnitAny* pItem);
//D2Common.0x6FD95F90 (#10868)
D2COMMON_DLL_DECL void __stdcall ITEMMODS_AddCraftPropertyList(UnitAny* pItem, Property* pProperty, int nUnused);
//D2Common.0x6FD95FC0
int __fastcall ITEMMODS_PropertyFunc01(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD96110
int __fastcall ITEMMODS_AddPropertyToItemStatList(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* pUnused);
//D2Common.0x6FD96210
int __fastcall ITEMMODS_PropertyFunc02(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD96350
int __fastcall ITEMMODS_PropertyFunc03(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD964A0
int __fastcall ITEMMODS_PropertyFunc04(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD965F0
int __fastcall ITEMMODS_PropertyFunc05(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD96880
int __fastcall ITEMMODS_PropertyFunc06(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD96B00
int __fastcall ITEMMODS_PropertyFunc07(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD96DA0
int __fastcall ITEMMODS_PropertyFunc08(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD96EE0
int __fastcall ITEMMODS_PropertyFunc09(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD97040
int __fastcall ITEMMODS_PropertyFunc24(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD97180
int __fastcall ITEMMODS_PropertyFunc10(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD972E0
int __fastcall ITEMMODS_PropertyFunc11(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD97430
int __fastcall ITEMMODS_PropertyFunc14(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD975F0
int __fastcall ITEMMODS_PropertyFunc19(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD97830
int __fastcall ITEMMODS_PropertyFunc18(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD97920
int __fastcall ITEMMODS_PropertyFunc15(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD979A0
int __fastcall ITEMMODS_PropertyFunc16(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD97A20
int __fastcall ITEMMODS_PropertyFunc17(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD97BA0
int __fastcall ITEMMODS_PropertyFunc20(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD97C20
int __fastcall ITEMMODS_PropertyFunc21(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD97D50
int __fastcall ITEMMODS_PropertyFunc22(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD97E80
int __fastcall ITEMMODS_PropertyFunc12(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD97FB0
int __fastcall ITEMMODS_PropertyFunc13(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD98120
int __fastcall ITEMMODS_PropertyFunc23(int nType, UnitAny* pUnit, UnitAny* pItem, const Property* pProperty, int nSet, short nStatId, int nLayer, int nValue, int nState, int fStatList, UnitAny* a11);
//D2Common.0x6FD98160 (#11292)
D2COMMON_DLL_DECL void __stdcall D2COMMON_11292_ItemAssignProperty(int nType, UnitAny* pUnit, UnitAny* pItem, const void* pMods, int nIndex, int nPropSet, const Property* pProperty, int nState, int fStatlist, UnitAny* a10);
//D2Common.0x6FD98220
int __fastcall sub_6FD98220(int nMin, int nMax, int nUnused, void* pUserData);
//D2Common.0x6FD982A0
int __fastcall sub_6FD982A0(int nStatId, int a2, int nUnused, void* pUserData);
//D2Common.0x6FD98300 (#11300)
D2COMMON_DLL_DECL int __stdcall ITEMMODS_EvaluateItemFormula(UnitAny* pUnit, UnitAny* pItem, unsigned int nCalc);
