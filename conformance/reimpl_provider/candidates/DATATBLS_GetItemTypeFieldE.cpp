#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemTypeFieldE
// [abi_static] GLOBAL-TABLE getter (no model): g_pDataTables->records[idx*0xe4] + 0xe.
extern "C" short __stdcall DATATBLS_GetItemTypeFieldE(int idx)
{
    if (idx < 0) return 0x0;
    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 0x0;
    char* base = (char*)*(void**)_g;
    if (base == nullptr) return 0x0;
    if (idx >= *(int*)(base + 0xbfc)) return 0x0;
    char* records = (char*)*(void**)(base + 0xbf8);
    char* rec = records + (int)idx * 0xe4;
    if (rec == nullptr) return 0x0;
    return *(short*)(rec + 0xe);
}
