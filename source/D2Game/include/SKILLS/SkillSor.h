#pragma once

#include <Units/Units.h>
#include <UNIT/SUnitDmg.h>


struct AuraCallback;


//D2Game.0x6FD156F0
int32_t __fastcall SKILLS_DoInferno(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel, int32_t nMissileId);
//D2Game.0x6FD15940
int32_t __fastcall SKILLS_StartInferno(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel, int32_t a5);
//D2Game.0x6FD15AB0
int32_t __fastcall SKILLS_SrvSt11_Inferno_ArcticBlast(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD15B40
int32_t __fastcall SKILLS_SrvSt12_Telekinesis_DragonFlight(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD15CF0
int32_t __fastcall SKILLS_SrvSt13_ThunderStorm(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD15D50
int32_t __fastcall SKILLS_SrvSt14_Hydra(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD15E50
void __fastcall SKILLS_MissileInit_ChargedBolt(UnitAny* pMissile, int32_t a2);
//D2Game.0x6FD15EB0
int32_t __fastcall SKILLS_SrvDo017_ChargedBolt_BoltSentry(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD16040
void __fastcall SKILLS_CurseStateCallback_DefensiveBuff(UnitAny* pUnit, int32_t nState, StatList* pStatList);
//D2Game.0x6FD160A0
int32_t __fastcall SKILLS_SrvDo018_DefensiveBuff(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD16270
int32_t __fastcall SKILLS_SrvDo019_Inferno_ArcticBlast(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD162D0
int32_t __fastcall SKILLS_SrvDo020_StaticField(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD163E0
int32_t __fastcall SKILLS_AuraCallback_StaticField(AuraCallback* pAuraCallback, UnitAny* pDefender);
//D2Game.0x6FD166A0
int32_t __fastcall SKILLS_SrvDo021_Telekinesis(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD169A0
int32_t __fastcall SKILLS_SrvDo022_NovaAttack(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD16A60
int32_t __fastcall SKILLS_SrvDo023_Blaze_EnergyShield_SpiderLay(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD16C00
void __fastcall SKILLS_CreateBlazeMissile(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FD16D70
int32_t __fastcall SKILLS_SrvDo024_FireWall(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD17040
int32_t __fastcall SKILLS_SrvDo025_Enchant(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD17200
int32_t __fastcall SKILLS_SrvDo026_ChainLightning(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD172B0
int32_t __fastcall SKILLS_SrvDo151_Unused(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD17450
int32_t __fastcall SKILLS_SrvDo027_Teleport(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD174E0
int32_t __fastcall SKILLS_SrvDo028_Meteor_Blizzard_Eruption_BaalTaunt_Catapult(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD17570
int32_t __fastcall SKILLS_SrvDo029_ThunderStorm(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD17820
int32_t __fastcall SKILLS_EventFunc24_EnergyShield(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD17C30
int32_t __fastcall SKILLS_SrvDo144_Hydra(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD17F40
int32_t __fastcall SKILLS_EventFunc01_ChillingArmor(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD180E0
int32_t __fastcall SKILLS_EventFunc02_FrozenArmor(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD18200
int32_t __fastcall SKILLS_EventFunc03_ShiverArmor(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
