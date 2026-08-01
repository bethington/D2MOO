// D2MOO_REIMPL_EXPORT: SetMonsterExpansionFlag
// Configures expansion flags when transitioning acts.
// No parameters, no return value. Mutates two globals by design.

#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SetMonsterExpansionFlag
extern "C" void __stdcall SetMonsterExpansionFlag(void)
{
    // Resolve all three globals by verified name. The resolver returns
    // &g_<name>.
    void* pActPendingAddr = D2MOO_Resolve("g_dwActTransitionPending");
    void* pLangIdAddr     = D2MOO_Resolve("g_dwLanguageId");
    void* pRosterUnitAddr = D2MOO_Resolve("g_pRosterUnit");

    if (!pActPendingAddr || !pLangIdAddr || !pRosterUnitAddr)
        return; // resolver not injected / unknown name -> mismatch sentinel

    // g_pRosterUnit is a pointer variable; deref ONCE to get the base record.
    char* base = *(char**)pRosterUnitAddr;
    if (!base)
        return;

    // 1. _g_dwActTransitionPending = 1;
    *(int*)pActPendingAddr = 1;

    // 2. Read low byte of g_dwLanguageId, add 1 (int math), multiply by 0x400.
    int langByte = (int)(*(uint8_t*)pLangIdAddr);
    int newBits  = (langByte + 1) * 0x400;

    // 3. Read existing ushort at +0x1eb, mask with 0xE0FF to preserve
    //    bits 0-7 and 13-15 while clearing bits 8-12.
    uint16_t cur        = *(uint16_t*)(base + 0x1eb);
    int           preserved  = (int)(cur & 0xe0ff);

    // OR computed expansion bit position with preserved lower bits.
    int combined = newBits | preserved;

    // 4. Store back as ushort (truncates to 16 bits, exact match to decompile).
    *(uint16_t*)(base + 0x1eb) = (uint16_t)combined;
}
