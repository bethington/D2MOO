#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: IsItemQualityMagicOrAbove

extern "C" int __stdcall IsItemQualityMagicOrAbove(void* pItem) {
    if (pItem == nullptr) return 0;
    int* p = (int*)pItem;
    if (p[0] != 4) return 0;
    int* pData = *(int**)((char*)pItem + 0x14);
    if (pData == nullptr) return 0;
    int quality = pData[0];
    if (quality < 4) return 0;
    if (quality > 9) return 0;
    return 1;
}
