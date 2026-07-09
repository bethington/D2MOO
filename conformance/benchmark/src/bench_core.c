/* bench_core.c -- BENCHMARK GROUND TRUTH (Phase 1 core).
 *
 * These are functions we FULLY understand because we authored them. Compiled to a
 * stripped 32-bit DLL, loaded fresh into Ghidra (auto-analysis only), and fed BLIND to
 * the documentation pipeline. The pipeline's output is scored against answer_key.json,
 * whose content is exactly the semantics encoded here. This is the objective measure of
 * "how true is the generated documentation" -- because we know the answer.
 *
 * Realism: natural alignment (like the real game), a mix of exported-by-ordinal and one
 * un-exported internal (so the pipeline also faces a bare FUN_ target), a global data
 * table, and a delegate call-through -- the shapes our translators claim to handle.
 *
 * Struct field offsets are stated in comments AND mirrored in answer_key.json; the
 * chosen field widths make natural-alignment offsets deterministic.
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

/* A creature's live stats block. */
typedef struct StatBlock {      /* size 0x0C */
    u32 dwExperience;           /* +0x00 accumulated experience points          */
    u16 wLife;                  /* +0x04 current life                           */
    u16 wMaxLife;               /* +0x06 maximum life                           */
    u8  bLevel;                 /* +0x08 character/monster level                */
    /* +0x09..0x0B tail padding */
} StatBlock;

/* A game unit (monster or player). */
typedef struct BenchUnit {      /* size 0x0C */
    u32 dwType;                 /* +0x00 unit type: 1=monster, 2=player         */
    u32 dwUnitId;               /* +0x04 unique runtime id                      */
    StatBlock* pStats;          /* +0x08 pointer to the unit's live stats       */
} BenchUnit;

/* A static monster-data record (one row of a data table). */
typedef struct MonsterRec {     /* size 0x08 */
    u32 dwFlags;                /* +0x00 behavior flag bits                     */
    u8  bSpeed;                 /* +0x04 base movement speed                    */
    u8  bSizeX;                 /* +0x05 collision size                         */
    u16 wSoundId;               /* +0x06 attack sound id                        */
} MonsterRec;

/* The monster data table header (a global the game populates at load). */
typedef struct MonsterTable {   /* size 0x08 */
    int         nRecordCount;   /* +0x00 number of records                      */
    MonsterRec* pRecords;       /* +0x04 base of the record array               */
} MonsterTable;

/* Global monster table pointer -- populated when data tables load. */
MonsterTable* g_pMonsterTable = 0;


/* [ORDINAL 10001] bench_GetUnitLife
 * Reads the unit's current life. Two-level: load the stats pointer at unit+0x08,
 * then read the u16 life field at stats+0x04. No type gate.
 * Returns 0 if the unit or its stats pointer is null. */
u16 __stdcall bench_GetUnitLife(BenchUnit* pUnit)
{
    if (pUnit == 0 || pUnit->pStats == 0) return 0;
    return pUnit->pStats->wLife;
}

/* [ORDINAL 10042] bench_GetPlayerMaxLife
 * Returns the maximum life, but ONLY for player units (dwType == 2). Type-gated
 * two-level getter: gate on unit+0x00 == 2, load stats at unit+0x08, read the u16
 * max-life at stats+0x06. Returns 0 for non-players or null pointers. */
u16 __stdcall bench_GetPlayerMaxLife(BenchUnit* pUnit)
{
    if (pUnit == 0 || pUnit->dwType != 2 || pUnit->pStats == 0) return 0;
    return pUnit->pStats->wMaxLife;
}

/* [ORDINAL 10108] bench_GetMonsterSpeed
 * Global-table indexed getter: bounds-check nIndex against g_pMonsterTable->nRecordCount,
 * index the record array (stride 0x08) and return the u8 speed at record+0x04.
 * Returns 0 for an out-of-range index. */
u8 __stdcall bench_GetMonsterSpeed(int nIndex)
{
    if (nIndex < 0 || nIndex >= g_pMonsterTable->nRecordCount) return 0;
    return g_pMonsterTable->pRecords[nIndex].bSpeed;
}

/* bench_LookupMonster (INTERNAL -- deliberately NOT exported, so the pipeline sees a
 * bare FUN_ callee). Bounds-checked lookup returning a record pointer or null. */
static MonsterRec* bench_LookupMonster(int nIndex)
{
    if (nIndex < 0 || g_pMonsterTable == 0 || nIndex >= g_pMonsterTable->nRecordCount)
        return 0;
    return &g_pMonsterTable->pRecords[nIndex];
}

/* [ORDINAL 10205] bench_GetMonsterSound
 * Delegate call-through: resolves the record via bench_LookupMonster(nIndex), then reads
 * the u16 sound id at record+0x06. Returns 0xFFFF when the lookup fails. */
u16 __stdcall bench_GetMonsterSound(int nIndex)
{
    MonsterRec* pRec = bench_LookupMonster(nIndex);
    if (pRec == 0) return 0xFFFF;
    return pRec->wSoundId;
}

/* [ORDINAL 10333] bench_ClampValue
 * Pure computed (non-getter): clamps nValue to the inclusive range [nLo, nHi].
 * No memory access -- exercises documentation of arithmetic/branch logic, not a field read. */
int __stdcall bench_ClampValue(int nValue, int nLo, int nHi)
{
    if (nValue < nLo) return nLo;
    if (nValue > nHi) return nHi;
    return nValue;
}

/* keep the internal helper referenced so it survives as a real (un-exported) function */
MonsterRec* (*const _bench_keep)(int) = bench_LookupMonster;
