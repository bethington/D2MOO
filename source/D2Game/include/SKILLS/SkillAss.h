#pragma once

#include <D2StatList.h>
#include <Units/Units.h>
#include <Units/UnitFinds.h>
#include <UNIT/SUnitDmg.h>


struct AuraCallback;


//D2Game.0x6FCF5090
int32_t __fastcall SKILLS_SrvSt22_PsychicHammer(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF50E0
int32_t __fastcall SKILLS_SrvDo033_PsychicHammer(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF52C0
int32_t __fastcall SKILLS_SrvSt23_AssasinChargeStrikes(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF52E0
void __fastcall SKILLS_StatRemoveCallback_ProgressiveStrike(UnitAny* pUnit, int32_t nState, StatList* pStatList);
//D2Game.0x6FCF52F0
int32_t __fastcall SKILLS_SrvDo034_TigerStrike_CobraStrike_RoyalStrike(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF55B0
int32_t __fastcall SKILLS_SrvDo035_FistsOfFire_ClawsOfThunder_BladesOfIce(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF5680
void __fastcall sub_6FCF5680(UnitAny* pUnit, Damage* pDamage);
//D2Game.0x6FCF5870
void __fastcall sub_6FCF5870(UnitAny* pUnit, Damage* pDamage);
//D2Game.0x6FCF5B90
SkillsTxt* __fastcall SKILLS_GetSkillsTxtRecord(int32_t nSkillId);
//D2Game.0x6FCF5BC0
void __fastcall sub_6FCF5BC0(UnitAny* pUnit, StatList* pStatList, Damage* pDamage);
//D2Game.0x6FCF5DE0
int32_t __fastcall sub_6FCF5DE0(AuraCallback* pAuraCallback, UnitAny* pUnit);
//D2Game.0x6FCF5E20
int32_t __fastcall SKILLS_GetProgressiveSkillMissileId(UnitAny* pUnit, int32_t nSkillId);
//D2Game.0x6FCF5EE0
int32_t __fastcall SKILLS_EvaluateProgressiveSkillCalc(UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF6000
int32_t __fastcall SKILLS_SrvDo038_FistsOfFire_BladesOfIce_ProgressiveFn2(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF6140
int32_t __fastcall SKILLS_SrvDo039_FistsOfFire_BladesOfIce_ProgressiveFn3(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF6420
int32_t __fastcall SKILLS_SrvDo036_ClawsOfThunder_ProgressiveFn2(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF6500
int32_t __fastcall SKILLS_SrvDo037_ClawsOfThunder_ProgressiveFn3(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF6600
void __fastcall sub_6FCF6600(Game* pGame, UnitAny* pUnit, int32_t nLowSeed, int32_t nMissileId, int32_t nX, int32_t nY, int32_t nSkillId, int32_t nSkillLevel, int32_t nStep);
//D2Game.0x6FCF6C10
void __fastcall sub_6FCF6C10(UnitAny* pMissile, int32_t nLowSeed);
//D2Game.0x6FCF6C70
int32_t __fastcall SKILLS_SrvDo143_FistsOfFire_RoyalStrike_ProgressiveFn(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF6D70
void __fastcall sub_6FCF6D70(Game* pGame, UnitAny* pUnit, int32_t nLowSeed, int32_t nMissileId, int32_t nX, int32_t nY, int32_t nSkillId, int32_t nSkillLevel, int32_t nStep);
//D2Game.0x6FCF7390
void __fastcall sub_6FCF7390(UnitAny* pMissile, int32_t nInitSeed);
//D2Game.0x6FCF7590
int32_t __fastcall SKILLS_SrvDo040_RoyalStrike_ProgressiveFn1(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF7610
int32_t __fastcall SKILLS_SrvDo041_RoyalStrike_ProgressiveFn3(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF77E0
void __fastcall sub_6FCF77E0(Game* pGame, UnitAny* pUnit, Damage* pDamage);
//D2Game.0x6FCF7AD0
int32_t __fastcall SKILLS_SrvSt24_DragonTalon(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF7BC0
void __fastcall sub_6FCF7BC0(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, SkillsTxt* pSkillsTxtRecordArg, int32_t nSkillId, int32_t nSkillLevel, int32_t bKnockback);;
//D2Game.0x6FCF7CE0
void __fastcall sub_6FCF7CE0(Game* pGame, Damage* pDamage, UnitAny* pUnit, UnitAny* pTarget, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF8110
int32_t __fastcall SKILLS_SrvDo042_DragonTalon(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF8330
int32_t __fastcall sub_6FCF8330(Game* pGame, UnitAny* pMissile, UnitAny* pUnit, int32_t nMissileId, int32_t nSubMissiles, int32_t nRange, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF8550
int32_t __fastcall SKILLS_SrvDo043_ShockField(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF8610
UnitAny* __fastcall sub_6FCF8610(Game* pGame, UnitAny* pUnit, int32_t nX, int32_t nY, SkillsTxt* pSkillsTxtRecord, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF88E0
int32_t __fastcall SKILLS_SrvDo044_BladeSentinel(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF8B00
int32_t __fastcall SKILLS_SrvDo045_Sentry(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF8BA0
int32_t __fastcall SKILLS_SrvDo046_DragonClaw(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF8C70
void __fastcall sub_6FCF8C70(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF8DD0
void __fastcall sub_6FCF8DD0(UnitAny* pDefender);
//D2Game.0x6FCF8ED0
int32_t __fastcall SKILLS_SrvDo047_CloakOfShadows(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF9160
int32_t __fastcall SKILLS_AuraCallback_CloakOfShadows(AuraCallback* pAuraCallback, UnitAny* pUnit);
//D2Game.0x6FCF9260
int32_t __fastcall SKILLS_SrvDo048_BladeFury(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF93B0
int32_t __fastcall SKILLS_SrvSt26_BladeFury(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF9550
void __fastcall SKILLS_StatRemoveCallback_RemoveState(UnitAny* pUnit, int32_t nState, StatList* pStatList);
//D2Game.0x6FCF9580
void __fastcall sub_6FCF9580(Game* pGame, UnitAny* pUnit, UnitAny* pPet, int32_t nSkillId, int32_t nSkillLevel, int32_t nItemLevel, int32_t bCheckOnInit);
//D2Game.0x6FCF9750
int32_t __fastcall SKILLS_SrvDo049_ShadowWarrior_Master(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF9B50
int32_t __fastcall SKILLS_SrvSt27_DragonTail(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF9C70
int32_t __fastcall SKILLS_SrvDo050_DragonTail(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCF9EA0
void __fastcall SKILLS_StatRemoveCallback_MindBlast(UnitAny* pItem, int32_t nState, StatList* pStatList);
//D2Game.0x6FCF9FF0
int32_t __fastcall SKILLS_AuraCallback_MindBlast(AuraCallback* pAuraCallback, UnitAny* pUnit);
//D2Game.0x6FCFA350
int32_t __fastcall SKILLS_SrvDo051_MindBlast(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFA4F0
int32_t __fastcall SKILLS_SrvDo052_DragonFlight(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFA720
int32_t __fastcall SKILLS_SrvSt28_BladeShield(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFA880
int32_t __fastcall SKILLS_SrvDo053_Unused(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFA980
int32_t __fastcall SKILLS_AuraCallback_SrvDo142(AuraCallback* pAuraCallback, UnitAny* pUnit);
//D2Game.0x6FCFAA20
int32_t __fastcall SKILLS_SrvDo142_Unused(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFAB60
int32_t __fastcall SKILLS_SrvDo054_BladeShield(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
