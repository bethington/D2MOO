#pragma once

#include <Units/Units.h>
#include "AiStates.h"

#pragma pack(push, 1)
struct BaalThroneAiCallbackArg
{
	UnitAny* pTarget;
	int32_t nDistance;
	int32_t unk0x08;
	int32_t nMaxDistance;
};

struct UnkAiCallbackArg
{
	UnitAny* pTarget;
	int32_t nDistance;
	int32_t unk0x08;
	int32_t nMaxDistance;
	int32_t unk0x10;
	UnitAny* pAlternativeTarget;
	int32_t nAlternativeDistance;
};

struct UnkAiCallbackArg2
{
	UnitAny* pTarget;
	int32_t nDistance;
	UnitAny* pAlternativeTarget;
	int32_t nAlternativeDistance;
};

struct AiCallback7Arg
{
	UnitAny* pTarget;
	int32_t nDistance;
	UnitAny* unk0x08;
};

struct AiCallback11Arg
{
	UnitAny* pTarget;
	int32_t nDistance;
	UnitAny* unk0x08;
};

struct DoorObjectAiCallbackArg
{
	UnitAny* pDoor;
	int32_t nDistance;
};

struct FallenShamanAiCallbackArg
{
	UnitAny* pTarget;
	int32_t nCounter;
	int32_t nDistance;
	int32_t nMaxDistance;
};

struct VileMotherAiCallbackArg
{
	int32_t nLastInClass;
	int32_t nMaxDistance;
	int32_t nCounter;
};

struct UnkAiStrc5
{
	int32_t nMaxDistance;
};
#pragma pack(pop)


//Inlined in D2Game.0x6FCF1210 and D2Game.0x6FCF1310
int32_t __fastcall AIUTIL_GetDistanceToCoordinatesWithSize(UnitAny* pUnit, int32_t nX, int32_t nY, int32_t nSize);
//D2Game.0x6FCF1210
int32_t __fastcall AIUTIL_GetDistanceToCoordinates_FullUnitSize(UnitAny* pTarget, UnitAny* pSource);
//D2Game.0x6FCF1310
int32_t __fastcall AIUTIL_GetDistanceToCoordinates_HalfUnitSize(UnitAny* pUnit, int32_t nX, int32_t nY);
//D2Game.0x6FCF13B0
int32_t __fastcall AIUTIL_GetDistanceToCoordinates_NoUnitSize(UnitAny* pUnit, int32_t nX, int32_t nY);
//D2Game.0x6FCF1440
uint32_t __fastcall AIUTIL_GetDistanceToCoordinates(UnitAny* pUnit, int32_t nX, int32_t nY);
//D2Game.0x6FCF14D0
int32_t __fastcall sub_6FCF14D0(UnitAny* pUnit1, UnitAny* pUnit2);
//D2Game.0x6FCF16D0
UnitAny* __fastcall sub_6FCF16D0(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCF1740
UnitAny* __fastcall sub_6FCF1740(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCF1780
UnitAny* __fastcall sub_6FCF1780(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCF1980
UnitAny* __fastcall sub_6FCF1980(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCF1A50
int32_t __fastcall sub_6FCF1A50(Game* pGame, UnitAny* pUnit, UnitAny* pTarget);
//D2Game.0x6FCF1B30
UnitAny* __fastcall sub_6FCF1B30(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCF1BD0
UnitAny* __fastcall sub_6FCF1BD0(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCF1CB0
UnitAny* __fastcall sub_6FCF1CB0(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCF1DC0
UnitAny* __fastcall sub_6FCF1DC0(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCF1E30
UnitAny* __fastcall sub_6FCF1E30(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCF1E80
UnitAny* __fastcall sub_6FCF1E80(Game* pGame, UnitAny* pUnit, void* a3, UnitAny* (__fastcall* a4)(Game*, UnitAny*, UnitAny*, void*), int32_t nCallbackId);
//D2Game.0x6FCF20E0
void __fastcall sub_6FCF20E0(UnitAny* pUnit, void* pArg, void* ppUnit);
//D2Game.0x6FCF2110
UnitAny* __fastcall sub_6FCF2110(Game* pGame, UnitAny* pUnit, AiControl* pAiControl, int32_t* pDistance, int32_t* pCombat);
//D2Game.0x6FCF27B0
int32_t __fastcall sub_6FCF27B0(UnitAny* pUnit, UnitAny** ppTarget, int32_t* pTargetDistance, UnitAny* a4, int32_t a5);
//D2Game.0x6FCF2920
int32_t __fastcall sub_6FCF2920(Game* pGame, UnitAny* pUnit, int32_t a3, int32_t a4, UnitAny** ppUnit, int32_t* pDistance);
//D2Game.0x6FCF2B80
UnitAny* __fastcall AIUTIL_FindTargetInAdjacentRooms(Game* pGame, UnitAny* pUnit, void* pArg, UnitAny* (__fastcall* pfCallback)(Game*, UnitAny*, UnitAny*, void*));
//D2Game.0x6FCF2C00
UnitAny* __fastcall AIUTIL_FindTargetInAdjacentActiveRooms(Game* pGame, UnitAny* pUnit, void* pArg, UnitAny* (__fastcall* pfCallback)(Game*, UnitAny*, UnitAny*, void*));
//D2Game.0x6FCF2CC0
UnitAny* __fastcall sub_6FCF2CC0(Game* pGame, UnitAny* pUnit, int32_t* pDistance, int32_t* pInMeleeRange);
//D2Game.0x6FCF2DF0
int32_t __fastcall AIUTIL_CheckAiControlFlag(AiControl* pAiControl, uint16_t nFlag);
//D2Game.0x6FCF2E00
void __fastcall AIUTIL_ToggleAiControlFlag(AiControl* pAiControl, uint16_t nFlag, int32_t bSet);
//D2Game.0x6FCF2E20
void __fastcall AIUTIL_ToggleAiControlFlag0x20(UnitAny* pUnit, int32_t bSet);
//D2Game.0x6FCF2E70
int32_t __fastcall sub_6FCF2E70(UnitAny* pUnit);
//D2Game.0x6FCF2E90
int32_t __fastcall AIUTIL_CheckIfMonsterUsesSkill(UnitAny* pUnit, int32_t nSkillId);
//D2Game.0x6FCF2EF0
void __fastcall AIUTIL_SetOwnerGUIDAndType(UnitAny* pUnit, UnitAny* pOwner);
//D2Game.0x6FCF2F30
void __fastcall AIUTIL_ApplyTerrorCurseState(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, int32_t nSkillId, int32_t nParam1, int32_t nDuration);
//D2Game.0x6FCF3000
BOOL __fastcall AIUTIL_CanUnitSwitchAi(UnitAny* pUnit, AiSpecialState nAiSpecialState);
