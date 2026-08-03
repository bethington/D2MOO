#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_InitializeNpcMenuStructures
extern "C" void __stdcall CLIENT_InitializeNpcMenuStructures(void)
{
    // NEEDS GLOBAL: CLIENT_InitializePartyInviteDialog

    // NPC Menu 1: type=2, status=0, callback=NULL
    *(int*)D2MOO_Resolve("g_dwAssertCallbackType") = 2;
    *(short*)D2MOO_Resolve("g_wNpcMenu1Status") = 0;
    *(void**)D2MOO_Resolve("g_pfnNpcMenu1Callback") = 0;

    // Dialog panel: type=2, status=0, callback=NULL
    *(int*)D2MOO_Resolve("g_dwDialogPanelState") = 2;
    *(short*)D2MOO_Resolve("g_wDialogPanelStatus") = 0;
    *(void**)D2MOO_Resolve("g_pfnDialogPanelCallback") = 0;

    // Char inv dialog: type=2, height (high word)=0, callback=NULL
    *(int*)D2MOO_Resolve("g_dwCharacterInventoryDialogId") = 2;
    *(short*)((char*)D2MOO_Resolve("g_dwCharacterInventoryDialogHeight") + 2) = 0;
    *(void**)D2MOO_Resolve("g_pfnCharInvDialogCallback") = 0;

    // Server socket: type=3, status (low word)=0, callback=NULL
    *(int*)D2MOO_Resolve("g_dwServerSocketDialogType") = 3;
    *(short*)D2MOO_Resolve("g_dwServerSocketStatus") = 0;
    *(void**)D2MOO_Resolve("g_pfnServerSocketCallback") = 0;

    // Panel A: type=2, status (low word)=0, callback=NULL
    *(int*)D2MOO_Resolve("g_dwPanelStateA") = 2;
    *(short*)D2MOO_Resolve("g_dwNpcPanelStatus") = 0;
    *(void**)D2MOO_Resolve("g_pfnPanelACallback") = 0;

    // Conditional override: if g_pfnCreatePartyStatsDialog == CreatePartyInviteDialog (0x6faf6d10)
    if (*(void**)D2MOO_Resolve("g_pfnCreatePartyStatsDialog") == (void*)0x6faf6d10) {
        *(int*)D2MOO_Resolve("g_dwPanelStateA") = 3;
    }

    // 4 UI state fields: zeroed
    *(short*)D2MOO_Resolve("g_wNpcMenuFieldA") = 0;
    *(int*)D2MOO_Resolve("g_dwNpcMenuFieldB") = 0;
    *(short*)D2MOO_Resolve("g_wNpcMenuFieldC") = 0;
    *(int*)D2MOO_Resolve("g_dwNpcMenuFieldD") = 0;

    // NPC menu 8: type=2, statusA=0, callbackA=NULL, statusB=0, callbackB=NULL
    *(int*)D2MOO_Resolve("g_dwNpcMenu8Type") = 2;
    *(short*)D2MOO_Resolve("g_wNpcMenu8StatusA") = 0;
    *(void**)D2MOO_Resolve("g_pfnNpcMenu8CallbackA") = 0;
    *(short*)D2MOO_Resolve("g_wNpcMenu8StatusB") = 0;
    *(void**)D2MOO_Resolve("g_pfnNpcMenu8CallbackB") = 0;

    // Special dialog state types
    *(int*)D2MOO_Resolve("g_dwServerDialogStateType") = 3;
    *(int*)D2MOO_Resolve("g_dwNpcMenuSpecialType") = 4;

    // Resolve trade-dialog callback function pointer
    void* pfn = (void*)D2MOO_Resolve("CLIENT_InitializePartyInviteDialog");

    // Trade entry 1: controlId=0xfb4, callback=pfn, type=3
    *(short*)D2MOO_Resolve("g_dwTradeEntry1ControlId") = 0xfb4;
    *(void**)D2MOO_Resolve("g_pfnTradeEntry1Callback") = pfn;
    *(int*)D2MOO_Resolve("g_dwTradeEntry1Type") = 3;

    // Trade entry 2
    *(short*)D2MOO_Resolve("g_wTradeEntry2ControlId") = 0xfb4;
    *(void**)D2MOO_Resolve("g_pfnTradeEntry2Callback") = pfn;
    *(int*)D2MOO_Resolve("g_dwTradeEntry2Type") = 3;

    // Trade entry 3
    *(short*)D2MOO_Resolve("g_wTradeEntry3ControlId") = 0xfb4;
    *(void**)D2MOO_Resolve("g_pfnTradeEntry3Callback") = pfn;
    *(int*)D2MOO_Resolve("g_dwTradeEntry3Type") = 3;

    // Trade entry 4
    *(short*)D2MOO_Resolve("g_wTradeEntry4ControlId") = 0xfb4;
    *(void**)D2MOO_Resolve("g_pfnTradeEntry4Callback") = pfn;
    *(int*)D2MOO_Resolve("g_dwTradeEntry4Type") = 3;

    // Trade entry 5
    *(short*)D2MOO_Resolve("g_wTradeEntry5ControlId") = 0xfb4;
    *(void**)D2MOO_Resolve("g_pfnTradeEntry5Callback") = pfn;
    *(int*)D2MOO_Resolve("g_dwTradeEntry5Type") = 3;
}
