#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemTypePropertyByte14
// pUnit -> ItemTypes[idx]+0x14, default 0
typedef int (__stdcall* _gt_t)(void*);
extern "C" unsigned int __stdcall DATATBLS_GetItemTypePropertyByte14(void* pUnit)
{
    if (pUnit != nullptr && *(unsigned int*)pUnit == 4) {
        _gt_t _gt = (_gt_t)D2MOO_Resolve("DATATBLS_GetItemTypeFromUnit");
        if (_gt != nullptr) {
            int idx = _gt(pUnit);
            if (idx >= 0) {
                void* _g = D2MOO_Resolve("g_pDataTables");
                if (_g != nullptr) {
                    char* dt = (char*)*(void**)_g;
                    if (dt != nullptr && idx < *(int*)(dt + 0xbfc)) {
                        char* rec = (char*)*(void**)(dt + 0xbf8) + idx * 0xe4;
                        return (unsigned int)*(unsigned char*)(rec + 0x14);
                    }
                }
            }
        }
    }
    return 0;
}
