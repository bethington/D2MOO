#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetUnitStatListField58
extern "C" uint32_t __stdcall GetUnitStatListField58(void* pUnit) {
    if (pUnit == nullptr) return 0;
    void* pStats = *(void**)((char*)pUnit + 0x5C);
    if (pStats == nullptr) return 0;
    int modifierNode = *(int*)((char*)pStats + 0x10);
    if (modifierNode < 0) {
        return *(uint32_t*)((char*)pStats + 0x58);
    }
    return 0;
}
