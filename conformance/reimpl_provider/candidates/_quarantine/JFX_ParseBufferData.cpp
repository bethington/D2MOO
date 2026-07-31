// JFX_ParseBufferData reimpl -- ijl11.dll 0x6001b510
// Parses JFX buffer data and validates elements against a validation table.
// All four params are pointers; pbBuffer / pContext / pValidationTable feed
// the algorithm, pResults is the outbuf (count, pElements, byte2, byte3,
// nibble4, nibble5).
//
// Struct layouts inferred from the decompile (offsets are ground truth):
//   JFX_ParseElement (stride 0x18):
//     +0x00  uint dwTableIndex
//     +0x04  uint dwField04
//     +0x08  uint dwField08
//     +0x0C  void* pField0C
//     +0x10  void* pField10
//     +0x14  void* pField14
//   ParseBufferTable:
//     +0x00  uint nCount
//     +0x04  void* pEntries   (array of 0x10-byte records: {ID, +4, +8, +0xc type})
//   JFX_ParseBufferDataCtx:
//     +0x94    base for type*0xB4
//     +0x364   base for nibble2*0x670
//     +0x1d24  base for nibble1*0x670

#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: JFX_ParseBufferData

extern "C" int JFX_GetBufferSize(uint8_t* pbBuffer, int* outSize);
extern "C" int JFX_AppendBufferData(void* pCtx, uint32_t size,
                                    void** outCleanup,
                                    uint8_t** outCursor,
                                    uint32_t* outCount);
extern "C" void ROOM_DeleteTileState(void* pTileState);
extern "C" void* operator_new(uint32_t size);

extern "C" int __cdecl JFX_ParseBufferData(
    void* pContext,
    uint8_t* pbBuffer,
    void* pValidationTable,
    uint32_t* pResults)
{
    uint32_t dwBufferSize = 0;
    int nResult = JFX_GetBufferSize(pbBuffer, (int*)&dwBufferSize);
    if (nResult != 0)
        return nResult;

    if (dwBufferSize < 2)
        return -0x15;

    dwBufferSize -= 2;

    void* pCleanupData = (void*)0;
    uint8_t* pbDataCursor = (uint8_t*)0;
    uint32_t dwElementCount = 0;

    nResult = JFX_AppendBufferData((void*)pbBuffer, dwBufferSize,
                                    &pCleanupData, &pbDataCursor,
                                    &dwElementCount);
    if (nResult != 0)
        goto cleanup;

    // Element count = first byte at the cursor returned by AppendBufferData
    uint8_t bBufferByte = *pbDataCursor;
    pbDataCursor = pbDataCursor + 1;
    pResults[0] = (uint32_t)bBufferByte;

    // Element array (stride 0x18)
    void* pElementArray = operator_new((uint32_t)bBufferByte * 0x18u);
    pResults[0xd] = (uint32_t)pElementArray;

    if (pElementArray == (void*)0) {
        nResult = -5;
        goto cleanup;
    }

    // Validation table: { uint nCount; void* pEntries; }
    uint32_t nCount = *(uint32_t*)pValidationTable;
    uint8_t* pEntries = *(uint8_t**)((char*)pValidationTable + 4);

    int nElementIndex;
    uint8_t* pCurrent = (uint8_t*)pElementArray;
    for (nElementIndex = 0;
         nElementIndex < (int)(uint32_t)bBufferByte;
         nElementIndex = nElementIndex + 1) {

        uint32_t dwElementId = (uint32_t)*pbDataCursor;
        uint32_t dwNibble1  = (uint32_t)(pbDataCursor[1] >> 4);
        uint32_t dwNibble2  = (uint32_t)(pbDataCursor[1] & 0xF);
        pbDataCursor = pbDataCursor + 2;

        // Nibble validation: nibble1 must be 4 OR <=3; nibble2 must be <=4
        if ((dwNibble1 != 4 && dwNibble1 > 3) || (dwNibble2 > 4))
            goto cleanup_invalid;

        // Linear search in validation table for matching ID (0x10-byte stride)
        int nSearchIndex = 0;
        while (nCount > (uint32_t)nSearchIndex) {
            if (dwElementId == *(uint32_t*)(pEntries + nSearchIndex * 0x10))
                break;
            nSearchIndex = nSearchIndex + 1;
        }
        if ((int)nCount <= nSearchIndex)
            goto cleanup_invalid;

        // Type field check: entries[i].type must be 0..4 (signed check)
        int iVar3 = nSearchIndex * 0x10;
        int typeField = *(int*)(pEntries + iVar3 + 0xC);
        if (typeField < 0 || typeField > 4)
            goto cleanup_invalid;

        // Store 6 DWORDs for this element
        *(uint32_t*)pCurrent              = (uint32_t)nSearchIndex;
        *(uint32_t*)(pCurrent + 0x04)     = *(uint32_t*)(pEntries + iVar3 + 0x04);
        *(uint32_t*)(pCurrent + 0x08)     = *(uint32_t*)(pEntries + iVar3 + 0x08);
        *(uint32_t*)(pCurrent + 0x0C)     =
            (uint32_t)((char*)pContext + 0x1d24 + dwNibble1 * 0x670);
        *(uint32_t*)(pCurrent + 0x10)     =
            (uint32_t)((char*)pContext + 0x364 + dwNibble2 * 0x670);
        *(uint32_t*)(pCurrent + 0x14)     =
            (uint32_t)((char*)pContext + 0x94 + typeField * 0xB4);

        pCurrent = pCurrent + 0x18;
    }

    // 3 trailing bytes (checksum / flags)
    pResults[2] = (uint32_t)*pbDataCursor;
    pResults[3] = (uint32_t)pbDataCursor[1];
    pResults[4] = (uint32_t)(pbDataCursor[2] >> 4);
    pResults[5] = (uint32_t)(pbDataCursor[2] & 0xF);

cleanup:
    if (dwElementCount != 0 && pCleanupData != (void*)0) {
        ROOM_DeleteTileState(pCleanupData);
    }
    return nResult;

cleanup_invalid:
    ROOM_DeleteTileState((void*)pResults[0xd]);
    pResults[0xd] = 0;
    nResult = -0x15;
    goto cleanup;
}
