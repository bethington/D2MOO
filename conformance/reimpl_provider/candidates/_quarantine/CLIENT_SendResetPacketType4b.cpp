#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_SendResetPacketType4b
// NEEDS GLOBAL: cRam6fbc971f
// NEEDS GLOBAL: cRam6fbc9720

extern "C" void __stdcall ProcessGameEndCleanup(void);
extern "C" void __stdcall CLIENT_ResetGameStateGlobals(void);
extern "C" void __stdcall CLIENT_SendChatMessageThrottled(char* msg, uint32_t len);

extern "C" uint32_t __stdcall CLIENT_SendResetPacketType4b(void)
{
    char* res_pPartyInviteDialog = (char*)D2MOO_Resolve("g_pPartyInviteDialog_6fbc9763");
    if (!res_pPartyInviteDialog)
        return 0;
    char* res_bExpansionVersionCheck = (char*)D2MOO_Resolve("g_bExpansionVersionCheck");
    if (!res_bExpansionVersionCheck)
        return 0;
    char* res_bExpansionVersionCheck_1 = (char*)D2MOO_Resolve("g_bExpansionVersionCheck_1");
    if (!res_bExpansionVersionCheck_1)
        return 0;
    char* res_cRam6fbc971f = (char*)D2MOO_Resolve("cRam6fbc971f");
    if (!res_cRam6fbc971f)
        return 0;
    char* res_cRam6fbc9720 = (char*)D2MOO_Resolve("cRam6fbc9720");
    if (!res_cRam6fbc9720)
        return 0;

    void* pPartyInviteDialog = *(void**)res_pPartyInviteDialog;

    if (pPartyInviteDialog != (void*)0x0) {
        ProcessGameEndCleanup();
        *(void**)res_pPartyInviteDialog = (void*)0x0;
    }

    CLIENT_ResetGameStateGlobals();

    char szResetMessage[9];
    szResetMessage[0] = '8';
    szResetMessage[1] = '\0';
    szResetMessage[2] = '\0';
    szResetMessage[3] = '\0';
    szResetMessage[4] = '\0';
    szResetMessage[5] = *res_bExpansionVersionCheck;
    szResetMessage[6] = *res_bExpansionVersionCheck_1;
    szResetMessage[7] = *res_cRam6fbc971f;
    szResetMessage[8] = *res_cRam6fbc9720;

    CLIENT_SendChatMessageThrottled(szResetMessage, 0xD);
    return 1;
}
