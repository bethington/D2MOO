#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetUltraOrBaseCode
// pUnit(item) -> record dwUltraCode(0x90) if set else dwItemCode(0x80)
typedef void* (__stdcall* _rec_t)(unsigned int);
extern "C" unsigned int __stdcall ITEMS_GetUltraOrBaseCode(void* pUnit)
{
    if (pUnit == nullptr || *(unsigned int*)pUnit != 4) return 0;
    _rec_t _f = (_rec_t)D2MOO_Resolve("GetItemDataRecord");
    if (_f == nullptr) return 0;
    char* rec = (char*)_f(*(unsigned int*)((char*)pUnit + 0x4));
    if (rec == nullptr) return 0;
    unsigned int ultra = *(unsigned int*)(rec + 0x90);
    return ultra != 0 ? ultra : *(unsigned int*)(rec + 0x80);
}
