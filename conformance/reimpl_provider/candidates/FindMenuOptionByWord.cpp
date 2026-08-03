#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: FindMenuOptionByWord
extern "C" uint32_t __fastcall FindMenuOptionByWord(uint16_t wNpcType)
{
    char* countBase = (char*)D2MOO_Resolve("g_dwDialogOptionCount");
    char* npcTypesBase = (char*)D2MOO_Resolve("g_wDialogOptionNpcType0");
    if (!countBase || !npcTypesBase)
        return 0xDEADBEEFu;

    uint32_t countFull = *(uint32_t*)countBase;
    uint16_t count = (uint16_t)countFull;

    uint32_t uVar1 = 0;
    if (count != 0) {
        do {
            if (*(uint16_t*)(npcTypesBase + uVar1 * 2u) == wNpcType) {
                return uVar1 & 0xFFFFFF00u;
            }
            uVar1 = uVar1 + 1u;
        } while ((int)uVar1 < (int)(uint32_t)count);
    }
    return (uVar1 & 0xFFFFFF00u) | 1u;
}
