// D2MOO_REIMPL_EXPORT: UNIT_GetObjectField0x168
#include "../provider_runtime.h"

struct UnitAny;

extern "C" uint32_t __stdcall UNIT_GetObjectField0x168(UnitAny* pUnit)
{
    if (pUnit == (UnitAny*)0x0) {
        /* Original: CleanupAndAbort(line=0xfd3, GetReturnAddress(), errcode=0x6fdda728)
           then _exit(-1). No decompiles provided for callees (GetReturnAddress,
           CleanupAndAbort, _exit); trigger an inline trap to abort the process. */
        __debugbreak();
    }
    if (*(uint32_t*)((uint8_t*)pUnit + 0x0) == 0x2) {
        /* dwType == Object (0x2): double-deref pObjectData union at +0x14,
           then read dword at +0x168. Matches:
             MOV EAX, [EAX+0x14]   ; ptrA = pUnit->pObjectData
             MOV ECX, [EAX]        ; ptrB = *ptrA (double-deref)
             MOV EAX, [ECX+0x168]  ; result = *(ptrB + 0x168) */
        uint32_t ptrA = *(uint32_t*)((uint8_t*)pUnit + 0x14);
        uint32_t ptrB = *(uint32_t*)ptrA;
        return *(uint32_t*)(ptrB + 0x168);
    }
    /* Not an Object unit type: return 0 (matches XOR EAX,EAX; RET 0x4). */
    return 0;
}
