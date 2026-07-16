// D2MOO_REIMPL_EXPORT: SKILLS_GetSkillNodePos1
#include "../provider_runtime.h"
#include <stdlib.h>

extern "C" uint32_t __stdcall SKILLS_GetSkillNodePos1(void* p)
{
    // Decompiled source has callees (GetReturnAddress / CleanupAndAbort / _exit)
    // with no provided decompile. The path terminates the process, matching
    // the original's _exit(-1) tail. abort() is the faithful inline equivalent.
    if (p == (void*)0x0) {
        abort(); /* does not return */
    }
    return *(uint32_t*)((uint32_t)p + 0x18);
}
