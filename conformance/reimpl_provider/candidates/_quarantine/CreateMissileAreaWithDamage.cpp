#include "../provider_runtime.h"

// Forward declarations for external game functions called by the original
extern "C" int Unwind_6fd179c0(void*, ...);
extern "C" void* FindUnitByTypeAndId(int, uint32_t);
extern "C" void MISSILE_CalculateElementalDamageStats(int);
extern "C" void ApplyAreaEffectToUnits(void*, void*, int, int, uint32_t, void*);

// D2MOO_REIMPL_EXPORT: CreateMissileAreaWithDamage
extern "C" uint32_t __fastcall CreateMissileAreaWithDamage(
    int nOwnerType,            // ECX
    void* pMissileData,        // EDX
    int nDamage)               // Stack (callee-cleans 4 bytes)
{
    // Resolve the DataTables pointer (dereferenced once -- g_p* is a pointer variable)
    char* dt = *(char**)D2MOO_Resolve("g_pDataTables");
    if (!dt)
        return 0; // resolver missing -> obvious mismatch sentinel

    if (!pMissileData)
        return 1;

    // dwOwnerParam at offset 0x00 in MissileDataTbl (Ghidra shows as pMissileData->dwOwnerParam)
    uint32_t uVar1 = *(uint32_t*)((char*)pMissileData + 0x00);

    // Validate missile ID in range
    if ((int)uVar1 < 0)
        return 1;
    int missilesTxtCount = *(int*)(dt + 0xb6c);
    if ((int)uVar1 >= missilesTxtCount)
        return 1;

    // Compute missile record address: missilesTxtBase + id * 0x1A4
    int missilesTxtBase = *(int*)(dt + 0xb64);
    int nMissileIdx = (int)uVar1 * 0x1A4 + missilesTxtBase;
    if (nMissileIdx == 0)
        return 1;

    // Validation flag: pMissileData[1].nField3C (Ghidra treats sizeof=0 -> offset 0x3C), bit 10
    uint32_t validationFlag = *(uint32_t*)((char*)pMissileData + 0x3C);
    if (((validationFlag >> 10) & 1) == 0)
        return 1;

    // Owner unit ID: pMissileData[1].dwField0C (offset 0x0C with sizeof=0 quirk)
    uint32_t ownerUnitId = *(uint32_t*)((char*)pMissileData + 0x0C);
    void* pOwnerUnit = FindUnitByTypeAndId(nOwnerType, ownerUnitId);
    if (!pOwnerUnit)
        return 1;

    // Read nAreaRadius from missile txt record (+0x4C); compute if < 1
    int nAreaRadius = *(int*)(nMissileIdx + 0x4C);
    if (nAreaRadius < 1) {
        int nMissileTblIdx = Unwind_6fd179c0(pMissileData);
        uint32_t uVar3 = (uint32_t)Unwind_6fd179c0(pMissileData);
        if (nMissileTblIdx < 0)
            return 1;
        int missileTblCount = *(int*)(dt + 0xba0);
        if (missileTblCount <= nMissileTblIdx)
            return 1;
        int missileTblBase = *(int*)(dt + 0xb98);
        int nMissileTblOffset = nMissileTblIdx * 0x23C + missileTblBase;
        if (nMissileTblOffset == 0)
            return 1;
        nAreaRadius = Unwind_6fd179c0(pOwnerUnit, *(void**)(nMissileTblOffset + 0x138), nMissileTblIdx, (void*)uVar3);
        if (nAreaRadius < 1)
            nAreaRadius = 1;
    }

    // Type dispatch for damage data lookup (pMissileData->dwType at offset 0x04)
    uint32_t uVar1b = *(uint32_t*)((char*)pMissileData + 0x04);
    void* pField2C = *(void**)((char*)pMissileData + 0x2C);

    uint32_t dwMissileData = 0;
    if ((uVar1b == 2) || ((3 < (int)uVar1b) && ((int)uVar1b < 6))) {
        // type 2 or 4-5: direct ptr read
        dwMissileData = *(uint32_t*)((char*)pField2C + 0x0C);
    } else if (pField2C == (void*)0x0) {
        dwMissileData = 0;
    } else {
        Unwind_6fd179c0((void*)0);
    }

    // Re-read type and ptr for second dispatch
    uVar1b = *(uint32_t*)((char*)pMissileData + 0x04);
    pField2C = *(void**)((char*)pMissileData + 0x2C);

    uint32_t uVar5 = 0;
    if ((uVar1b == 2) || ((3 < (int)uVar1b) && ((int)uVar1b < 6))) {
        uVar5 = *(uint32_t*)((char*)pField2C + 0x10);
    } else if (pField2C != (void*)0x0) {
        uint16_t uVar2 = (uint16_t)Unwind_6fd179c0((void*)0);
        uVar5 = (uint32_t)uVar2;
    }

    // Clear 28-entry damage output array (0x1c * 4 bytes = 0x70 bytes)
    uint32_t damageArr[0x1c];
    for (int i = 0; i < 0x1c; i++)
        damageArr[i] = 0;

    MISSILE_CalculateElementalDamageStats(nDamage);

    // OR flags from missile record: dwFieldA8 (+0xa8) uint, wFieldAC (+0xac) ushort
    damageArr[0] |= *(uint32_t*)(nMissileIdx + 0xa8);
    ((uint16_t*)damageArr)[2] |= *(uint16_t*)(nMissileIdx + 0xac);

    ApplyAreaEffectToUnits(pOwnerUnit, (void*)dwMissileData, (int)uVar5, nAreaRadius, 0x8583u, (void*)0x6fcc1210);

    return 1;
}
