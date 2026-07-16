// D2MOO_REIMPL_EXPORT: SKILLS_GetSkillDescriptionString
#include "../provider_runtime.h"
#include <stdlib.h>

/*
 * Shadow-dispatch reimpl of SKILLS_GetSkillDescriptionString @ 0x6fd8b1a0
 * ABI: __stdcall, 1 stack arg (pStruct at [ESP+0x4]), return in EAX, RET 0x4.
 *
 * Disassembly is authoritative; note the sub-struct pointer offset is +0x10
 * (not +0x0C as in the decompiled C source). This matches the live bytes:
 *     6fd8b1c7: MOV EAX,dword ptr [EAX + 0x10]
 *     6fd8b1ca: MOV EAX,dword ptr [EAX + 0x58]
 *     6fd8b1cd: ADD EAX,0x1e0
 */
extern "C" uint32_t __stdcall SKILLS_GetSkillDescriptionString(void* pStruct)
{
    if (pStruct == (void*)0) {
        /* NULL abort path: original calls CleanupAndAbort(0x6fdda728, line=0x5B9)
           then _exit(-1). CleanupAndAbort is a game function (not reimplemented
           here -- it terminates). Inlining _exit preserves the fatal semantics
           the real code relies on; with a live caller this branch is dead. */
        _exit(-1);
    }

    /* pStruct->pSubStruct at +0x10, then *(pSubStruct + 0x58) + 0x1E0 */
    uint32_t subStruct = *(uint32_t*)((uint8_t*)pStruct + 0x10u);
    return *(uint32_t*)(subStruct + 0x58u) + 0x1E0u;
}
