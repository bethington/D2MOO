#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemRecordByte135
// pUnit(item) -> GetItemDataRecord + byte@0x135
typedef void* (__stdcall* _rec_t)(unsigned int);
extern "C" unsigned int __stdcall DATATBLS_GetItemRecordByte135(void* pUnit)
{
    if (pUnit == nullptr || *(unsigned int*)pUnit != 4) return 0;
    _rec_t _f = (_rec_t)D2MOO_Resolve("GetItemDataRecord");
    if (_f == nullptr) return 0;
    char* rec = (char*)_f(*(unsigned int*)((char*)pUnit + 0x4));
    if (rec == nullptr) return 0;
    return (unsigned int)*(unsigned char*)(rec + 0x135);
}
