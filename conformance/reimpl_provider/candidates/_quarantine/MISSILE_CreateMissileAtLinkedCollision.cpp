#include "../provider_runtime.h"

// External game functions referenced by this routine
extern "C" uint64_t Unwind_6fd179c0(void* pPath);
extern "C" void FindLinkedUnit(void* pUnit, int targetUnitId);
extern "C" void CreateMissileUnit(void* pGame, uint32_t* params);
extern "C" int MISSILE_ProcessObjectMissileCollision(void* pGame, void* pUnit);

// D2MOO_REIMPL_EXPORT: MISSILE_CreateMissileAtLinkedCollision
extern "C" int __fastcall MISSILE_CreateMissileAtLinkedCollision(void* pGame, void* pUnit)
{
    // g_pDataTables is a pointer variable -> deref resolved address once to get struct base
    char* base = (char*)*(void**)D2MOO_Resolve("g_pDataTables");
    if (!base) return -1; // resolver missing -> obvious mismatch sentinel

    if (pUnit == 0) return 2;

    uint32_t dwClassId = *(uint32_t*)((char*)pUnit + 4);
    int nMissileClassCount = *(int*)(base + 0xb6c);

    if ((int)dwClassId < 0) return 2;
    if ((int)dwClassId >= nMissileClassCount) return 2;

    // pMissileTxtRec = dwClassId * 0x1A4 + g_pDataTables->nMissileTxtBase
    uint32_t uMissileTxtBase = *(uint32_t*)(base + 0xb64);
    uint32_t pMissileTxtRec = dwClassId * 0x1A4u + uMissileTxtBase;
    if (pMissileTxtRec == 0) return 2;
    if (*(short*)(pMissileTxtRec + 0x18) < 0) return 2;

    // Unwind_6fd179c0((pUnit->field10_0x2c).pPath) -- pPath is first 4 bytes of 8-byte field at +0x2c
    void* pPath = *(void**)((char*)pUnit + 0x2c);
    uint64_t qwLinkedUnitInfo = Unwind_6fd179c0(pPath);

    if ((int)qwLinkedUnitInfo != 0) {
        // Zero 23 DWORDs (92 bytes) on the stack for CreateMissile_MissileParams
        uint32_t params[23];
        for (int i = 0; i < 23; i++) params[i] = 0;

        FindLinkedUnit(0, (int)(qwLinkedUnitInfo >> 0x20));
        Unwind_6fd179c0(pUnit);
        Unwind_6fd179c0(pUnit);
        CreateMissileUnit(pGame, params);
    }

    return MISSILE_ProcessObjectMissileCollision(pGame, pUnit);
}
