#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: PATH_GetUnitPathModeByte

extern "C" unsigned int __stdcall PATH_GetUnitPathModeByte(void* pUnit)
{
    if (pUnit == nullptr) {
        return 0;
    }
    
    // Read dwType at offset 0x00
    int dwType = *(int*)((char*)pUnit + 0x0);
    
    // Read pPosition (field11_0x2c) at offset 0x2C
    void* pPosition = *(void**)((char*)pUnit + 0x2C);
    
    if (dwType != 2 && dwType != 4) {
        // Not Object (2) and not Item (4)
        if (pPosition == nullptr) {
            return 0;
        }
        // Return byte at pPosition + 0x64
        return *(unsigned char*)((char*)pPosition + 0x64);
    }
    
    // Object or Item: return byte at pPosition + 0x1C
    return *(unsigned char*)((char*)pPosition + 0x1C);
}
