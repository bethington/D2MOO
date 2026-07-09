#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetItemRecordFieldE4
// classId -> GetItemDataRecord + dword@0xE4
typedef void* (__stdcall* _rec_t)(unsigned int);
extern "C" unsigned int __stdcall ITEMS_GetItemRecordFieldE4(unsigned int dwClassId)
{
    _rec_t _f = (_rec_t)D2MOO_Resolve("GetItemDataRecord");
    if (_f == nullptr) return 0;
    char* rec = (char*)_f(dwClassId);
    if (rec == nullptr) return 0;
    return *(unsigned int*)(rec + 0xE4);
}
