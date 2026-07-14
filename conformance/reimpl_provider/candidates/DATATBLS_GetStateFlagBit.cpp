#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: DATATBLS_GetStateFlagBit

extern "C" int __stdcall DATATBLS_GetStateFlagBit(void* pItem)
{
    if (pItem == nullptr) return 0;

    // dwType at offset 0x0 must equal 4 (ITEM unit type)
    if (*(int*)((char*)pItem + 0x0) != 4) return 0xff;

    // Resolve DATATBLS_GetItemTypeFromUnit (stdcall, 1 stack arg pushed via PUSH EAX; result in EAX)
    typedef void* (__stdcall *DATATBLS_GetItemTypeFromUnit_t)(uint32_t);
    DATATBLS_GetItemTypeFromUnit_t _f = (DATATBLS_GetItemTypeFromUnit_t)D2MOO_Resolve("DATATBLS_GetItemTypeFromUnit");
    if (_f == nullptr) return 0xff;
    int itemTypeIndex = (int)_f((uint32_t)pItem);

    // JL 0x6fd734de -- must be >= 0
    if (itemTypeIndex < 0) return 0xff;

    // Resolve g_pDataTables (g_p* is a POINTER variable -> deref once for base struct)
    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 0xff;
    char* base = (char*)*(void**)_g;
    if (base == nullptr) return 0xff;

    // CMP EAX,[ECX+0xbfc] / JGE skip -- bounds-check itemTypeIndex vs record count
    if (itemTypeIndex >= *(int*)(base + 0xbfc)) return 0xff;

    // MOV EDX,[ECX+0xbf8] -- item types record base
    char* recordBase = *(char**)(base + 0xbf8);
    if (recordBase == nullptr) return 0xff;

    // IMUL EAX,EAX,0xe4 ; ADD EAX,EDX -- record stride 0xE4 = 228
    char* record = recordBase + itemTypeIndex * 0xe4;

    // MOV AL, byte ptr [EAX+0x22]
    uint8_t val = *(uint8_t*)(record + 0x22);

    // CMP AL,0xff ; JNZ ret -- 0xFF sentinel => invalid
    if (val == 0xff) return 0xff;

    return (int)val;
}
