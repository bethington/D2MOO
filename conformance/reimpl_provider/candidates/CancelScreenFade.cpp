#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CancelScreenFade
extern "C" void __stdcall CancelScreenFade(void)
{
    char* base = (char*)D2MOO_Resolve("g_bClickActionActive");
    if (!base)
        return;
    *base = 0;
}
