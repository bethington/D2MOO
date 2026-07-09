#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetPathFieldByUnitType

extern "C" int __stdcall GetPathFieldByUnitType(void* pUnit)
{
    if (pUnit == nullptr) {
        return 0;
    }
    int* p = (int*)pUnit;
    if (((*p == 0) || (*p == 1)) && (p[0xc] != 0)) {
        return p[0x10];
    }
    return p[4];
}
