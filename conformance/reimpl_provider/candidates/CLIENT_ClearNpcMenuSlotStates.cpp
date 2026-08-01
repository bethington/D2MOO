#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_ClearNpcMenuSlotStates
extern "C" void __stdcall CLIENT_ClearNpcMenuSlotStates(void)
{
    void* p0 = D2MOO_Resolve("g_pAutoMapVtableBase_6fbc9685");
    void* p1 = D2MOO_Resolve("g_anNpcMenuSlotStates_1");
    void* p2 = D2MOO_Resolve("g_anNpcMenuSlotStates_2");
    void* p3 = D2MOO_Resolve("g_anNpcMenuSlotStates3");
    void* p4 = D2MOO_Resolve("g_anNpcMenuSlotStates4");
    void* p5 = D2MOO_Resolve("g_anNpcMenuSlotStates_5");
    void* p6 = D2MOO_Resolve("g_anNpcMenuSlotStates");
    void* p7 = D2MOO_Resolve("g_anNpcMenuSlotStates7");
    void* p8 = D2MOO_Resolve("g_anNpcMenuSlotStates_8");
    void* p9 = D2MOO_Resolve("g_anNpcMenuSlotStates_9");
    if (!p0 || !p1 || !p2 || !p3 || !p4 || !p5 || !p6 || !p7 || !p8 || !p9)
        return;

    *(int*)p0 = 0;
    *(int*)p1 = 0;
    *(int*)p2 = 0;
    *(int*)p3 = 0;
    *(int*)p4 = 0;
    *(int*)p5 = 0;
    *(int*)p6 = 0;
    *(int*)p7 = 0;
    *(int*)p8 = 0;
    *(int*)p9 = 0;
}
