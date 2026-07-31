#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SetGameLogLevel
// NEEDS GLOBAL: g_nGameLogLevel
extern "C" void __fastcall SetGameLogLevel(int nLogLevel)
{
    int* pLevel = (int*)D2MOO_Resolve("g_nGameLogLevel");
    if (!pLevel)
        return;
    if ((-1 < nLogLevel) && (nLogLevel < 9)) {
        *pLevel = nLogLevel;
    }
}
