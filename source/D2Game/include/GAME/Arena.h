#pragma once

#include <Units/Units.h>

#pragma pack(1)


enum ArenaScoreTypes
{
    ARENASCORE_SUICIDE,
    ARENASCORE_PLAYERKILL,
    ARENASCORE_PLAYERKILLPERCENT,
    ARENASCORE_MONSTERKILL,
    ARENASCORE_PLAYERDEATH,
    ARENASCORE_PLAYERDEATHPERCENT,
    ARENASCORE_MONSTERDEATH,
    NUM_ARENA_SCORES,
};

struct Arena
{
    int32_t nAlternateStartTown;				//0x00
    int32_t nType;								//0x04
    uint32_t fFlags;							//0x08 GameFlags
    int32_t nTemplate;							//0x0C - uint8_t with 3 pad
};

struct ArenaUnit
{
    int32_t nScore;								//0x00
    BOOL bUpdateScore;						    //0x04 
};

#pragma pack()

//D2Game.0x6FC31010
int32_t __fastcall D2Game_10001_Return0();
//D2Game.0x6FC31040
void __fastcall ARENA_AllocArena(Game* pGame, int32_t nUnused, uint32_t nFlags, int32_t nTemplate);
//D2Game.0x6FC31090
void __fastcall ARENA_FreeArena(Game* pGame);
//D2Game.0x6FC310C0
void __fastcall ARENA_AllocArenaUnit(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC31110
void __fastcall ARENA_FreeArenaUnit(Game* pGame, UnitAny* pPlayer);
//D2Game.0x6FC31160
int32_t __fastcall ARENA_GetAlternateStartTown(Game* pGame);
//D2Game.0x6FC31190
void __fastcall ARENA_ProcessKillEvent(Game* pGame, UnitAny* pAttacker, UnitAny* pDefender);
//D2Game.0x6FC31280
void __fastcall ARENA_UpdateScore(Game* pGame, UnitAny* pAttacker, UnitAny* pDefender, ArenaScoreTypes eScore);
//D2Game.0x6FC31470
void __fastcall ARENA_SynchronizeWithClients(Game* pGame, GameClient* pClient);
//D2Game.0x6FC315C0
void __fastcall ARENA_SendScoresToClient(Game* pGame, GameClient* pClient);
//D2Game.0x6FC31690
uint32_t __fastcall ARENA_NeedsClientUpdate(Game* pGame);
//D2Game.0x6FC316D0
uint32_t __fastcall ARENA_IsInArenaMode(Game* pGame);
//D2Game.0x6FC31710
uint32_t __fastcall ARENA_IsActive(Game* pGame);
//D2Game.0x6FC31750
uint32_t __fastcall ARENA_GetFlags(Game* pGame);
//D2Game.0x6FC31780
int32_t __fastcall ARENA_Return0();
//1.10f: D2Game.0x6FC31790
//1.13c: D2Game.0x6FCD2620
uint32_t __fastcall ARENA_ShouldTreatClassIdAsTemplateId(Game* pGame);
//D2Game.0x6FC317C0
int32_t __fastcall ARENA_GetTemplateType(Game* pGame);
