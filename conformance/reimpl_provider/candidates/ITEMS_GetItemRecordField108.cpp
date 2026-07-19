#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetItemRecordField108
// [abi_static] DELEGATE call-through (no model): resolves + calls GetItemDataRecord.
typedef void* (__stdcall *_callee_t)(uint32_t);
extern "C" uint16_t __stdcall ITEMS_GetItemRecordField108(void* p)
{
    if (p == nullptr) return 0x64;
    char* r = (char*)p;
    if (*(uint32_t*)(r + 0x0) != 0x4u) return 0x64;
    uint32_t _arg = *(uint32_t*)(r + 0x4);
    _callee_t _f = (_callee_t)D2MOO_Resolve("GetItemDataRecord");
    if (_f == nullptr) return 0x64;
    char* _rec = (char*)_f(_arg);
    if (_rec == nullptr) return 0x64;
    uint16_t _v = *(uint16_t*)(_rec + 0x108);
    return (_v == 0x1) ? (uint16_t)0x64 : _v;
}
