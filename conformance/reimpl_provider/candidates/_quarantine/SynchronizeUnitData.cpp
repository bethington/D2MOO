#include "../provider_runtime.h"

// Forward declarations for subroutines called by SynchronizeUnitData.
// These are real D2 functions defined elsewhere in the reimpl provider.
extern "C" void __stdcall InitializeUnitSeedSlots(int pPath, void* pSeedTemplate);
extern "C" void __stdcall ResizeAndCopyDataBuffer(void* pDataRecord);
extern "C" void __stdcall CopyDataArrayElements();

// D2MOO_REIMPL_EXPORT: SynchronizeUnitData
extern "C" void* __stdcall SynchronizeUnitData(void* pDest, void* pSource) {
    char* pD = (char*)pDest;
    char* pS = (char*)pSource;

    // Step 1: Compare &nAct (offset 0x18) -- if addresses differ, call InitializeUnitSeedSlots
    if ((void*)(pD + 0x18) != (void*)(pS + 0x18)) {
        InitializeUnitSeedSlots(
            *(int*)(pS + 0x28),       // (int)pSource->pPath
            *(void**)(pS + 0x2C)      // (SeedTemplate*)pSource->dwSeedTemplate
        );
    }

    // Step 2: Copy pDataBuffer (offset 0x30)
    *(void**)(pD + 0x30) = *(void**)(pS + 0x30);

    // Step 3: Compare &nField34 (offset 0x34) -- if addresses differ, call ResizeAndCopyDataBuffer
    if ((void*)(pD + 0x34) != (void*)(pS + 0x34)) {
        ResizeAndCopyDataBuffer(*(void**)(pS + 0x18));  // (DataRecord*)pSource->dwAct
    }

    // Step 4: Copy nField3C (offset 0x3C)
    *(uint32_t*)(pD + 0x3C) = *(uint32_t*)(pS + 0x3C);

    // Step 5: Compare &nField40 (offset 0x40) -- if addresses differ, call CopyDataArrayElements
    if ((void*)(pD + 0x40) != (void*)(pS + 0x40)) {
        CopyDataArrayElements();
    }

    // Step 6: Copy graphics block fields (offsets 0x4C, 0x50, 0x54)
    *(uint32_t*)(pD + 0x4C) = *(uint32_t*)(pS + 0x4C);
    *(uint32_t*)(pD + 0x50) = *(uint32_t*)(pS + 0x50);
    *(uint32_t*)(pD + 0x54) = *(uint32_t*)(pS + 0x54);

    // Step 7: Copy trailing fields (offsets 0x58, 0x5C, 0x60, 0x64)
    *(uint32_t*)(pD + 0x58) = *(uint32_t*)(pS + 0x58);
    *(uint32_t*)(pD + 0x5C) = *(uint32_t*)(pS + 0x5C);
    *(uint32_t*)(pD + 0x60) = *(uint32_t*)(pS + 0x60);
    *(uint32_t*)(pD + 0x64) = *(uint32_t*)(pS + 0x64);

    return pDest;
}
