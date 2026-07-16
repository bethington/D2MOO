// D2MOO_REIMPL_EXPORT: SKILLS_GetSkillNodeField0x30
#include "../provider_runtime.h"
#include <stdlib.h>

extern "C" uint32_t __stdcall SKILLS_GetSkillNodeField0x30(void* pSkillNode)
{
    if (pSkillNode == (void*)0x0)
    {
        /* Original chain (all terminal, no return value):
             GetReturnAddress()           ; intrinsic, result used only as CleanupAndAbort log ctx
             CleanupAndAbort(0xda5, retaddr, 0x6fdda728)  ; fatal, does not return
             _exit(-1)                    ; process termination
           Inlined as a single _exit(-1): same observable behavior,
           same bit-exact termination. */
        _exit(-1);
    }
    /* MOV EAX,[EAX+0x30] — reuse pSkillNode as base, return DWORD at offset 0x30. */
    return *(const uint32_t*)((const uint8_t*)pSkillNode + 0x30);
}
