#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: UnpackAnimComponentFields
extern "C" uint32_t __stdcall UnpackAnimComponentFields(int nMissileId)
{
    void* pdt_addr = D2MOO_Resolve("g_pDataTables");
    if (!pdt_addr)
        return 0xDEADBEEFu; // resolver missing -> obvious mismatch sentinel

    char* base = *(char**)pdt_addr;
    if (!base)
        return 0xDEADBEEFu;

    int count = *(int*)(base + 0xBC0);
    if (nMissileId < 0 || nMissileId >= count)
        return 0;

    void* array = *(void**)(base + 0xBBC);
    if (!array)
        return 0;

    char* record = (char*)array + (unsigned)nMissileId * 0x84u;
    return *(uint32_t*)(record + 0x54);
}
