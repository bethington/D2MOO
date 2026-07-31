#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: IsRemoteSessionActive
extern "C" int __stdcall IsRemoteSessionActive(void)
{
    // Global g_dwRemoteSessionEnabled: a DWORD, !0 means remote-session flag is on.
    uint32_t* pEnabled = (uint32_t*)D2MOO_Resolve("g_dwRemoteSessionEnabled");
    if (!pEnabled)
        return -1; // resolver missing -> obvious mismatch sentinel

    // Global g_abVersionInfoBuffer: a byte buffer; the decompile reads
    //   *(int*)(&g_abVersionInfoBuffer + 0x10)  -> "_16_4_"
    //   *(int*)(&g_abVersionInfoBuffer + 0x04)  -> "_4_4_"
    char* base = (char*)D2MOO_Resolve("g_abVersionInfoBuffer");
    if (!base)
        return -1; // resolver missing -> obvious mismatch sentinel

    // Mirrors the decompiled boolean expression EXACTLY:
    //   g_dwRemoteSessionEnabled != 0
    //   && ( byte@0x10 == 1
    //        || (byte@0x10 == 2
    //            && (byte@0x04 == 4 || byte@0x04 == 5)) )
    if (*pEnabled != 0 &&
        (*(int*)(base + 0x10) == 1 ||
         (*(int*)(base + 0x10) == 2 &&
          (*(int*)(base + 0x04) == 4 || *(int*)(base + 0x04) == 5))))
    {
        return 1;
    }
    return 0;
}
