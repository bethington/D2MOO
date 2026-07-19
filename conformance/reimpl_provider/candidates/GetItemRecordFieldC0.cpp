#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetItemRecordFieldC0
// [abi_static] DELEGATE call-through (no model): resolves + calls GetItemDataRecord.
typedef void* (__stdcall *_callee_t)(uint32_t);
extern "C" uint32_t __stdcall GetItemRecordFieldC0(void* p)
{
    if (p == nullptr) return 0x0;
    char* r = (char*)p;
    if (*(uint32_t*)(r + 0x0) != 0x4u) return 0x0;
    uint32_t _arg = *(uint32_t*)(r + 0x4);
    _callee_t _f = (_callee_t)D2MOO_Resolve("GetItemDataRecord");
    if (_f == nullptr) return 0x0;
    char* _rec = (char*)_f(_arg);
    if (_rec == nullptr) return 0x0;
    uint32_t _v = *(uint32_t*)(_rec + 0xc0);
    return _v;
}
