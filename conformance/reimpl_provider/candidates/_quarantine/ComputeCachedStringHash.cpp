#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ComputeCachedStringHash

extern "C" uint32_t __stdcall STORM_ComputeStringHash(const char* pString, int nSomething, uint32_t dwFlags);

extern "C" uint32_t __stdcall ComputeCachedStringHash(
    void* pString,
    uint32_t dwHashFlags
)
{
    // NEEDS GLOBAL: g_dwCacheValidFlag
    uint32_t* pValidFlag = (uint32_t*)D2MOO_Resolve("g_dwCacheValidFlag");
    // NEEDS GLOBAL: g_pCachedStringPtr
    void** pCachedStringPtr = (void**)D2MOO_Resolve("g_pCachedStringPtr");
    // NEEDS GLOBAL: g_dwCachedHashFlags
    uint32_t* pCachedFlags = (uint32_t*)D2MOO_Resolve("g_dwCachedHashFlags");
    // NEEDS GLOBAL: g_dwCachedFirstDword
    uint32_t* pCachedFirstDword = (uint32_t*)D2MOO_Resolve("g_dwCachedFirstDword");
    // NEEDS GLOBAL: g_dwCachedHash
    uint32_t* pCachedHash = (uint32_t*)D2MOO_Resolve("g_dwCachedHash");

    if (!pValidFlag || !pCachedStringPtr || !pCachedFlags || !pCachedFirstDword || !pCachedHash)
        return 0; // resolver missing - obvious mismatch sentinel

    uint32_t dwHashResult = 0;
    uint32_t dwFirstDword;

    if (pString == (void*)0) {
        dwFirstDword = 0;
    } else {
        dwFirstDword = *(uint32_t*)pString;
    }

    if (((*pValidFlag != 0) && (pString == *pCachedStringPtr)) && (dwHashFlags == *pCachedFlags)) {
        if (dwFirstDword == *pCachedFirstDword) {
            return *pCachedHash;
        }
        *pValidFlag = 0;
    }

    if (pString != (void*)0) {
        dwHashResult = STORM_ComputeStringHash((const char*)pString, 1, dwHashFlags);
    }

    *pCachedHash = dwHashResult & 0x7FFFFFFFu;
    if (*pCachedHash == 0) {
        *pCachedHash = 1;
    }
    *pCachedFirstDword = dwFirstDword;
    *pCachedStringPtr = pString;
    *pCachedFlags = dwHashFlags;
    return *pCachedHash;
}
