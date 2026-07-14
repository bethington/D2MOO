#pragma once

#include <Units/Units.h>
#include <DataTbls/SkillsTbls.h>

//D2Game.0x6FD061C0
int32_t __fastcall SKILLS_SrvSt42_FireHit(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD062E0
int32_t __fastcall SKILLS_SrvDo083_FireHit(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD06330
int32_t __fastcall SKILLS_SrvSt43_MaggotEgg(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD06360
int32_t __fastcall SKILLS_SrvDo084_MaggotEgg(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD064D0
int32_t __fastcall SKILLS_SrvDo085_UnholyBolt_ShamanFire(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD06590
int32_t __fastcall SKILLS_SrvSt44_MaggotUp(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD066A0
int32_t __fastcall SKILLS_SrvSt45_MaggotDown(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD06730
int32_t __fastcall SKILLS_SrvDo086_MaggotDown(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD06880
int32_t __fastcall SKILLS_SrvDo087_MaggotLay(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD06A60
int32_t __fastcall SKILLS_SrvSt46_AndrialSpray(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD06B20
int32_t __fastcall SKILLS_SrvDo088_AndrialSpray(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD06D60
int32_t __fastcall SKILLS_SrvSt47_Jump(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD070F0
int32_t __fastcall SKILLS_SrvDo089_Jump(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD074B0
int32_t __fastcall SKILLS_SrvSt48_SwarmMove(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD07540
int32_t __fastcall SKILLS_SrvDo090_SwarmMove(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD075E0
int32_t __fastcall SKILLS_SrvSt49_Nest_EvilHutSpawner(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD07770
int32_t __fastcall SKILLS_SrvDo091_Nest_EvilHutSpawner(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD07920
int32_t __fastcall SKILLS_SrvSt50_QuickStrike(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD07970
int32_t __fastcall SKILLS_SrvDo092_QuickStrike(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD07A30
int32_t __fastcall SKILLS_SrvDo093_GargoyleTrap(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD07C50
int32_t __fastcall SKILLS_SrvSt51_Submerge(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD07C70
int32_t __fastcall SKILLS_SrvDo094_Submerge(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD07CB0
int32_t __fastcall SKILLS_SrvSt52_Emerge(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD07CD0
int32_t __fastcall SKILLS_SrvSt53_MonInferno(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD07D80
void __fastcall SKILLS_SetInfernoFrame(SkillsTxt* pSkillsTxtRecord, UnitAny* pUnit);
//D2Game.0x6FD07E30
int32_t __fastcall SKILLS_SrvDo095_MonInferno(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD081E0
int32_t __fastcall SKILLS_UpdateInfernoAnimationParameters(UnitAny* pUnit, UnitAny* pMissile, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD082D0
int32_t __fastcall SKILLS_SrvDo152_DiabLight(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD08680
int32_t __fastcall SKILLS_SrvDo096_ZakarumHeal_Bestow(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD08850
int32_t __fastcall SKILLS_ResurrectUnit(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FD089E0
int32_t __fastcall SKILLS_SrvDo097_Resurrect(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD08BB0
int32_t __fastcall SKILLS_SrvDo098_MonTeleport(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD08CD0
int32_t __fastcall SKILLS_SrvDo099_PrimePoisonNova(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD08EB0
int32_t __fastcall SKILLS_SrvDo100_DiabCold(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD09000
int32_t __fastcall SKILLS_SrvDo101_FingerMageSpider(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD092B0
int32_t __fastcall SKILLS_SrvDo102_DiabWall(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD09410
void __fastcall SKILLS_MissileInit_DiabWall(UnitAny* pMissile, int32_t nInitSeed);
//D2Game.0x6FD094B0
int32_t __fastcall SKILLS_SrvSt54_DiabRun(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD09570
int32_t __fastcall SKILLS_SrvDo103_DiabRun(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD09890
int32_t __fastcall SKILLS_SrvDo104_DiabPrison(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD09970
int32_t __fastcall SKILLS_SrvDo105_DesertTurret(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD09BF0
int32_t __fastcall SKILLS_SrvDo106_ArcaneTower(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD09C90
int32_t __fastcall SKILLS_SrvSt55_Mosquito(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD09DE0
int32_t __fastcall SKILLS_SrvDo107_Mosquito(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0A190
int32_t __fastcall SKILLS_SrvDo108_RegurgitatorEat(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0A340
int32_t __fastcall SKILLS_GetMonFrenzySequenceFrame(UnitAny* pUnit);
//D2Game.0x6FD0A3D0
int32_t __fastcall SKILLS_RollMonFrenzyDamage(Game* pGame, UnitAny* pUnit, UnitAny* pTargetUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0A520
int32_t __fastcall SKILLS_SrvSt25_64_DragonClaw_MonFrenzy(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0A530
int32_t __fastcall SKILLS_SrvDo109_MonFrenzy(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0A5E0
int32_t __fastcall SKILLS_SrvDo148_DoomKnightMissile(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0A720
int32_t __fastcall SKILLS_SrvDo149_NecromageMissile(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0A860
int32_t __fastcall SKILLS_SrvDo110_Hireable_RogueMissile(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0A9B0
int32_t __fastcall SKILLS_SrvDo112_MonCurseCast(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0AC20
UnitAny* __fastcall SKILLS_CreateSpiderLayMissile(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FD0AE10
int32_t __fastcall SKILLS_SrvDo111_FetishAura(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
