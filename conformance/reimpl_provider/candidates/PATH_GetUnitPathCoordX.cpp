// D2MOO_REIMPL_EXPORT: PATH_GetUnitPathCoordX
#include "../provider_runtime.h"

extern "C" int __stdcall PATH_GetUnitPathCoordX(void* pUnit)
{
    if (pUnit == nullptr) {
        return 0;
    }
    
    unsigned int type = *(unsigned int*)((char*)pUnit + 0);
    
    char* pPath = *(char**)((char*)pUnit + 0x2c);
    
    if ((type != 2) && ((type < 4) || (type > 5))) {
        return *(int*)(pPath + 8);
    }
    return *(int*)(pPath + 4);
}
