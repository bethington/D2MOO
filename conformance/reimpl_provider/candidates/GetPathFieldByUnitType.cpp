#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetPathFieldByUnitType
extern "C" int __stdcall GetPathFieldByUnitType(void* pUnit)
{
    if (pUnit == nullptr) return 0;
    int type = *(int*)pUnit;
    if ((type == 0 || type == 1) && (*(int*)((char*)pUnit + 0x30) != 0)) {
        return *(int*)((char*)pUnit + 0x40);
    }
    return *(int*)((char*)pUnit + 0x10);
}
