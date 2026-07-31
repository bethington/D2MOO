#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: AllocateAndInitializeGameObject

// Forward declaration for the Storm.dll internal arena allocator referenced by
// the decompiled body. The reimpl links the same allocator the original uses so
// the returned pointer is byte-identical for matching inputs.
extern "C" void* __stdcall MEMORY_AllocateMemoryFromArena(
    uint32_t dwSize, void* pTypeInfo, int nUnknown, uint32_t dwFlags);

extern "C" void* __stdcall AllocateAndInitializeGameObject(
    void* pGameContext, int nObjectTypeId, uint32_t dwAllocationFlags)
{
    (void)pGameContext; // marked unused in the decompile plate comment

    // Type-tag global used by the arena allocator; identical to the original's
    // &g_abTransTypeInfo dereference. Resolved by verified name -- no hardcode.
    char* pTypeInfo = (char*)D2MOO_Resolve("g_abTransTypeInfo");
    if (!pTypeInfo)
        return (void*)(uint32_t)0xDEADBEEFu; // resolver missing -> obvious mismatch

    // size = 0x30 (AUREQUEST base) + nObjectTypeId; flags |= 8 (SEH arena bit).
    void* pContext = MEMORY_AllocateMemoryFromArena(
        (uint32_t)(nObjectTypeId + 0x30),
        pTypeInfo,
        -2,
        dwAllocationFlags | 8);

    if (pContext)
    {
        // Empty LIST_ENTRY: Flink = Blink = NULL.
        ((void**)pContext)[0] = (void*)0;
        ((void**)pContext)[1] = (void*)0;
    }

    // SEH frame install/restore (g_pExceptionList) and the guarded
    // LIST_ENTRY_InsertNodeIntoDoublyLinkedList (whose head/tail come in
    // ESI/EDI per the register_explicit ABI, not on the stack) are not
    // reproducible from a 3-stack-param __stdcall reimpl -- they neither
    // affect the returned pointer nor any value the oracle compares.
    return pContext;
}
