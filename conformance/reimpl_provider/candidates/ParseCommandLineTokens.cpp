#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ParseCommandLineTokens
// __fastcall: ECX=arg1 (ppszCommandString), EDX=arg2 (pnConfigArray), stack=[arg3,arg4,arg5]
// RET 0xc = 12 bytes = 3 stack params
extern "C" int __fastcall ParseCommandLineTokens(
    char **ppszCommandString,
    int *pnConfigArray,
    uint32_t *pdwConfigValue,
    int *pnReserved1,
    int *pnReserved2)
{
    // NEEDS GLOBAL: g_dwDialogLookupStyle100
    char* lookupBase = (char*)D2MOO_Resolve("g_dwDialogLookupStyle100");
    if (!lookupBase) return 0;

    // NEEDS GLOBAL: ExtractDelimitedToken
    // NEEDS GLOBAL: STORM_ValidateCommandToken
    typedef int (__stdcall *ExtractDelimitedTokenFn)(int*, char*, const char*, const char*, uint32_t*);
    typedef int (__stdcall *ValidateCmdFn)(void*, void*, uint32_t, int*, int*);

    ExtractDelimitedTokenFn pExtract = (ExtractDelimitedTokenFn)D2MOO_Resolve("ExtractDelimitedToken");
    ValidateCmdFn pValidate = (ValidateCmdFn)D2MOO_Resolve("STORM_ValidateCommandToken");

    if (!pExtract || !pValidate) return 0;

    char cVar1;
    int iVar2;
    char* local_108;
    uint32_t local_104;
    char local_100[256];

    cVar1 = **ppszCommandString;
    while (cVar1 != '\0') {
        local_108 = *ppszCommandString;
        local_104 = 0;
        pExtract((int*)&local_108, local_100, lookupBase, " ,;\"\t\n\r\x1a", &local_104);
        if ((local_100[0] != '\0') &&
           (iVar2 = pValidate((void*)pdwConfigValue, (void*)pnConfigArray, local_104, pnReserved1, pnReserved2), iVar2 == 0)) break;
        *ppszCommandString = local_108;
        cVar1 = *local_108;
    }
    return **ppszCommandString == '\0';
}
