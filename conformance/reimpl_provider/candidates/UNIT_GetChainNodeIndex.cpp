#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: UNIT_GetChainNodeIndex
extern "C" unsigned int __stdcall UNIT_GetChainNodeIndex(void* pUnit)
{
    if (pUnit == nullptr) {
        return 0;
    }
    
    // pPath = *(pUnit + 0x10)
    unsigned int* pPath = *(unsigned int**)((char*)pUnit + 0x10);
    
    // ECX = *(pPath + 0x2c)
    unsigned int expectedPath = *(unsigned int*)((char*)pPath + 0x2c);
    
    // ESI = *(pUnit + 0x24) - this is the loop limit
    unsigned int chainLimit = *(unsigned int*)((char*)pUnit + 0x24);
    
    if (chainLimit != expectedPath) {
        return 0;
    }
    
    if (chainLimit == 0) {
        return 1;
    }
    
    // EAX = 1 (return value on success)
    unsigned int result = 1;
    
    // EDX = *(pUnit) - chain pointer base
    unsigned int chainPtr = *(unsigned int*)pUnit;
    
    unsigned int counter = 0;
    
    while (1) {
        // EDI = *(EDX) - load chain node pointer
        unsigned int chainNode = *(unsigned int*)chainPtr;
        
        // Check flag byte at [chainNode + 0x34], bit 0
        if ((*(unsigned char*)((char*)chainNode + 0x34) & 1) == 0) {
            result = 0;
            break;
        }
        
        counter++;
        chainPtr += 4;
        
        if (counter >= chainLimit) {
            break;
        }
    }
    
    return result;
}
