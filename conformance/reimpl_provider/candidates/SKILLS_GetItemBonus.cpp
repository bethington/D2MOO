#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SKILLS_GetItemBonus
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter.
extern "C" unsigned int __stdcall SKILLS_GetItemBonus(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    r = *(char**)(r + 0x4);
    return *(unsigned int*)(r + 0x28);
}
