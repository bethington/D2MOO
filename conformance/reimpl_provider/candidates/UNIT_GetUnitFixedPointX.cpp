// D2MOO_REIMPL_EXPORT: UNIT_GetUnitFixedPointX
#include "../provider_runtime.h"
#include <stdlib.h>

/*
  SHADOW reimpl of UNIT_GetUnitFixedPointX @ 0x6fd8b440
  Ground-truth disassembly (authoritative):
    MOV  EAX,[ESP+0x4]       ; EAX = pUnit
    TEST EAX,EAX
    JNZ  skip_abort
    PUSH 0x42F               ; error code
    CALL GetReturnAddress
    PUSH EAX
    PUSH 0x6fdda728
    CALL CleanupAndAbort
    ADD  ESP,0xC
    PUSH -1
    CALL _exit               ; does not return
  skip_abort:
    MOV  EAX,[EAX+0x34]      ; load DWORD at pUnit+0x34
    AND  EAX,0x2
    RET  0x4
*/

extern "C" uint32_t __stdcall UNIT_GetUnitFixedPointX(void* pUnit)
{
    if (pUnit == (void*)0) {
        /* Reimplement the abort path inline per house rules
           (no game callees by name/address).
           Original: CleanupAndAbort -> _exit(-1). */
        _exit(-1);
    }

    /* Load the DWORD at pUnit+0x34 and mask bit 0x2.
       This is bit-exact with:  MOV EAX,[EAX+0x34]; AND EAX,0x2 */
    uint32_t v = *(uint32_t*)((uint8_t*)pUnit + 0x34u);
    return v & 0x2u;
}
