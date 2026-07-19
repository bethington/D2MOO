#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: UNITS_HasEntryById

extern "C" uint8_t __stdcall UNITS_HasEntryById(void* pUnit)
{
    if (pUnit == nullptr) return 0;

    // NEG / SBB / NEG -> nonzero => 1, zero => 0 (bit-identical to the disassembly)
    typedef void* (__stdcall *UNITS_GetEntryValueById_t)(void*);
    UNITS_GetEntryValueById_t _f =
        (UNITS_GetEntryValueById_t)D2MOO_Resolve("UNITS_GetEntryValueById");
    if (_f == nullptr) return 0;

    void* _r = _f(pUnit);
    uint32_t _v = (uint32_t)(intptr_t)_r;
    return (_v != 0) ? (uint8_t)1 : (uint8_t)0;
}
