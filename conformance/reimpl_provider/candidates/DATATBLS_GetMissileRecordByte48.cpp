#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetMissileRecordByte48
// [abi_static] GLOBAL-TABLE getter (no model): g_pDataTables->records[idx*0x84] + 0x48.
extern "C" unsigned int __stdcall DATATBLS_GetMissileRecordByte48(int idx)
{
    if (idx < 0) return 0x0;
    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 0x0;
    char* base = (char*)*(void**)_g;
    if (base == nullptr) return 0x0;
    if (idx >= *(int*)(base + 0xbc0)) return 0x0;
    char* records = (char*)*(void**)(base + 0xbbc);
    char* rec = records + (int)idx * 0x84;
    if (rec == nullptr) return 0x0;
    return (unsigned int)*(unsigned char*)(rec + 0x48);
}
