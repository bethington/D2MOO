#pragma once

#include <Units/Units.h>
#include <UNIT/SUnitDmg.h>


struct AuraCallback;


//D2Game.0x6FD0AF30
int32_t __fastcall SKILLS_SrvSt15_RaiseSkeleton_Mage(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0AF70
int32_t __fastcall SKILLS_SrvSt16_PoisonDagger(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0B0B0
int32_t __fastcall SKILLS_SrvSt17_Poison_CorpseExplosion(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0B0F0
int32_t __fastcall SKILLS_SrvSt19_BonePrison(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0B120
int32_t __fastcall SKILLS_SrvSt20_IronGolem(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0B190
int32_t __fastcall SKILLS_SrvSt21_Revive(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0B250
void __fastcall sub_6FD0B250(UnitAny* pUnit, int32_t nState, StatList* pStatList);
//D2Game.0x6FD0B2B0
int32_t __fastcall sub_6FD0B2B0(Game* pGame, UnitAny* pUnit, UnitAny* pTarget);
//D2Game.0x6FD0B3D0
int32_t __fastcall sub_6FD0B3D0(UnitAny* pUnit, int32_t nStatId, int32_t nResist);
//D2Game.0x6FD0B450
int32_t __fastcall sub_6FD0B450(UnitAny* pUnit, void* pArgs);
//D2Game.0x6FD0B790
int32_t __fastcall SKILLS_SrvDo030_Curse(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0B9F0
void __fastcall sub_6FD0B9F0(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, int32_t nSkillLevel);
//D2Game.0x6FD0BB60
int32_t __fastcall SKILLS_SrvDo059_Attract(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0BDA0
int32_t __fastcall sub_6FD0BDA0(UnitAny* pUnit, void* pArg);
//D2Game.0x6FD0BE50
int32_t __fastcall SKILLS_SrvDo061_Confuse(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0C060
int32_t __fastcall sub_6FD0C060(UnitAny* pUnit, void* pArg);
//D2Game.0x6FD0C2B0
void __fastcall sub_6FD0C2B0(UnitAny* pUnit, int32_t nState, StatList* pStatList);
//D2Game.0x6FD0C2E0
void __fastcall D2GAME_SetSummonResistance_6FD0C2E0(UnitAny* pUnit, UnitAny* pPet);
//D2Game.0x6FD0C3A0
void __fastcall D2GAME_SetUnitComponent_6FD0C3A0(UnitAny* pUnit, UnitAny* pPet, int32_t nLevel, int32_t bHasShield, int32_t bSpecial);
//D2Game.0x6FD0C500
void __fastcall sub_6FD0C500(UnitAny* pMonster, uint8_t nIndex, uint8_t nComponent);
//D2Game.0x6FD0C530
int32_t __fastcall D2GAME_SetSummonPassiveStats_6FD0C530(Game* pGame, UnitAny* pUnit, UnitAny* pPet, int32_t nSkillId, int32_t nSkillLevel, int32_t nItemLevel);
//D2Game.0x6FD0CB10
int32_t __fastcall D2GAME_SKILLS_SetSummonBaseStats_6FD0CB10(Game* pGame, UnitAny* pUnit, UnitAny* pPet, int32_t nPetLevelArg, int32_t nSkillLevel);
//D2Game.0x6FD0CC10
int32_t __fastcall SKILLS_SrvDo031_RaiseSkeleton_Mage(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0CFC0
int32_t __fastcall SKILLS_SrvDo032_PoisonDagger(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0D000
int32_t __fastcall sub_6FD0D000(AuraCallback* pAuraCallback, UnitAny* pUnit);
//D2Game.0x6FD0D0D0
int32_t __fastcall SKILLS_SrvDo055_CorpseExplosion(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0D620
int32_t __fastcall SKILLS_SrvDo056_Golem(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0D7B0
int32_t __fastcall SKILLS_SrvDo057_IronGolem(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0DAC0
int32_t __fastcall SKILLS_SrvDo058_Revive(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0DF40
void __fastcall sub_6FD0DF40(Game* pGame, UnitAny* pUnit, UnitAny* pMonster, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0E050
int32_t __fastcall SKILLS_SrvDo060_BoneWall(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0E4C0
int32_t __fastcall SKILLS_SrvDo062_BonePrison(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0E790
int32_t __fastcall SKILLS_SrvDo063_PoisonExplosion(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0E840
int32_t __fastcall D2GAME_EventFunc04_6FD0E840(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0EDE0
int32_t __fastcall D2GAME_EventFunc05_6FD0EDE0(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0F000
int32_t __fastcall D2GAME_EventFunc22_6FD0F000(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0F1F0
int32_t __fastcall D2GAME_EventFunc23_6FD0F1F0(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0F590
int32_t __fastcall sub_6FD0F590(UnitAny* pUnit, int32_t nValue);
//D2Game.0x6FD0F5E0
int32_t __fastcall D2GAME_EventFunc26_6FD0F5E0(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD0F7A0
int32_t __fastcall D2GAME_EventFunc27_6FD0F7A0(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
