#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetItemRecordByte137
// classId -> GetItemDataRecord + byte@0x137 (orig indexes g_pItemRecords; via resolvable record getter)
typedef void* (__stdcall* _rec_t)(unsigned int);
extern "C" unsigned int __stdcall ITEMS_GetItemRecordByte137(unsigned int dwClassId)
{
    _rec_t _f = (_rec_t)D2MOO_Resolve("GetItemDataRecord");
    if (_f == nullptr) return 0;
    char* rec = (char*)_f(dwClassId);
    if (rec == nullptr) return 0;
    return (unsigned int)*(unsigned char*)(rec + 0x137);
}
