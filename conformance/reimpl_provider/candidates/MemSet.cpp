#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: MemSet
extern "C" void* __stdcall MemSet(void* pBuffer, uint32_t dwBufferSize, uint8_t byFillByte)
{
    uint32_t dwFillPattern = (uint32_t)byFillByte * 0x01010101u;

    uint8_t* p = (uint8_t*)pBuffer;

    // Write dword-aligned chunks (size >> 2 iterations)
    uint32_t dwWordCount = dwBufferSize >> 2;
    while (dwWordCount != 0) {
        *(uint32_t*)p = dwFillPattern;
        p += 4;
        dwWordCount--;
    }

    // Write remaining 0-3 bytes (size & 3 iterations)
    uint32_t uVar1 = dwBufferSize & 3;
    while (uVar1 != 0) {
        *p = byFillByte;
        p += 1;
        uVar1--;
    }

    return (void*)dwFillPattern;
}
