#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ROOM_GetDrawLayerIndex
// Read BYTE at this+0x44 (zero-extended). Pure member access -- no global resolve.
// Caller passes `this` in ECX per __thiscall ABI (matches MSVC's D2Common member getter).
extern "C" uint32_t __fastcall ROOM_GetDrawLayerIndex(void* pThis)
{
    return *(uint8_t*)((char*)pThis + 0x44);
}
