#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetUnitPathCoordY

extern "C" int __stdcall GetUnitPathCoordY(void* pUnit)
{
    if (pUnit == nullptr) {
        return 0;
    }
    
    unsigned int dwType = *(unsigned int*)((char*)pUnit + 0x00);
    
    if ((dwType != 2) && ((int)dwType < 4 || (int)dwType > 5)) {
        return *(int*)((*(int*)((char*)pUnit + 0x2c)) + 0xc);
    }
    return *(int*)((*(int*)((char*)pUnit + 0x2c)) + 8);
}
