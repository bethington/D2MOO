#pragma once

#include <Units/Units.h>
#include "MonsterRegion.h"

struct UnkMonCreate2;


//D2Game.0x6FC62020
void __stdcall sub_6FC62020(Seed* pSeed, MonRegData* pMonRegData, int32_t nCount);
//D2Game.0x6FC62420
int32_t __fastcall MONSTERCHOOSE_GetPresetMonsterId(Game* pGame, MonsterRegion* pMonsterRegion, Room1* pRoom, MonStatsTxt** ppMonStatsTxtRecord, uint8_t nChance, int32_t bSpawnUMon);
//D2Game.0x6FC62640
UnkMonCreate2* __fastcall sub_6FC62640(Room1* pRoom);
//D2Game.0x6FC62670
int32_t __fastcall MONSTERCHOOSE_GetBossSpawnType(MonsterRegion* pMonsterRegion, Room1* pRoom);
