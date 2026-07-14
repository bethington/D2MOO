#pragma once

#include "AiGeneral.h"
#include <Units/Units.h>
#include <Units/UnitFinds.h>


void __fastcall AITHINK_Fn000(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD1660
void __fastcall D2GAME_AI_SpecialState02_6FCD1660(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD1750
void __fastcall AITHINK_Fn002_Skeleton(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD1880
void __fastcall AITHINK_Fn003_Zombie(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD1990
void __fastcall AITHINK_Fn004_Bighead(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD1BA0
void __fastcall AITHINK_Fn005_BloodHawk(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD1D50
void __fastcall AITHINK_Fn006_Fallen(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD2220
void __fastcall AITHINK_Fn007_Brute(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD2370
void __fastcall AITHINK_Fn008_SandRaider(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD2680
UnitAny* __fastcall AITHINK_TargetCallback_SandRaider(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCD27A0
void __fastcall AITHINK_Fn009_Wraith(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD2850
void __fastcall AITHINK_Fn010_CorruptRogue(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD2A00
void __fastcall AITHINK_Fn011_Baboon(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD2E80
void __fastcall AITHINK_Fn014_QuillRat(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD2FF0
void __fastcall AITHINK_Fn013_FallenShaman(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD32E0
UnitAny* __fastcall AITHINK_TargetCallback_FallenShaman(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCD34A0
void __fastcall D2GAME_AI_Unk015_6FCD34A0(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD3540
void __fastcall AITHINK_Fn015_SandMaggot(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD3900
void __fastcall AITHINK_Fn016_ClawViper(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD3B90
void __fastcall AITHINK_Fn142_ClawViperEx(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD3E70
void __fastcall AITHINK_Fn017_SandLeaper(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD4050
void __fastcall AITHINK_Fn018_PantherWoman(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD41F0
UnitAny* __fastcall AITHINK_TargetCallback_Panther(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCD4390
void __fastcall AITHINK_Fn012_019_Goatman_Swarm(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD4440
void __fastcall AITHINK_Fn020_Scarab(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD4720
void __fastcall AITHINK_Fn021_Mummy(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD48B0
void __fastcall AITHINK_Fn022_GreaterMummy(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD4C10
UnitAny* __fastcall AITHINK_TargetCallback_GreaterMummy(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCD4DD0
void __fastcall AITHINK_Fn023_Vulture(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD55D0
int32_t __fastcall sub_6FCD55D0(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCD5710
UnitAny* __fastcall AITHINK_TargetCallback_Vulture(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCD5850
void __fastcall AITHINK_Fn024_Mosquito(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD5A60) --------------------------------------------------------
void __fastcall AITHINK_Fn025_Willowisp(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD6960
int32_t __fastcall AITHINK_UnitFindCallback_Willowisp(UnitAny* pUnit, UnitFindArg* pUnitFindArg);
//D2Game.0x6FCD69E0
void __fastcall AITHINK_Fn026_Arach(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD6D60
void __fastcall AITHINK_Fn027_ThornHulk(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD6FD0
void __fastcall AITHINK_Fn028_Vampire(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD76F0
void __fastcall D2GAME_AI_Unk029_6FCD76F0(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD7760
void __fastcall AITHINK_Fn029_BatDemon(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD7BA0
void __fastcall AITHINK_Fn030_Fetish(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD7EB0
void __fastcall AITHINK_Fn033_HellMeteor(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD8090
void __fastcall AITHINK_Fn034_Andariel(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD8260
void __fastcall AITHINK_Fn035_CorruptArcher(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD85B0
void __fastcall AITHINK_Fn036_CorruptLancer(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD88C0
void __fastcall AITHINK_Fn037_SkeletonBow(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD8A60
void __fastcall AITHINK_Fn038_MaggotLarva(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD8B60
void __fastcall AITHINK_Fn039_PinHead(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD8D20
void __fastcall AITHINK_Fn040_MaggotEgg(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD8E10
void __fastcall D2GAME_AI_Unk043_045_121_6FCD8E10(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD8E30
void __fastcall AITHINK_Fn043_FoulCrowNest(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD8FE0
void __fastcall AITHINK_Fn044_Duriel(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD91F0
void __fastcall AITHINK_Fn045_Sarcophagus(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD93A0
void __fastcall AITHINK_Fn046_ElementalBeast(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD94D0
void __fastcall AITHINK_Fn047_FlyingScimitar(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD9640
void __fastcall AITHINK_Fn048_ZakarumZealot(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD9A10
void __fastcall AITHINK_Fn049_ZakarumPriest(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCD9F10
UnitAny* __fastcall AITHINK_TargetCallback_ZakarumPriest(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCDA0C0
UnitAny* __fastcall AITHINK_FindTargetForMephisto(Game* pGame, UnitAny* pUnit, MonStatsTxt* pMonStatsTxtRecord, UnitAny* a4);
//D2Game.0x6FCDA190
UnitAny* __fastcall AITHINK_TargetCallback_Mephisto(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCDA300
void __fastcall AITHINK_Fn050_Mephisto(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDA910
void __fastcall D2GAME_AI_Unk052_6FCDA910(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDAAA0
void __fastcall AITHINK_Fn052_FrogDemon(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDAFC0
void __fastcall AITHINK_Fn053_Summoner(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDB3E0
void __fastcall AITHINK_Fn055_Izual(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDB720
void __fastcall AITHINK_Fn056_Tentacle(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDBAA0
void __fastcall AITHINK_Fn057_TentacleHead(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDBCE0
void __fastcall AITHINK_Fn063_GargoyleTrap(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDBF20
void __fastcall AITHINK_Fn064_SkeletonMage(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDC170
void __fastcall D2GAME_AI_SpecialState04_6FCDC170(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDC1C0
void __fastcall AITHINK_Fn065_FetishShaman(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDC420
UnitAny* __fastcall AITHINK_TargetCallback_FetishShaman(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCDC600
void __fastcall AITHINK_Fn066_SandMaggotQueen(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDC840
void __fastcall AITHINK_Fn068_VileMother(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDCBF0
void __fastcall AITHINK_Fn069_VileDog(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDCCD0
void __fastcall AITHINK_Fn070_FingerMage(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDD060
void __fastcall AITHINK_Fn071_Regurgitator(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDD5C0
UnitAny* __fastcall AITHINK_TargetCallback_Regurgitator(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCDD790
void __fastcall AITHINK_Fn072_DoomKnight(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDD850
void __fastcall AITHINK_Fn073_AbyssKnight(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDDB10
void __fastcall AITHINK_Fn074_OblivionKnight(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDDFA0
UnitAny* __fastcall AITHINK_TargetCallback_OblivionKnight(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCDE150
void __fastcall AITHINK_Fn075_QuillMother(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDE2B0
void __fastcall AITHINK_Fn076_EvilHole(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDE4D0
void __fastcall AITHINK_Fn077_TrapMissile(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDE570
void __fastcall AITHINK_Fn078_TrapRightArrow(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDE710
void __fastcall AITHINK_Fn079_TrapLeftArrow(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDE8B0
void __fastcall AITHINK_Fn080_092_TrapPoison_TrapNova(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDE960
void __fastcall AITHINK_Fn087_TrapMelee(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDE9E0
void __fastcall AITHINK_Fn082_InvisoSpawner(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDEAF0
void __fastcall AITHINK_Fn083_MosquitoNest(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDEC70
void __fastcall D2GAME_AI_Unk084_6FCDEC70(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDECE0
void __fastcall AITHINK_Fn084_BoneWall(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDED10
void __fastcall AITHINK_Fn085_HighPriest(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDF2E0
UnitAny* __fastcall AITHINK_TargetCallback_HighPriest(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCDF410
void __fastcall AITHINK_Fn094_DesertTurret(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDF780
void __fastcall AITHINK_Fn089_Megademon(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDFA50
void __fastcall AITHINK_Fn093_ArcaneTower(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDFB80
void __fastcall AITHINK_Fn095_PantherJavelin(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCDFD50
void __fastcall AITHINK_Fn096_FetishBlowgun(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE0050
void __fastcall AITHINK_Fn114_ReanimatedHorde(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE0220
void __fastcall AITHINK_Fn113_SiegeTower(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE0430
UnitAny* __fastcall AITHINK_TargetCallback_SiegeBeast(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCE0610
void __fastcall AITHINK_Fn115_SiegeBeast(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE0960
int32_t __fastcall AITHINK_GetSquaredDistance(UnitAny* pUnit1, UnitAny* pUnit2);
//D2Game.0x6FCE0A50
void __fastcall AITHINK_Fn116_Minion(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE0C10
void __fastcall AITHINK_Fn117_SuicideMinion(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE0CD0
void __fastcall AITHINK_Fn118_Succubus(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE0FE0
void __fastcall AITHINK_Fn119_SuccubusWitch(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE1480
void __fastcall D2GAME_AI_SpecialState14_6FCE1480(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE1550
void __fastcall AITHINK_Fn120_Overseer(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE1910
UnitAny* __fastcall AITHINK_TargetCallback_Overseer_Nihlathak(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCE1B90
void __fastcall AITHINK_Fn121_MinionSpawner(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE1CA0
UnitAny* __fastcall AITHINK_TargetCallback_MinionSpawner(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCE1D30
void __fastcall D2GAME_AI_SpecialState16_6FCE1D30(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE1DC0
void __fastcall D2GAME_AI_SpecialState16_6FCE1DC0(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE2080
void __fastcall D2GAME_AI_Unk122_6FCE2080(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE2090
void __fastcall AITHINK_Fn122_Imp(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE2570
void __fastcall AITHINK_Fn123_Catapult(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE25D0
void __fastcall AITHINK_Fn124_FrozenHorror(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE2760
void __fastcall AITHINK_Fn125_BloodLord(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE28A0
void __fastcall AITHINK_Fn097_Spirit(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE28F0
void __fastcall AITHINK_Fn098_Smith(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE2960
void __fastcall AITHINK_Fn067_NecroPet(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE2BA0
int32_t __fastcall D2GAME_AI_PetMove_6FCE2BA0(Game* pGame, UnitAny* pOwner, UnitAny* pUnit, int32_t a4, int32_t bRun, int32_t nVelocity, int32_t a7);
//D2Game.0x6FCE34B0
void __fastcall sub_6FCE34B0(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCE34E0
int32_t __fastcall sub_6FCE34E0(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, UnitAny* pOwner, int32_t a5, AiTickParam* pAiTickParam, int32_t a7, int32_t a8);
//D2Game.0x6FCE3740
void __fastcall sub_6FCE3740(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE39E0
void __fastcall AITHINK_Fn061_Hireable(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE3EE0
int32_t __fastcall D2GAME_PETAI_PetMove_6FCE3EE0(Game* pGame, UnitAny* pOwner, UnitAny* pUnit, int32_t eMotionType, int32_t bRun, int32_t nSpeed, BYTE bSteps);
//D2Game.0x6FCE4610
void __fastcall sub_6FCE4610(Game* pGame, UnitAny* pUnit, int32_t nClassId, UnitAny* pOwner, UnitAny* pTarget, Seed* pSeed, AiTickParam* pAiTickParam);
//D2Game.0x6FCE4830
void __fastcall sub_6FCE4830(Game* pGame, UnitAny* pUnit, int32_t nMonsterId, int32_t nHirelingId, UnitAny* pTarget, Seed* pSeed);
//D2Game.0x6FCE4B90
void __fastcall D2GAME_AI_SpecialState03_6FCE4B90(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE4CC0
void __fastcall D2GAME_AI_SpecialState03_6FCE4CC0(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE4FD0
void __fastcall sub_6FCE4FD0(UnitAny* pUnit, void* a2, void* a3);
//D2Game.0x6FCE4FF0
void __fastcall AITHINK_Fn090_Griswold(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE5080
void __fastcall D2GAME_AI_SpecialState13_6FCE5080(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE5520
int32_t __fastcall sub_6FCE5520(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE5610
void __fastcall D2GAME_AI_Unk129_6FCE5610(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE5640
void __fastcall AITHINK_Fn129_GenericSpawner(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE58D0
void __fastcall D2GAME_AI_Unk59_6FCE58D0(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE58E0
void __fastcall AITHINK_Fn059_BloodRaven(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE5EE0
uint8_t __fastcall sub_6FCE5EE0(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE61E0
int32_t __fastcall sub_6FCE61E0(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam, MapAIPathPosition* pPosition);
//D2Game.0x6FCE6270
int32_t __fastcall sub_6FCE6270(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam, MapAIPathPosition* pPosition);
//D2Game.0x6FCE6340
int32_t __fastcall sub_6FCE6340(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam, MapAIPathPosition* pPosition);
//D2Game.0x6FCE64D0
int32_t __fastcall sub_6FCE64D0(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam, MapAIPathPosition* pPosition);
//D2Game.0x6FCE6660
void __fastcall AITHINK_Fn032_Npc(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE69A0
uint8_t __fastcall sub_6FCE69A0(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE6CD0
int32_t __fastcall AITHINK_ExecuteMapAiAction(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE6DC0
void __fastcall AITHINK_Fn054_NpcStationary(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE6F80
void __fastcall AITHINK_Fn041_Towner(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE7070
void __fastcall AITHINK_Fn081_JarJar(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE73A0
void __fastcall AITHINK_Fn031_NpcOutOfTown(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE7640
int32_t __fastcall AITHINK_SpawnNpcPortal(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE77A0
void __fastcall AITHINK_Fn060_GoodNpcRanged(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE7930
void __fastcall D2GAME_AI_SpecialState06_6FCE7930(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE7A60
void __fastcall AITHINK_Fn062_TownRogue(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE7AC0
void __fastcall AITHINK_Fn058_Navi(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE7C10
void __fastcall AITHINK_Fn001_100_Idle_Buffy(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE7C40
void __fastcall D2GAME_AI_SpecialState09_6FCE7C40(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE7CF0
void __fastcall D2GAME_AI_SpecialState10_17_6FCE7CF0(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE7E20
void __fastcall D2GAME_AI_SpecialState11_6FCE7E20(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE7E80
void __fastcall D2GAME_AI_SpecialState11_6FCE7E80(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE81B0
void __fastcall D2GAME_AI_SpecialState12_6FCE81B0(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE82F0
void __fastcall D2GAME_AI_Unk051_6FCE82F0(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE83A0
int32_t __fastcall AITHINK_GetTargetScore(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, AiCmd* pAiCmd);
//D2Game.0x6FCE86C0
UnitAny* __fastcall AITHINK_GetTargetForBoss(Game* pGame, UnitAny* pUnit, int32_t* a3, int32_t* pCounter, AiCmd* pAiCmd, int32_t(__fastcall* pfCull)(UnitAny*, UnitAny*));
//D2Game.0x6FCE8950
void __fastcall AITHINK_Fn051_Diablo(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE97C0
int32_t __fastcall AITHINK_CullPotentialTargetsForDiablo(UnitAny* pUnit, UnitAny* pTarget);
//D2Game.0x6FCE9890
void __fastcall AITHINK_Fn042_Vendor(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE98E0
void __fastcall AITHINK_Fn086_Hydra(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE9980
void __fastcall AITHINK_Fn099_TrappedSoul(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE9AF0
void __fastcall AITHINK_Fn088_7TIllusion(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE9BA0
void __fastcall AITHINK_Fn091_DarkWanderer(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE9CE0
void __fastcall D2GAME_AI_Unk101_104_6FCE9CE0(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE9D00
void __fastcall AITHINK_Fn101_AssassinSentry(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE9E60
int32_t __fastcall AITHINK_AssasinSentryHasLostTarget(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam, int32_t bDecreaseParam);
//D2Game.0x6FCE9FB0
void __fastcall D2GAME_AI_Unk102_6FCE9FB0(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCE9FD0
void __fastcall AITHINK_Fn102_BladeCreeper(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEA2A0
void __fastcall AITHINK_Fn103_InvisoPet(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEA490
void __fastcall AITHINK_Fn104_DeathSentry(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEA680
void __fastcall D2GAME_AI_Unk105_6FCEA680(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEA6D0
void __fastcall AITHINK_Fn105_ShadowWarrior(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEAC10
BOOL __fastcall AITHINK_ShadowWarriorCheckUseSkill(Game* pGame, UnitAny* pUnit, UnitAny* pOwner, Skill* pSkill, int32_t nSkillId, int32_t bCombat, AiTickParam* pAiTickParam);
//D2Game.0x6FCEAF20
int32_t __fastcall AITHINK_ShadowMasterCheckTargetPetType(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, int32_t nSkillId);
//D2Game.0x6FCEAFE0
void __fastcall D2GAME_AI_Unk106_6FCEAFE0(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEB1B0
void __fastcall D2GAME_AI_Unk143_6FCEB1B0(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEB240
void __fastcall AITHINK_Fn106_143_ShadowMaster(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEC840
int32_t __fastcall AITHINK_IsTargetKillableByShadowMaster(UnitAny* a1, UnitAny* a2);
//D2Game.0x6FCEC910
UnitAny* __fastcall AITHINK_TargetCallback_ShadowMaster(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCECBA0
int32_t __fastcall sub_6FCECBA0(Game* pGame, UnitAny* a2, UnitAny* pUnit, WORD wSkillId, int32_t a5, UnitAny* pTarget, int32_t nX, int32_t nY, AiTickParam* a9);
//D2Game.0x6FCECC40
void __fastcall D2GAME_AI_Unk_110_111_6FCECC40(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCECC50
void __fastcall AITHINK_Fn110_Vines(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCECE50
void __fastcall AITHINK_Fn111_CycleOfLife(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCED140
void __fastcall D2GAME_AI_Unk107_6FCED140(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCED190
void __fastcall AITHINK_Fn107_Raven(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//Inlined in D2Game.0x6FCED540
void __fastcall AITHINK_Fn108_Fenris(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//Inlined in D2Game.0x6FCED540
void __fastcall AITHINK_Fn108_SpiritWolf(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCED540
void __fastcall AITHINK_Fn108_DruidWolf(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEDF70
void __fastcall AITHINK_Fn112_DruidBear(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEE250
void __fastcall AITHINK_Fn109_Totem(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEE450
void __fastcall AITHINK_Fn127_NpcBarb(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEE6F0
void __fastcall AITHINK_Fn126_CatapultSpotter(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEEAD0
void __fastcall AITHINK_Fn130_DeathMauler(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEEC00
void __fastcall AITHINK_Fn131_Wussie(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEEE60
void __fastcall AITHINK_Fn128_Nihlathak(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEF330
void __fastcall AITHINK_Fn132_AncientStatue(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEF3F0
void __fastcall AITHINK_AncientBarb1SkillHandler(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEF730
int32_t __fastcall AITHINK_AreUnitsInSameLevel(UnitAny* pUnit, UnitAny* pTarget);
//Inlined in D2Game.0x6FCEFBC0
void __fastcall AITHINK_AncientBarb2SkillHandler(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEFA10
void __fastcall AITHINK_AncientBarb3SkillHandler(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEFBC0
void __fastcall AITHINK_Fn133_Ancient(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCEFBF0
void __fastcall AITHINK_Fn134_BaalThrone(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCF0030
void __fastcall AITHINK_BaalThroneSetAiParamFlag(AiTickParam* pAiTickParam, int32_t nFlag, int32_t bSet);
//D2Game.0x6FCF0050
UnitAny* __fastcall AITHINK_TargetCallback_BaalToStairs(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCF0090
void __fastcall AITHINK_Fn138_BaalToStairs(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCF0180
void __fastcall AITHINK_Fn136_BaalTaunt(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCF02D0
void __fastcall AITHINK_Fn139_BaalTentacle(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCF0420
void __fastcall AITHINK_Fn141_BaalMinion(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCF0570
UnitAny* __fastcall AITHINK_TargetCallback_PutridDefiler(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, void* pCallbackArg);
//D2Game.0x6FCF05B0
void __fastcall AITHINK_Fn137_PutridDefiler(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCF06A0
AiSpecialState __fastcall AITHINK_GetSpecialStateFromAiControl(AiControl* pAiControl);
//D2Game.0x6FCF06B0
void __fastcall AITHINK_SetAiControlParams(AiControl* pAiControl, int32_t nParam0, int32_t nParam1, int32_t nParam2);
//D2Game.0x6FCF06E0
BOOL __fastcall AITHINK_CanUnitSwitchAi(UnitAny* pUnit, MonStatsTxt* pMonStatsTxtRecord, AiSpecialState nAiSpecialState, int32_t bCheckIfSuperUnique);
//D2Game.0x6FCF0750
const AiTable* __fastcall AITHINK_GetAiTableRecord(UnitAny* pUnit, AiSpecialState nAiSpecialState);
//D2Game.0x6FCF07D0
void __fastcall AITHINK_ExecuteAiFn(Game* pGame, UnitAny* pUnit, AiControl* pAiControl, AiSpecialState nAiSpecialState);
//D2Game.0x6FCF0A70
void __fastcall D2GAME_MONSTERS_AiFunction03_6FCF0A70(Game* pGame, UnitAny* pUnit, int32_t a3, int32_t a4);
//D2Game.0x6FCF0D10
int32_t __fastcall D2GAME_AICORE_MinionLeash_6FCF0D10(Game* pGame, UnitAny* pMonster, AiTickParam* pAiTickParam);
//Inlined in D2Game.0x6FCF0E40
int32_t __fastcall sub_6FCF0E40_inline(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);
//D2Game.0x6FCF0E40
int32_t __fastcall sub_6FCF0E40(Game* pGame, UnitAny* pUnit, AiTickParam* pAiTickParam);