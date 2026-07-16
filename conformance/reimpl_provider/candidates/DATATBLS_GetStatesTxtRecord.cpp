#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetStatesTxtRecord

/* DATATBLS_GetStatesTxtRecord

   Retrieves a States.txt record by state ID.

   NOTE 2026-07-14: originally proven under the name DATATBLS_GetItemTypeTxtRecord;
   the name-audit (phase 4) established by code that g_pDataTables +0xbc/+0xc4
   with stride 0x3c is the STATES table, and the Ghidra function at 0x6fd51770
   was renamed accordingly. The proof body is unchanged and remains valid.

   Algorithm:
   1. Validate that nStateId is non-negative and less than the States record count
   2. If valid, calculate record address: base + (index * 0x3c)
   3. Return pointer to record, or NULL if out of bounds

   Parameters:
     nStateId: int - State ID (index into States.txt) [IMPLICIT EAX register]

   Returns:
     void*: Pointer to States.txt record, or NULL if invalid index

   Special Cases:
     - Negative indices return NULL
     - Indices >= record count return NULL
     - Resolver returns null: returns NULL

   Structure Layout (relative to g_pDataTables):
     +0xbc  | 4    | pStatesTxt        | ptr   | Base pointer to States.txt records
     +0xc4  | 4    | nStatesTxtCount   | int   | Number of records in table */
extern "C" void* __fastcall DATATBLS_GetStatesTxtRecord(int nStateId)
{
    // STEP 1: Resolve the base pointer (g_pDataTables is a pointer variable, deref once)
    void* resolved = D2MOO_Resolve("g_pDataTables");
    if (!resolved)
        return 0; // resolver not injected / name unknown -> NULL

    char* base = (char*)*(void**)resolved;

    // STEP 2: Translate decompile literally
    if ((-1 < nStateId) && (nStateId < *(int*)(base + 0xc4))) {
        return (void*)(nStateId * 0x3c + *(int*)(base + 0xbc));
    }
    return 0;
}
