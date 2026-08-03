#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: InitializeServerSocketDialog
extern "C" uint32_t __stdcall InitializeServerSocketDialog(void)
{
    // g_dwServerSocketDialogType = 4
    char* pType = (char*)D2MOO_Resolve("g_dwServerSocketDialogType");
    if (!pType) return 0u;
    *(uint32_t*)pType = 4u;
    
    // g_pfnServerSocketDialogHandler = CLIENT_CompleteNpcInteractionAndReset (0x6FAFBD00)
    char* pHandler = (char*)D2MOO_Resolve("g_pfnServerSocketDialogHandler");
    if (!pHandler) return 0u;
    *(void**)pHandler = (void*)0x6FAFBD00;
    
    // g_wServerSocketDialogResourceId = 0xFB1
    char* pRes = (char*)D2MOO_Resolve("g_wServerSocketDialogResourceId");
    if (!pRes) return 0u;
    *(uint16_t*)pRes = (uint16_t)0xFB1;
    
    return 1u;
}
