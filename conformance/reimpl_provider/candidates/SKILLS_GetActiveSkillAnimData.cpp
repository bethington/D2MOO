#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SKILLS_GetActiveSkillAnimData
//
// [RE-DRAFTED 2026-07-08] Prior reimpl computed a bogus nested offset off the
// pointer -> 99.56% shadow divergence. Ground truth (disasm @0x6fd80460):
// MOV EAX,[EAX+0xa8]; MOV EAX,[EAX+0x8] -> deref the pointer at +0xA8, then read
// the dword at +0x8 (the sibling STAT_GetActiveSkillFieldC reads +0xC of the same
// pointer). Single deref + fixed offset, not array indexing.
extern "C" void* __stdcall SKILLS_GetActiveSkillAnimData(void* pUnit)
{
    if (pUnit == nullptr) return nullptr;
    void* pStateLinkedList = *(void**)((char*)pUnit + 0xA8);
    if (pStateLinkedList == nullptr) return nullptr;  // original aborts; never hit
    return *(void**)((char*)pStateLinkedList + 0x8);
}
