#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemTypeTxtRecord

/* DATATBLS_GetItemTypeTxtRecord
   
   Retrieves an ItemType.txt record by item type ID.
   
   Source: ..\Source\D2Common\DATATBLS\DataTbls.cpp
   
   Algorithm:
   1. Validate that nItemTypeId is non-negative and less than nItemPropertyTableCount
   2. If valid, calculate record address: base + (index * 0x3c)
   3. Return pointer to record, or NULL if out of bounds
   
   Parameters:
     nItemTypeId: int - Item type ID (index into ItemType.txt) [IMPLICIT EAX register]
   
   Returns:
     void*: Pointer to ItemType.txt record, or NULL if invalid index
   
   Special Cases:
     - Negative indices return NULL
     - Indices >= nItemPropertyTableCount return NULL
     - Resolver returns null: returns NULL
   
   Structure Layout (relative to g_pDataTables):
     +0xbc  | 4    | nItemPropertyTableBase  | ptr   | Base pointer to ItemType.txt records
     +0xc4  | 4    | nItemPropertyTableCount | int   | Number of records in table */
extern "C" void* __fastcall DATATBLS_GetItemTypeTxtRecord(int nItemTypeId)
{
    // STEP 1: Resolve the base pointer (g_pDataTables is a pointer variable, deref once)
    void* resolved = D2MOO_Resolve("g_pDataTables");
    if (!resolved)
        return 0; // resolver not injected / name unknown -> NULL
    
    char* base = (char*)*(void**)resolved;
    
    // STEP 2: Translate decompile literally
    if ((-1 < nItemTypeId) && (nItemTypeId < *(int*)(base + 0xc4))) {
        return (void*)(nItemTypeId * 0x3c + *(int*)(base + 0xbc));
    }
    return 0;
}
