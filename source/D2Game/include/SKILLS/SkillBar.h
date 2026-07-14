#pragma once

#include <Units/Units.h>


//D2Game.0x6FCFABF0
int32_t __fastcall SKILLS_SrvSt32_Conversion_Bash_Stun_Concentrate_BearSmite(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFAE50
int32_t __fastcall SKILLS_SrvSt33_FindPotion_GrimWard(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFAE90
int32_t __fastcall SKILLS_SrvDo069_FindPotion(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFB070
int32_t __fastcall SKILLS_ApplyWarcryStats(Game* pGame, UnitAny* a2, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFB1D0
int32_t __fastcall SKILLS_SrvDo068_BasicShout(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFB270
int32_t __fastcall SKILLS_SrvDo070_DoubleSwing(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFB320
int32_t __fastcall SKILLS_Callback_FindTargetForTaunt(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCFB390
int32_t __fastcall SKILLS_SrvDo071_Taunt(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFB610
int32_t __fastcall SKILLS_SrvSt34_FindItem(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFB630
int32_t __fastcall SKILLS_SrvDo072_FindItem(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFB7A0
int32_t __fastcall SKILLS_SrvDo074_DoubleThrow(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFB8A0
int32_t __fastcall SKILLS_SrvDo075_GrimWard(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFBB40
int32_t __fastcall SKILLS_RollFrenzyDamage(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFBCE0
void __fastcall SKILLS_ApplyFrenzyStats(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFBE80
int32_t __fastcall SKILLS_SrvDo009_Frenzy(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFBF80
void __fastcall SKILLS_CurseStateCallback_Whirlwind(UnitAny* pUnit, int32_t nState, struct StatList* pStatList);
//D2Game.0x6FCFBFE0
int32_t __fastcall SKILLS_SrvSt38_Whirlwind(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFC3A0
int32_t __fastcall SKILLS_RemoveWhirlwindStats(Game* pGame, UnitAny* pUnit, Skill* pSkill, int32_t a4, int32_t nX, int32_t nY);
//D2Game.0x6FCFC4C0
int32_t __fastcall SKILLS_SrvDo076_Whirlwind(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFC8E0
int32_t __fastcall SKILLS_SrvSt39_Berserk(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFCB30
int32_t __fastcall SKILLS_SrvSt40_Leap(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFCF60
int32_t __fastcall SKILLS_FindLeapTargetPosition(UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel, int32_t* pX, int32_t* pY);
//D2Game.0x6FCFD280
int32_t __fastcall SKILLS_SrvDo077_Leap(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFD3C0
int32_t __fastcall SKILLS_SetVelocityForLeap(Game* pGame, UnitAny* pUnit, Skill* pSkill);
//D2Game.0x6FCFD4C0
int32_t __fastcall SKILLS_Leap(Game* pGame, UnitAny* pUnit, Skill* pSkill);
//D2Game.0x6FCFD880
int32_t __fastcall SKILLS_SrvSt41_LeapAttack(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFD9B0
int32_t __fastcall SKILLS_SrvDo078_LeapAttack(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FCFDC40
UnitAny* __fastcall SKILLS_FindLeapAttackTarget(Game* pGame, UnitAny* pUnit, Skill* pSkill);
