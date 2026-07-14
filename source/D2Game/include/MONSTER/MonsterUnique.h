#pragma once

#include <Units/Units.h>

#include <DataTbls/MissilesTbls.h>
#include <DataTbls/MonsterTbls.h>

#include <Drlg/D2DrlgDrlg.h>

//D2Game.0x6FC6AC00
void __fastcall MONSTERUNIQUE_ToggleUnitFlag(UnitAny* pUnit, int32_t nFlag, int32_t bSet);
//D2Game.0x6FC6AC30
uint8_t* __fastcall MONSTERUNIQUE_GetUMods(UnitAny* pUnit);
//D2Game.0x6FC6AC50
uint16_t __fastcall MONSTERUNIQUE_GetNameSeed(UnitAny* pUnit);
//D2Game.0x6FC6AC70
int32_t __fastcall MONSTERUNIQUE_CheckMonTypeFlag(UnitAny* pUnit, uint16_t nFlag);
//D2Game.0x6FC6ACA0
void __fastcall MONSTERUNIQUE_ToggleMonTypeFlag(UnitAny* pUnit, uint16_t nFlag, int32_t bSet);
//D2Game.0x6FC6ACD0
int16_t __fastcall MONSTERUNIQUE_GetBossHcIdx(UnitAny* pUnit);
//D2Game.0x6FC6ACF0
int32_t __fastcall MONSTERUNIQUE_HasUMods(UnitAny* pUnit);
//D2Game.0x6FC6AD10
int32_t __fastcall MONSTERUNIQUE_GetSuperUniqueBossHcIdx(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC6AD50
void __fastcall MONSTERUNIQUE_UMod1_RandomName(UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6AD90
void __fastcall MONSTERUNIQUE_UMod2_HealthBonus(UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6AF70
int32_t __fastcall MONSTERUNIQUE_CalculatePercentage(int32_t a1, int32_t a2, int32_t a3);
//D2Game.0x6FC6AFF0
void __fastcall MONSTERUNIQUE_UMod4_LevelBonus(UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6B030
void __fastcall MONSTERUNIQUE_UMod16_Champion(UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6B210
void __fastcall MONSTERUNIQUE_UMod36_Ghostly(UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6B3A0
void __fastcall MONSTERUNIQUE_UMod37_Fanatic(UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6B3E0
void __fastcall MONSTERUNIQUE_UMod38_Possessed(UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6B4B0
void __fastcall MONSTERUNIQUE_UMod39_Berserk(UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6B5D0
void __fastcall MONSTERUNIQUE_UMod41_AlwaysRun(UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6B610
void __fastcall MONSTERUNIQUE_UMod8_Resistant(UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6B8C0
void __fastcall MONSTERUNIQUE_UMod26_Teleport(UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6B910
void __fastcall MONSTERUNIQUE_UMod30_AuraEnchanted(UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6BA70
void __fastcall MONSTERUNIQUE_UMod5_Strong(UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6BB80
void __fastcall MONSTERUNIQUE_UMod6_Fast(UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6BC10
void __fastcall MONSTERUNIQUE_UMod9_FireEnchanted(UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6BDD0
void __fastcall MONSTERUNIQUE_UMod17_LightningEnchanted(UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6BF90
void __fastcall MONSTERUNIQUE_UMod18_ColdEnchanted(UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6C160
void __fastcall MONSTERUNIQUE_UMod23_PoisonEnchanted(UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6C340
void __fastcall MONSTERUNIQUE_UMod25_ManaSteal(UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6C4F0
void __fastcall MONSTERUNIQUE_CastAmplifyDamage(Game* pGame, UnitAny* pUnit, int32_t a3, int32_t a4);
//D2Game.0x6FC6C5B0
void __fastcall MONSTERUNIQUE_CurseCallback_ApplyAmplifyDamage(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, int32_t nSkillLevel);
//D2Game.0x6FC6C710
void __fastcall MONSTERUNIQUE_FireEnchantedModeChange(Game* pGame, UnitAny* pBoss, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6C740
void __fastcall MONSTERUNIQUE_CastCorpseExplode(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6C9E0
void __fastcall MONSTERUNIQUE_CastCorpseExplode2(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6CAB0
void __fastcall MONSTERUNIQUE_CastNova(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6CB40
void __fastcall MONSTERUNIQUE_CastLightUniqueMissile(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6CD30
void __fastcall sub_6FC6CD30(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6CD60
void __fastcall MONSTERUNIQUE_CastColdUniqueMissile(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6CDB0
void __fastcall MONSTERUNIQUE_CastCorpsePoisonCloud(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6CE50
void __fastcall MONSTERUNIQUE_KillMinions(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6CEC0
void __fastcall MONSTERUNIQUE_MinionCallback_KillMinion(UnitAny* pUnit, void* pGameArg, void* pUnused);
//D2Game.0x6FC6CF10
void __fastcall sub_6FC6CF10(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6CF90
void __fastcall sub_6FC6CF90(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6D030
void __fastcall MONSTERUNIQUE_ScarabModeChange(Game* pGame, UnitAny* pBoss, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6D060
void __fastcall MONSTERUNIQUE_CastBugLightningMissile(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6D1C0
void __fastcall MONSTERUNIQUE_ApplyElementalDamage(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6D410
MonUModTxt* __fastcall MONSTERUNIQUE_GetMonUModTxtRecord(uint32_t nUMod);
//D2Game.0x6FC6D440
void __fastcall sub_6FC6D440(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6D690
void __fastcall MONSTERUNIQUE_StealBeltItem(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6D800
void __fastcall MONSTERUNIQUE_QuestCompleteModeChange(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6D8B0
void __fastcall MONSTERUNIQUE_CastQueenPoisonCloudMissile(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6DA40
void __fastcall sub_6FC6DA40(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6DCB0
void __fastcall MONSTERUNIQUE_LightningEnchantedModeChange(Game* pGame, UnitAny* pBoss, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6DCE0
void __fastcall MONSTERUNIQUE_ColdEnchantedModeChange(Game* pGame, UnitAny* pBoss, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6DD20
void __fastcall sub_6FC6DD20(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6DDE0
void __fastcall sub_6FC6DDE0(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6DFA0
void __fastcall MONSTERUNIQUE_ApplyShatterState(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6DFC0
void __fastcall sub_6FC6DFC0(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6E070
void __fastcall MONSTERUNIQUE_SetTrapDamage(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6E240
void __fastcall sub_6FC6E240(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6E390
void __fastcall MONSTERUNIQUE_SuicideModeChange(Game* pGame, UnitAny* pBoss, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6E410
void __fastcall MONSTERUNIQUE_CastSuicideExplodeMissile(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6E700
void __fastcall MONSTERUNIQUE_CreatePainWorm(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6E730
void __fastcall sub_6FC6E730(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6E770
void __fastcall sub_6FC6E770(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC6E780
void __fastcall sub_6FC6E780(Game* pGame, UnitAny* pUnit, UnitAny* pMissile, int32_t nIndex);
//D2Game.0x6FC6E860
void __fastcall sub_6FC6E860(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC6E870
void __fastcall D2GAME_MONSTERS_AiFunction08_6FC6E870(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6E890
void __fastcall sub_6FC6E890(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC6E8A0
void __fastcall sub_6FC6E8A0(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC6E8B0
void __fastcall sub_6FC6E8B0(Game* pGame, UnitAny* pUnit, UnitAny* pMissile);
//D2Game.0x6FC6E8D0
UnitAny* __fastcall sub_6FC6E8D0(Game* pGame, Room1* pRoom, RoomCoordList* pRoomCoordList, int32_t nSuperUniqueId, int32_t a5, uint16_t nX, uint16_t nY, int32_t a8);
//D2Game.0x6FC6E940
void __fastcall sub_6FC6E940(Game* pGame, UnitAny* pUnit, int32_t a3);
//D2Game.0x6FC6EBE0
MonsterData* __fastcall MONSTERUNIQUE_GetMonsterData(UnitAny* pUnit);
//D2Game.0x6FC6EBF0
int32_t __fastcall MONSTERUNIQUE_GetUModCount(uint8_t* pUMods);
//D2Game.0x6FC6EC10
BOOL __fastcall sub_6FC6EC10(UnitAny* pUnit, MonUModTxt* pMonUModTxtRecord, int32_t bExpansion);
//D2Game.0x6FC6EE90
int32_t __fastcall sub_6FC6EE90(Game* pGame, UnitAny* pUnit, uint8_t* a3);
//D2Game.0x6FC6F160
uint32_t __fastcall MONSTERUNIQUE_CheckMonModeFlag(int32_t nMonsterId, int32_t nFlag);
//D2Game.0x6FC6F1D0
uint32_t __fastcall MONSTERUNIQUE_CheckMonStatsFlag(int32_t nMonsterId, int32_t nFlag);
//D2Game.0x6FC6F220
UnitAny* __fastcall D2GAME_SpawnMonster_6FC6F220(Game* pGame, Room1* pRoom, RoomCoordList* pRoomCoordList, int32_t nX, int32_t nY, int32_t nUnitGUID, int32_t nClassId, int32_t a8);
//D2Game.0x6FC6F440
void __fastcall D2GAME_SpawnMinions_6FC6F440(Game* pGame, Room1* pRoom, RoomCoordList* pRoomCoordList, UnitAny* pUnit, int32_t bSpawnMinions, int32_t nMinGroup, int32_t nMaxGroup);
//D2Game.0x6FC6F670
void __fastcall sub_6FC6F670(UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6F690
UnitAny* __fastcall D2GAME_SpawnSuperUnique_6FC6F690(Game* pGame, Room1* pRoom, int32_t nX, int32_t nY, int32_t nSuperUnique);
//D2Game.0x6FC6FBA0
UnitAny* __fastcall sub_6FC6FBA0(Game* pGame, Room1* pRoom, int32_t nX, int32_t nY, int32_t nClassId, int32_t nUnitGUID, uint16_t nNameSeed, int32_t bChampion, int32_t bSuperUnique, int16_t nBossHcIdx, uint8_t* pUMods);
//D2Game.0x6FC6FDC0
UnitAny* __fastcall sub_6FC6FDC0(Game* pGame, Room1* pRoom, int32_t nX, int32_t nY, int32_t nClassId, int32_t nUnitGUID, uint8_t* pUMods);
//D2Game.0x6FC6FF10
void __fastcall D2GAME_BOSSES_AssignUMod_6FC6FF10(Game* pGame, UnitAny* pUnit, int32_t nUMod, int32_t bUnique);
//D2Game.0x6FC6FFD0
void __fastcall D2GAME_MONSTERS_Unk_6FC6FFD0(Game* pGame, UnitAny* pUnit, uint8_t nUMod);
