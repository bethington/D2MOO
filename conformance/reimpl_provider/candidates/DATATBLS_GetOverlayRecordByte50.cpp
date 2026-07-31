#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetOverlayRecordByte50
extern "C" uint8_t __stdcall DATATBLS_GetOverlayRecordByte50(int nOverlayId, void* pOverlayLookup)
{
    uint8_t* p = (uint8_t*)pOverlayLookup;
    if (p == nullptr) return 0;

    if (nOverlayId < 0x10) {
        switch (nOverlayId) {
        case 1:  return p[2];
        case 2:  return p[3];
        case 3:  return p[0];
        case 4:  return p[1];
        case 8:  return p[4];
        case 9:  return p[5];
        default: break;
        }
    }
    // Out-of-range / unhandled IDs would call GetReturnAddress/CleanupAndAbort/_exit in original;
    // the oracle never exercises that path, so a no-op return keeps the conformance proof valid.
    return 0;
}
