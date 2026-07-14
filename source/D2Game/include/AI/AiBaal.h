#pragma once

#include <Units/Units.h>
#include "AiGeneral.h"


//D2Game.0x6FCCD450
void __fastcall AIBAAL_CountLivingMinions(UnitAny* pUnit, void* ppUnitArg, void* pCounterArg);
//D2Game.0x6FCCD470
void __fastcall D2GAME_AI_Unk135_140_6FCCD470(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCCD520
void __fastcall AITHINK_Fn135_BaalCrab(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCCD630
UnitAny* __fastcall AIBAAL_GetTarget(Game* pGame, UnitAny* pUnit, int32_t* pMax, int32_t* pCount, void* pArgs, int32_t(__fastcall* pfCull)(UnitAny*, UnitAny*));
//D2Game.0x6FCCD8A0
int32_t __fastcall AIBAAL_GetTargetScore(UnitAny* pUnit, UnitAny* pTarget, void* pArgs);
//D2Game.0x6FCCDBB0
int32_t __fastcall AIBAAL_CullPotentialTargets(UnitAny* pBaal, UnitAny* pTarget);
//D2Game.0x6FCCDC80
int32_t __fastcall AIBAAL_RollRandomAiParam(Game* pGame, UnitAny* pUnit, AiControl* pAiControl, UnitAny* pTarget, int32_t nMax, int32_t nCount, AiCmd* pAiCmd);
//D2Game.0x6FCCE040
int32_t __fastcall AI_CheckSpecialSkillsOnPrimeEvil(UnitAny* pUnit);
//D2Game.0x6FCCE100
int32_t __fastcall AI_GetRandomArrayIndex(int32_t* pArray, int32_t nArraySize, UnitAny* pUnit, int32_t nDefaultValue);
//D2Game.0x6FCCE1A0
int32_t __fastcall AIBAAL_RollRandomAiParamForNonCollidingUnit(Game* pGame, AiControl* pAiControl, UnitAny* pUnit, UnitAny* pTarget, int32_t nCount, int32_t bInMediumRange, int32_t bInFarRange, int32_t bInCloseRange, int32_t nMax);
//D2Game.0x6FCCE450
void __fastcall AIBAAL_MainSkillHandler(Game* pGame, UnitAny* pUnit, AiControl* pAiControl, UnitAny* pTarget, int32_t nParam, AiCmd* pAiCmd);
//D2Game.0x6FCCEB70
void __fastcall AITHINK_Fn140_BaalCrabClone(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
