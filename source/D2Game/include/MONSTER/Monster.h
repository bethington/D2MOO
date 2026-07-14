#pragma once

#include <Units/Units.h>
#include "MonsterRegion.h"


//D2Game.0x6FC601C0
void __fastcall MONSTER_SetAiState(UnitAny* pMonster, int32_t nAiState);
//D2Game.0x6FC601E0
int32_t __fastcall MONSTER_GetAiState(UnitAny* pMonster);
//D2Game.0x6FC60200
void __fastcall MONSTER_SetLevelId(UnitAny* pMonster, int32_t nLevelId);
//D2Game.0x6FC60220
int32_t __fastcall MONSTER_GetLevelId(UnitAny* pMonster);
//D2Game.0x6FC60240
int32_t __fastcall MONSTER_CheckSummonerFlag(UnitAny* pMonster, uint16_t nFlag);
//D2Game.0x6FC60270
void __fastcall MONSTER_ToggleSummonerFlag(UnitAny* pMonster, uint16_t nFlag, int32_t bSet);
//D2Game.0x6FC602A0
void __fastcall MONSTER_Initialize(Game* pGame, Room1* pRoom, UnitAny* pMonster, int32_t nUnitGUID);
//D2Game.0x6FC603D0
void __fastcall MONSTER_InitializeStatsAndSkills(Game* pGame, Room1* pRoom, UnitAny* pUnit, MonRegData* pMonRegData);
//D2Game.0x6FC60B10
int32_t __fastcall MONSTER_SetVelocityAndPosition(Game* pGame, UnitAny* pMonster, int32_t nX, int32_t nY, int32_t a5);
//D2Game.0x6FC60BC0
void __fastcall MONSTER_RemoveAll(Game* pGame);
//D2Game.0x6FC60C10
void __fastcall MONSTER_Free(Game* pGame, UnitAny* pMonster);
//D2Game.0x6FC60CD0
void __fastcall MONSTER_UpdateAiCallbackEvent(Game* pGame, UnitAny* pMonster);
//D2Game.0x6FC60E50
void __fastcall MONSTER_DeleteEvents(Game* pGame, UnitAny* pMonster);
//D2Game.0x6FC60E70
int32_t __fastcall MONSTER_GetHpBonus(int32_t nPlayerCount);
//Inlined in D2Game.0x6FC60E90
int32_t __fastcall MONSTER_GetExperienceBonus(int32_t nPlayerCount);
//D2Game.0x6FC60E90
void __fastcall MONSTER_GetPlayerCountBonus(Game* pGame, PlayerCountBonus* pPlayerCountBonus, Room1* pRoom, UnitAny* pMonster);
//D2Game.0x6FC60F70
void __fastcall MONSTER_SetComponents(MonRegData* pMonRegData, UnitAny* pUnit);
//D2Game.0x6FC610C0
int32_t __fastcall MONSTER_HasComponents(UnitAny* pMonster);
//D2Game.0x6FC610F0
int32_t __fastcall MONSTER_Reinitialize(Game* pGame, UnitAny* pUnit, int32_t nClassId, int32_t nMode);
