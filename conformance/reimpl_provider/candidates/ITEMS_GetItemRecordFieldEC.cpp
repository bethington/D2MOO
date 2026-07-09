#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetItemRecordFieldEC
// scalar classId -> GetItemDataRecord + dword@0xEC (orig indexes g_pItemRecords stride 0x1A8;
// abort path unreachable on real classIds). prove_spec fast-path.
typedef void* (__stdcall* _rec_t)(unsigned int);
extern "C" unsigned int __stdcall ITEMS_GetItemRecordFieldEC(unsigned int dwClassId)
{
    _rec_t _f = (_rec_t)D2MOO_Resolve("GetItemDataRecord");
    if (_f == nullptr) return 0;
    char* rec = (char*)_f(dwClassId);
    if (rec == nullptr) return 0;
    return *(unsigned int*)(rec + 0xEC);
}
