#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_InitializeMenuOptionType2
extern "C" void __fastcall CLIENT_InitializeMenuOptionType2(int nEntryIndex)
{
    char* base_an = (char*)D2MOO_Resolve("g_anPartyEntryCount");
    char* base_aw = (char*)D2MOO_Resolve("g_awMenuOptionInit");
    char* base_dw = (char*)D2MOO_Resolve("g_dwPartyMenuOptionField04");

    if (!base_an || !base_aw || !base_dw)
        return;

    int offset = nEntryIndex * 0x27;
    *(int*)(base_an + offset) = 2;
    *(short*)(base_aw + offset) = 0;
    *(int*)(base_dw + offset) = 0;
}
