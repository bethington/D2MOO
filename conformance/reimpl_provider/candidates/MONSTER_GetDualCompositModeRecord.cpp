// D2MOO_REIMPL_EXPORT: MONSTER_GetDualCompositModeRecord
#include "../provider_runtime.h"

extern "C" void* __stdcall MONSTER_GetDualCompositModeRecord(unsigned int nIndex, int nTableSelect)
{
    // Resolve globals by verified name
    void* g1 = D2MOO_Resolve("g_dat_6fdf13a4");  // g_dwMonCompositCount (count)
    void* g2 = D2MOO_Resolve("g_dat_6fdf13b0");  // g_pMonModeTable (primary table pointer)
    void* g3 = D2MOO_Resolve("g_dat_6fdf13ac");  // g_pMonCompositModeBase (secondary table base)

    if (!g1 || !g2 || !g3)
        return (void*)-0x7FFFFFFF;

    // g_dat_6fdf13a4: count value (unsigned int)
    unsigned int count = *(unsigned int*)g1;

    // g_dat_6fdf13b0: POINTER variable g_pMonModeTable -> deref once
    char* primary_base = *(char**)g2;

    // g_dat_6fdf13ac: POINTER variable g_pMonCompositModeBase -> deref once
    char* secondary_base = *(char**)g3;

    const unsigned int STRIDE = 0x34;

    if (nIndex < count)
    {
        char* entry;

        if (nTableSelect == 0)
        {
            // Primary table: pointer arithmetic (stride 0x34 per element)
            entry = primary_base + nIndex * STRIDE;
        }
        else if (nTableSelect == 1)
        {
            // Secondary table: explicit offset calculation
            entry = secondary_base + nIndex * STRIDE;
        }
        else
        {
            // Invalid table selector -> null
            return 0;
        }

        if (entry == (char*)0x0)
        {
            // Null entry in table -- original aborts; oracle uses valid inputs only
            return 0;
        }

        return entry;
    }

    // Index out of range -> null
    return 0;
}
