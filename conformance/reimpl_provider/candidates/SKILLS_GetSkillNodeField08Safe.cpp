// D2MOO_REIMPL_EXPORT: SKILLS_GetSkillNodeField08Safe
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall SKILLS_GetSkillNodeField08Safe(void* pSkillNode)
{
    if (pSkillNode == (void*)0x0) {
        return 0;
    }
    return *(uint32_t*)((uint32_t)pSkillNode + 0x8);
}
