#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_FindQuestDataByUnitId
extern "C" uint32_t __fastcall CLIENT_FindQuestDataByUnitId(uint32_t dwUnitId)
{
    void* resolved = D2MOO_Resolve("g_pRosterPetList");
    if (!resolved)
        return 0xDEADBEEF; // wrong-value sentinel
    
    char* pRVar1 = *(char**)resolved;
    
    while (pRVar1 != NULL)
    {
        if (*(uint32_t*)(pRVar1 + 0x08) == dwUnitId)
            break;
        pRVar1 = *(char**)(pRVar1 + 0x30);
    }
    
    if (pRVar1 == NULL)
        return 0xFFFFFFFF;
    
    return *(uint32_t*)(pRVar1 + 0x0C);
}
