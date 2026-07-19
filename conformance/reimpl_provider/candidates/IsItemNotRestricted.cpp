#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: IsItemNotRestricted
extern "C" int __stdcall IsItemNotRestricted(void* pUnit)
{
    if (pUnit == nullptr) return 0;

    uint32_t dwType = *(uint32_t*)pUnit;
    if (dwType != 4) return 1;

    void* pItemData = *(void**)((char*)pUnit + 0x14);
    if (pItemData == nullptr) return 1;

    uint32_t dwFlags = *(uint32_t*)((char*)pItemData + 0x18);
    if ((dwFlags & 0x100) != 0) return 0;
    if ((dwFlags & 0x4000) != 0) return 0;

    return 1;
}
