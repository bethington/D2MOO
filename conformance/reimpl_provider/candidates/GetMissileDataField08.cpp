#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetMissileDataField08
extern "C" int __stdcall GetMissileDataField08(void* pUnit) {
    if (pUnit == nullptr) return 0;

    int dwType = *(int*)((char*)pUnit + 0x00);
    int pData  = *(int*)((char*)pUnit + 0x14);

    if (dwType == 3 && pData != 0) {
        // _padding sits at offset 0x04 in the missile-data struct, so
        // _padding + 4 == byte offset 0x08 from pData (the "Field08" the
        // function name advertises). The mechanical translation that took
        // _padding at offset 0 (=> offset 4) is wrong on a synthetic
        // object whose bytes at +4 and +8 differ.
        short val = *(short*)((char*)pData + 0x08);
        return (int)val;
    }
    return 0;
}
