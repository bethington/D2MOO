#pragma once

#include <D2StatList.h>
#include <Units/Units.h>
#include <UNIT/SUnitDmg.h>


struct AuraCallback;


//D2Game.0x6FD18330
int32_t __fastcall SKILLS_SrvSt29_Sacrifice(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD184B0
int32_t __fastcall SKILLS_SrvDo064_Sacrifice(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD18650
int32_t __fastcall SKILLS_SrvDo150_Smite(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD18900
int32_t __fastcall SKILLS_SrvDo065_BasicAura(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD18BC0
int32_t __fastcall SKILLS_AuraCallback_BasicAura(AuraCallback* pAuraCallback, UnitAny* pUnit);
//D2Game.0x6FD18FE0
void __fastcall SKILLS_CurseStateCallback_BasicAura(UnitAny* pUnit, int32_t nState, StatList* pStatList);
//D2Game.0x6FD19020
int32_t __fastcall SKILLS_SrvDo066_HolyFire_HolyShock_Sanctuary_Conviction(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD19390
int32_t __fastcall SKILLS_AuraCallback_HolyFire_HolyShock_Sanctuary_Conviction(AuraCallback* pAuraCallback, UnitAny* pUnit);
//D2Game.0x6FD19460
int32_t __fastcall SKILLS_SrvSt31_Charge(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD19780
int32_t __fastcall SKILLS_SrvDo067_Charge(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD19C80
int32_t __fastcall SKILLS_SrvSt35_Vengeance(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD1A200
int32_t __fastcall SKILLS_SrvDo073_BlessedHammer(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD1A480
int32_t __fastcall SKILLS_SrvSt36_HolyShield(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD1A4A0
int32_t __fastcall SKILLS_SrvDo079_Conversion(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD1A900
void __fastcall SKILLS_StatRemoveCallback_Conversion(UnitAny* pUnit, int32_t nState, StatList* pStatList);
//D2Game.0x6FD1AA50
int32_t __fastcall SKILLS_SrvDo080_FistOfTheHeavens(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD1ABC0
int32_t __fastcall SKILLS_SrvDo081_HolyFreeze(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD1AF40
void __fastcall SKILLS_CurseStateCallback_HolyFreeze(UnitAny* pUnit, int32_t nState, StatList* pStatList);
//D2Game.0x6FD1AF90
int32_t __fastcall SKILLS_AuraCallback_HolyFreeze(AuraCallback* pAuraCallback, UnitAny* pUnit);
//D2Game.0x6FD1B100
int32_t __fastcall SKILLS_ApplyRedemptionEffect(Game* pGame, UnitAny* pUnit, UnitAny* pCorpse, int32_t nSkillId, int32_t nSkillLevel, int32_t bSkipChanceRoll);
//D2Game.0x6FD1B260
int32_t __fastcall SKILLS_SrvDo082_Redemption(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD1B490
int32_t __fastcall SKILLS_AuraCallback_Redemption(AuraCallback* pAuraCallback, UnitAny* pUnit);
//D2Game.0x6FD1B4C0
void __fastcall SKILLS_ApplyThornsDamage(Game* pGame, UnitAny* pAttacker, UnitAny* pDefender, Damage* pDamage);
