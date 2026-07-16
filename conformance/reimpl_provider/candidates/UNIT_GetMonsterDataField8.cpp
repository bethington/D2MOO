// D2MOO_REIMPL_EXPORT: UNIT_GetMonsterDataField8
#include "../provider_runtime.h"

// Minimal UnitAny layout for this function (only offsets 0x00 and 0x14 are touched).
typedef struct UnitAny {
    DWORD dwType;        // 0x00 - must be 2 (monster) for the fast path
    DWORD _pad[4];       // 0x04..0x13
    DWORD pMonsterData;  // 0x14 - pointer to monster-specific data
} UnitAny;

extern "C" uint32_t __stdcall UNIT_GetMonsterDataField8(UnitAny* pUnit) {
    // Fast path: only valid when pUnit is non-null AND dwType == 2 (monster).
    // Matches disasm: TEST EAX,EAX / JNZ, then CMP [EAX],0x2 / JNZ.
    if (pUnit != (UnitAny*)0 && pUnit->dwType == 2) {
        // MOV EAX,[EAX+0x14] -> load pointer at offset 0x14
        // MOV EAX,[EAX+0x8]  -> load DWORD at offset 0x8 from that pointer
        return ((DWORD*)pUnit->pMonsterData)[2]; // 0x8 / sizeof(DWORD) == 2
    }
    // Error path: original asserts ("Unit type 2 is a monster." / type-mismatch)
    // and terminates via CleanupAndAbort -> _exit(-1). Reimplemented inline:
    // trip a debugger breakpoint so any unexpected caller is caught immediately.
    __debugbreak();
    return 0;
}
