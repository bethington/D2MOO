#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetItemRecordField106
// [abi_static] DELEGATE call-through (no model): resolves + calls GetItemDataRecord.
typedef void* (__stdcall *_callee_t)(unsigned int);
extern "C" unsigned short __stdcall ITEMS_GetItemRecordField106(void* p)
{
    if (p == nullptr) return 0x64;
    char* r = (char*)p;
    if (*(unsigned int*)(r + 0x0) != 0x4u) return 0x64;
    unsigned int _arg = *(unsigned int*)(r + 0x4);
    _callee_t _f = (_callee_t)D2MOO_Resolve("GetItemDataRecord");
    if (_f == nullptr) return 0x64;
    char* _rec = (char*)_f(_arg);
    if (_rec == nullptr) return 0x64;
    unsigned short _v = *(unsigned short*)(_rec + 0x106);
    return (_v == 0x1) ? (unsigned short)0x64 : _v;
}
