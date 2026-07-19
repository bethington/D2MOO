#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: IsMonsterInSpecialDeathMode

extern "C" uint32_t __stdcall IsMonsterInSpecialDeathMode(void* pUnit)
{
    if (pUnit == nullptr) return 0;

    char* p = (char*)pUnit;

    // dwType == 1 (MONSTER)
    if (*(uint32_t*)(p + 0x0) != 1u) return 0;

    // animMode == 0xC (DEATH)
    if (*(uint32_t*)(p + 0x10) != 0xCu) return 0;

    // CALL 0x6fd513b0 with EAX = [ESI+0x4]; no verified D2MOO name for
    // this callee (DATATBLS_GetMonSoundTxtRecord was rejected by the
    // GLOBAL-RESOLVE verifier). Inline conservatively: treat [ESI+0x4]
    // as the record pointer (the callee returns it unchanged or
    // validates non-null). If null -> return false.
    char* record = (char*)*(void**)(p + 0x4);
    if (record == nullptr) return 0;

    // Load bitmask table base from g_dat_6fdd90b0 (pointer variable -> deref once)
    void* _g = D2MOO_Resolve("g_dat_6fdd90b0");
    if (_g == nullptr) return 0;
    char* bitmask = (char*)*(void**)_g;

    // (bitmask[0xC] & record[6]) must equal 0  ->  TEST [ECX+0xC], DL / JNZ false
    {
        uint8_t dl = *(uint8_t*)(record + 0x6);
        uint8_t bit = *(uint8_t*)(bitmask + 0xC);
        if ((bit & dl) != 0u) return 0;
    }

    // (bitmask[0x1C] & record[4]) must NOT equal 0  ->  TEST [ECX+0x1C], AL / JZ false
    {
        uint8_t al = *(uint8_t*)(record + 0x4);
        uint8_t bit = *(uint8_t*)(bitmask + 0x1C);
        if ((bit & al) == 0u) return 0;
    }

    // CALL MISSILES_TestTblMaskField_150(pUnit) -- verified D2MOO call-through.
    // Disasm: PUSH ESI / CALL / RET 0x4  =>  __stdcall with 1 stack arg.
    typedef uint32_t (__stdcall *TestTblMaskField_150_t)(void*);
    TestTblMaskField_150_t _f = (TestTblMaskField_150_t)D2MOO_Resolve("MISSILES_TestTblMaskField_150");
    if (_f == nullptr) return 0;
    uint32_t result = _f(p);

    // NEG EAX; SBB EAX,EAX; INC EAX  =>  return 1 iff prev result == 0
    return (result == 0u) ? 1u : 0u;
}
