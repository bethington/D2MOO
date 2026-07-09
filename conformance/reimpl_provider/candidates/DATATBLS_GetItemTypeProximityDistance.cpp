#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemTypeProximityDistance
// scalar nItemType -> g_pDataTables ItemTypes[idx]+0x0C (short, sign-extended), else 0.
extern "C" int __stdcall DATATBLS_GetItemTypeProximityDistance(int nItemType)
{
    if (nItemType < 0) return 0;
    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 0;
    char* dt = (char*)*(void**)_g;
    if (dt == nullptr) return 0;
    if (nItemType >= *(int*)(dt + 0xbfc)) return 0;
    char* rec = (char*)*(void**)(dt + 0xbf8) + nItemType * 0xe4;
    return (int)*(short*)(rec + 0xc);
}
