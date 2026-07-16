// D2MOO_REIMPL_EXPORT: ROOM_GetRoomDataByIndex
#include "../provider_runtime.h"

extern "C" void* __cdecl ROOM_GetRoomDataByIndex(
    void* pRoom,    // EAX  -- DrlgRoom*
    int ebx_unused, // EBX  -- unused in algorithm (LEA EBX,[EBX] padding only)
    int nLvlPrestId // EDX  -- level preset ID (0-based index into LvlPrest table)
) {
    // Step 1: Get override list head from pRoom->pOverrideList (+0x90)
    int* node = *(int**)((char*)pRoom + 0x90);

    // Step 2: Walk override linked list
    while (node != nullptr) {
        // node->dwTileIndex at +0x00
        int dwTileIndex = *node;

        // If zero, original calls CleanupAndAbort (does not return).
        // Port as spin since we cannot call any game function and have no decompile.
        if (dwTileIndex == 0) {
            volatile int abort_spin = 0;
            while (abort_spin == 0) { /* matches "does not return" */ }
            return nullptr; // unreachable
        }

        // Match check against nLvlPrestId (EDX)
        if (nLvlPrestId == dwTileIndex) {
            // Found: return node + 0x24 (24 bytes into associated record data)
            return (char*)node + 0x24;
        }

        // Advance to next node at +0x44 (per ground-truth disassembly)
        node = *(int**)((char*)node + 0x44);
    }

    // Step 3: List exhausted or empty -- validate nLvlPrestId
    if (nLvlPrestId < 0) {
        return (void*)0x68; // dummy
    }

    // nLvlPrestCount lives at g_pDataTables + 0xc5c
    void* pDataTables = D2MOO_Resolve("g_pDataTables");
    int nLvlPrestCount = *(int*)((char*)pDataTables + 0xc5c);

    if (nLvlPrestId >= nLvlPrestCount) {
        return (void*)0x68; // dummy
    }

    // Step 4: Valid id -- return &g_dat_6fdf0b38[nLvlPrestId] + 0x68
    void* pTable = D2MOO_Resolve("g_dat_6fdf0b38");
    char* pEntry = (char*)pTable + (nLvlPrestId * 0x9c); // 0x9C = 156-byte LvlPrest stride
    return (char*)pEntry + 0x68;
}
