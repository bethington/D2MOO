#pragma once

#include <Units/Units.h>


//D2Game.0x6FC7EA50
void __fastcall PLAYERSTATS_SetStatsForStartingAct(Game* pGame, UnitAny* pUnit, uint8_t nAct);
//D2Game.0x6FC7EC00
void __fastcall PLAYERSTATS_LevelUp(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC7EDE0
void __fastcall PLAYERSTATS_OnPlayerLeveledUp(Game* pGame, UnitAny* pUnit, void* pLeveledUpPlayer);
//D2Game.0x6FC7EDF0
int32_t __fastcall PLAYERSTATS_SpendStatPoint(UnitAny* pUnit, int32_t nStatId);
