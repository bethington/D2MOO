// D2MOO_REIMPL_EXPORT: MONSTER_IsNeutralMode
#include "../provider_runtime.h"

// Only the two fields this function reads are declared; layout proven
// from the disassembly (dwType at +0x00, dwMode at +0x10).
struct UnitAny {
    uint32_t dwType;     // +0x00
    uint8_t _pad[12];  // +0x04..+0x0F
    uint32_t dwMode;     // +0x10
};

extern "C" uint32_t __stdcall MONSTER_IsNeutralMode(UnitAny* pUnit)
{
    if (pUnit != (UnitAny*)0 &&
        pUnit->dwType == 1 &&
        (pUnit->dwMode == 0 || pUnit->dwMode == 0xC))
    {
        return 1;
    }
    return 0;
}
