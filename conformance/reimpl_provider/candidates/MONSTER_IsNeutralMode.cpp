#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: MONSTER_IsNeutralMode
extern "C" int __stdcall MONSTER_IsNeutralMode(void* pUnit) {
    if (pUnit == nullptr) return 0;
    if (*(int*)((char*)pUnit + 0x0) != 1) return 0;
    int dwMode = *(int*)((char*)pUnit + 0x10);
    if (dwMode == 0 || dwMode == 0xC) return 1;
    return 0;
}
