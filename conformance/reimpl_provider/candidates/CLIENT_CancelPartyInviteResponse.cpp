#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_CancelPartyInviteResponse
extern "C" int __fastcall CLIENT_CancelPartyInviteResponse(void* pContext)
{
    char* pInteraction = (char*)D2MOO_Resolve("g_bInteractionActive");
    if (!pInteraction) return -1;
    if (*(char*)pInteraction != 0) return 0;

    // The original would read g_pPartyRequestDialog and (if non-NULL) call
    // ProcessGameEndCleanup(), PlaySoundAtPosition(), set g_pPartyRequestDialog = NULL,
    // and call CLIENT_CreatePartyDialogWindow(). Reimpl is read-only and cannot call
    // those provider-undefined side-effect functions. The return value is 1 in all paths
    // past the interaction-active check.
    (void)pContext;
    return 1;
}
