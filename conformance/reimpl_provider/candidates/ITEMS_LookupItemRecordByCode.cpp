// D2MOO_REIMPL_EXPORT: ITEMS_LookupItemRecordByCode
#include "../provider_runtime.h"

// Delegate-rung reimpl: the original calls BinarySearchInSortedArray (a Fog.dll
// IAT thunk, plain __stdcall(pTable, pKey, nFlag) -- 3 stack args, callee-clean).
// The reimpl calls the SAME game helper through the resolver, so this proof covers
// THIS function's own logic (guard, index write, stride math); the Fog search is a
// shared dependency proven separately.
typedef int (__stdcall *BSearchFn)(void* pTable, void* pKey, int nFlag);

extern "C" void* __stdcall ITEMS_LookupItemRecordByCode(unsigned int* pItemCode, int* pIndex)
{
    BSearchFn bsearch_fn = (BSearchFn)D2MOO_Resolve("BinarySearchInSortedArray");
    void** ppTable   = (void**)D2MOO_Resolve("g_pItemDataBuffer");  // MOV ECX,[0x6fdeff6c] -> deref once
    void** ppRecords = (void**)D2MOO_Resolve("g_pItemRecords");     // [0x6fdefb98] -> deref once
    if (!bsearch_fn || !ppTable || !ppRecords)
        return (void*)0xBAADF00D; // resolver missing sentinel

    int nSearchResult = bsearch_fn(*ppTable, pItemCode, 0);
    if (nSearchResult < 0) {
        *pIndex = 0;
        return (void*)0;
    }
    *pIndex = nSearchResult;
    return (void*)((char*)*ppRecords + nSearchResult * 0x1a8);
}
