#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CODEC_GetEntityStateFields

// LIMITATION: IsHandleInCodecList (called by the original at the inner
// branch) is a Storm.dll internal function that the decompile names.
// Per the reimpl rules we may not call functions the decompile names
// (no forward declaration / no extern linking), and IsHandleInCodecList
// is not in the resolver. So the IsHandleInCodecList call is omitted.
//
// The reimpl therefore returns 0 for all non-NULL pEntity, which matches
// the original when the handle is NOT in the codec list. For test inputs
// where pEntity IS in the codec list, the original returns 1 with state
// fields populated; the reimpl returns 0 with defaults -- those cases will
// not prove. _g_dwLastError and the DAT_0004d892 thunk are likewise
// unreproducible (read-only rule + invalid absolute address).
//
// Per plate comment layout, the success path reads:
//   CodecContext + 0x58 -> pCodecBuffer
//   CodecBuffer  + 0x04 -> pStateBlock
//   StateBlock   + 0x04 -> dwStateValue1 (into *pField1Out)
//   StateBlock   + 0x08 -> dwStateValue2 (into *pField2Out)
// and returns 1 iff any of the three out-pointers is non-NULL.

extern "C" int __stdcall CODEC_GetEntityStateFields(
    void* pEntity,
    uint32_t* pField1Out,
    uint32_t* pField2Out,
    uint32_t* pField3Out)
{
    if (pField1Out) *pField1Out = 0;
    if (pField2Out) *pField2Out = 0;
    if (pField3Out) *pField3Out = 8;

    if (pEntity == 0) {
        // Original: _g_dwLastError = 0x57; (*(code *)&DAT_0004d892)(0x57);
        // Read-only rule: no global mutation; thunk address is a
        // decompile artifact and not callable. Fall through to return 0.
        return 0;
    }

    // IsHandleInCodecList((CodecNode*)pEntity) call omitted (decompile-named
    // function, not resolvable). Returning 0 here matches the original's
    // "handle not found in codec list" branch.
    return 0;
}
