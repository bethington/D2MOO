#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_CompleteNpcInteractionAndReset
extern "C" {
    void ProcessGameEndCleanup();
    void FindAndValidateD2ExpMpq();
    void UpdateTimeoutValue();
    void CLIENT_HandleScreenStateTransition();
    void CLIENT_InitializeGameView();
}

extern "C" int __stdcall CLIENT_CompleteNpcInteractionAndReset(void)
{
    char* pPID         = (char*)D2MOO_Resolve("g_pPartyInviteDialog_6fbc9763");
    if (!pPID) return 0;
    char* pUIPanels    = (char*)D2MOO_Resolve("g_dwUIStatePanelsArr");
    if (!pUIPanels) return 0;
    char* pPanelCnt    = (char*)D2MOO_Resolve("g_dwUiPanelCountExpected");
    if (!pPanelCnt) return 0;
    char* pFadeEnd     = (char*)D2MOO_Resolve("g_dwFadeEndTime_6fb8d5d4");
    if (!pFadeEnd) return 0;
    char* pPfnTC       = (char*)D2MOO_Resolve("g_pfnGetTickCount");
    if (!pPfnTC) return 0;
    char* pViewportX   = (char*)D2MOO_Resolve("g_dwViewportX");
    if (!pViewportX) return 0;
    char* pUIStateA    = (char*)D2MOO_Resolve("g_dwUIState_6fb90eac");
    if (!pUIStateA) return 0;
    char* pCursorXBelt = (char*)D2MOO_Resolve("g_dwCursorScreenX_Belt");
    if (!pCursorXBelt) return 0;
    char* pCursorY2    = (char*)D2MOO_Resolve("g_dwCursorScreenY_2");
    if (!pCursorY2) return 0;
    char* pCursorX     = (char*)D2MOO_Resolve("g_dwCursorScreenX");
    if (!pCursorX) return 0;
    char* pCursorY     = (char*)D2MOO_Resolve("g_dwCursorScreenY");
    if (!pCursorY) return 0;
    char* pGameCfg     = (char*)D2MOO_Resolve("g_pGameStateConfig");
    if (!pGameCfg) return 0;
    char* pUIFlags     = (char*)D2MOO_Resolve("g_dwUIStateFlags");
    if (!pUIFlags) return 0;
    char* pSelItem     = (char*)D2MOO_Resolve("g_pSelectedItem");
    if (!pSelItem) return 0;
    char* pSelEnt      = (char*)D2MOO_Resolve("g_pSelectedEntity");
    if (!pSelEnt) return 0;
    char* pIntMode     = (char*)D2MOO_Resolve("g_dwInteractionMode");
    if (!pIntMode) return 0;
    char* pNpcInt      = (char*)D2MOO_Resolve("g_dwNpcInteractionState");
    if (!pNpcInt) return 0;
    char* pPendDlg     = (char*)D2MOO_Resolve("g_dwPendingDialogFlag");
    if (!pPendDlg) return 0;
    char* pSavedUI     = (char*)D2MOO_Resolve("g_dwSavedUIState");
    if (!pSavedUI) return 0;
    char* pExpVer      = (char*)D2MOO_Resolve("g_bExpansionVersionCheck");
    if (!pExpVer) return 0;

    uint32_t nPrevDataState;

    if (*(void**)pPID != (void*)0) {
        ProcessGameEndCleanup();
        *(void**)pPID = (void*)0;
    }
    nPrevDataState = *(uint32_t*)(pUIPanels + 0x24);
    FindAndValidateD2ExpMpq();
    if (*(uint32_t*)pPanelCnt != 0x26u) {
        return 0;
    }
    *(uint32_t*)(pUIPanels + 0x24) = 0u;
    if (nPrevDataState != 0u) {
        UpdateTimeoutValue();
        CLIENT_HandleScreenStateTransition();
    }
    if (*(uint32_t*)pFadeEnd != 0u) {
        void* pfn = *(void**)pPfnTC;
        uint32_t tc = ((uint32_t (*)(void))pfn)();
        *(uint32_t*)pPfnTC = tc + 100u;
    }
    *(uint32_t*)(pViewportX + 0u)  = 0xFFFFFFFFu;
    *(uint32_t*)pUIStateA          = 0xFFFFFFFFu;
    *(uint32_t*)pCursorXBelt       = 0xFFFFFFFFu;
    *(uint32_t*)pCursorY2          = 0xFFFFFFFFu;
    *(uint32_t*)pCursorX           = 0xFFFFFFFFu;
    *(uint32_t*)pCursorY           = 0xFFFFFFFFu;
    *(void**)pGameCfg                  = (void*)0;
    *(uint32_t*)pUIFlags           = 0u;
    *(void**)pSelItem                  = (void*)0;
    *(void**)pSelEnt                   = (void*)0;
    *(uint32_t*)pIntMode           = 7u;
    *(uint32_t*)pNpcInt            = 0u;
    *(uint32_t*)pPendDlg           = 1u;
    *(uint32_t*)pSavedUI           = (uint32_t)*(uint8_t*)pExpVer;
    CLIENT_InitializeGameView();
    return 1;
}
