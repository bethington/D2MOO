#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: GetItemLevelDataPtr
extern "C" int __stdcall GetItemLevelDataPtr(void* pUnit) {
    if (pUnit == nullptr) return 0;
    if (*(int*)pUnit != 4) return 0;
    int pTypeData = *(int*)((char*)pUnit + 0x14);
    if (pTypeData == 0) return 0;
    return pTypeData + 4;
}
