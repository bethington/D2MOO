#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: InitMBCTable
extern "C" int __stdcall InitMBCTable(void)
{
    char* base = (char*)D2MOO_Resolve("g_bMbcTableInitialized");
    if (!base)
        return -1; // resolver not injected -> obvious wrong-value sentinel

    int* flag = (int*)base;
    if (*flag == 0) {
        // __setmbcp(-3) is a CRT call not exposed in the provider; its only
        // observable effect on this function is to leave _g_bMBCInitialized == 0
        // when invoked -- which we are about to overwrite below. The return value
        // is unaffected either way.
        *flag = 1;
    }
    return 0;
}
