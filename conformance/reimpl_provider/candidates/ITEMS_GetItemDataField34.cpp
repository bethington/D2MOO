// D2MOO_REIMPL_EXPORT: ITEMS_GetItemDataField34
#include "../provider_runtime.h"

/*
 * ITEMS_GetItemDataField34 (Ordinal 10433, addr 0x6fd73990)
 * Reads uint16 at item_data+0x34 (wRareSuffix) from a type-4 (Item) UnitAny.
 * Bit-exact port of:
 *   MOV  EAX,[ESP+4]   ; pUnit
 *   TEST EAX,EAX       ; JZ fail
 *   CMP  [EAX],4       ; JNZ fail
 *   MOV  EAX,[EAX+14h] ; pItemData
 *   TEST EAX,EAX       ; JZ fail
 *   MOV  AX,[EAX+34h]  ; wRareSuffix (16-bit)
 *   RET  4
 *   fail: XOR AX,AX    ; preserves upper 16 bits of EAX in non-zero pUnit/dwType!=4 case
 *   RET  4
 *
 * The success path returns EAX = (pItemData & 0xFFFF0000) | wRareSuffix
 * because MOV AX only writes the low 16 of EAX; upper bits remain the pointer's upper bits.
 * Failure path returns 0 (if EAX was 0 going in) or pUnit & 0xFFFF0000 (if jump came after the
 * dwType check, with EAX still holding pUnit).
 */
extern "C" uint32_t __stdcall ITEMS_GetItemDataField34(void* pUnit)
{
    uint32_t eax = (uint32_t)(uintptr_t)pUnit;

    if (eax != 0u && (*(uint32_t*)(uintptr_t)eax) == 4u) {
        eax = *(uint32_t*)(uintptr_t)(eax + 0x14u);
        if (eax != 0u) {
            uint16_t ax = *(uint16_t*)(uintptr_t)(eax + 0x34u);
            /* preserve upper 16 bits (pointer bits), per MOV AX semantics */
            return (eax & 0xFFFF0000u) | (uint32_t)ax;
        }
    }

    /* XOR AX, AX failure path: zero low 16 of EAX, keep upper 16 */
    return eax & 0xFFFF0000u;
}
