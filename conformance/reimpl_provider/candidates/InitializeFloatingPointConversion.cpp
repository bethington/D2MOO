#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: InitializeFloatingPointConversion
extern "C" void __stdcall InitializeFloatingPointConversion()
{
    void* pCvt = D2MOO_Resolve("__cfltcvt");
    void* pCrop = D2MOO_Resolve("__cropzeros");
    void* pAssign = D2MOO_Resolve("__fassign");
    void* pForce = D2MOO_Resolve("__forcdecpt");
    void* pTrap = D2MOO_Resolve("__fptrap");

    if (!pCvt || !pCrop || !pAssign || !pForce || !pTrap)
        return;

    *(void**)D2MOO_Resolve("g_pFpCvt") = pCvt;
    *(void**)D2MOO_Resolve("g_pFpCropZeros") = pCrop;
    *(void**)D2MOO_Resolve("g_pFpAssign") = pAssign;
    *(void**)D2MOO_Resolve("g_pFpForceDecPt") = pForce;
    *(void**)D2MOO_Resolve("g_pFpCvtCallback2") = pTrap;
    *(void**)D2MOO_Resolve("g_pFpCvt2") = pCvt;
}
