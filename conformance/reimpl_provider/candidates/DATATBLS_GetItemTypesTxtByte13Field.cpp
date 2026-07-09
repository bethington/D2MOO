#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemTypesTxtByte13Field
// scalar nItemType -> ItemTypes[idx]+0x13
extern "C" unsigned int __stdcall DATATBLS_GetItemTypesTxtByte13Field(int nItemType)
{
    if (nItemType < 0) return 0;
    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 0;
    char* dt = (char*)*(void**)_g;
    if (dt == nullptr) return 0;
    if (nItemType >= *(int*)(dt + 0xbfc)) return 0;
    char* rec = (char*)*(void**)(dt + 0xbf8) + nItemType * 0xe4;
    return (unsigned int)*(unsigned char*)(rec + 0x13);
}
