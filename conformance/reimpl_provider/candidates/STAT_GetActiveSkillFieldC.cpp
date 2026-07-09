#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: STAT_GetActiveSkillFieldC
//
// [RE-DRAFTED 2026-07-08] deref the pointer at +0xA8, read the dword at +0xC.
extern "C" unsigned int __stdcall STAT_GetActiveSkillFieldC(void* pUnit)
{
    if (pUnit == nullptr) return 0;
    void* pStateLinkedList = *(void**)((char*)pUnit + 0xA8);
    if (pStateLinkedList == nullptr) return 0;
    return *(unsigned int*)((char*)pStateLinkedList + 0xC);
}
