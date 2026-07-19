#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_CheckIsQuestType
extern "C" uint32_t __stdcall ITEMS_CheckIsQuestType(void* pUnit)
{
    if (pUnit == nullptr) return 0;

    typedef uint32_t (__stdcall *IsUnitInItemType_t)(void*, uint32_t);
    IsUnitInItemType_t _f = (IsUnitInItemType_t)D2MOO_Resolve("IsUnitInItemType");
    if (_f == nullptr) return 0;

    uint32_t _r = _f(pUnit, 0x35u);
    return _r;
}
