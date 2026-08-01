#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ValidateAndSetGamePath
extern "C" uint32_t __stdcall ValidateAndSetGamePath(char* lpszPath)
{
    // Resolve the destination buffer (g_sz prefix -> data/array base, no deref)
    char* base = (char*)D2MOO_Resolve("g_szGamePathBuffer");
    // Resolve the last error variable (g_dw prefix -> data, no deref)
    uint32_t* base_err = (uint32_t*)D2MOO_Resolve("g_dwStormLastError");

    // Guard: if resolver missing for the buffer, return sentinel
    if (!base) return 0;

    // 1. Check NULL pointer -> set error 0x57, return 0
    if (lpszPath == (char*)0) {
        if (base_err) *base_err = 0x57;
        return 0;
    }

    // 2. Check empty string -> clear buffer, return 1
    if (*lpszPath == '\0') {
        base[0] = '\0';
        return 1;
    }

    // 3. Calculate string length (inline strlen, matches CalculateStringLength)
    uint32_t uVar2 = 0;
    while (lpszPath[uVar2] != '\0') uVar2++;

    // 4. Get last character
    char cVar1 = lpszPath[uVar2 - 1];

    // 5. Recalculate length (matches decompile exactly - redundant second call)
    uVar2 = 0;
    while (lpszPath[uVar2] != '\0') uVar2++;

    // 6. Length check: 0x104 < len + 1 + (lastChar != '\\')
    if (0x104 < uVar2 + 1 + (uint32_t)(cVar1 != '\\')) {
        if (base_err) *base_err = 0xa1;
        return 0;
    }

    // 7. Enter critical section (best effort - exact CS name not in resolver list)
    void* cs_ptr = (void*)0;
    {
        void* cs_resolved = D2MOO_Resolve("g_pCritSecBase");
        if (cs_resolved) {
            // g_p prefix -> pointer variable -> deref once
            cs_ptr = *(void**)cs_resolved;
        }
    }
    if (cs_ptr) {
        void* enter_addr = D2MOO_Resolve("g_pfnEnterCritSec");
        if (enter_addr) {
            typedef void (__stdcall *EnterCS_t)(void*);
            EnterCS_t enter_fn = *(EnterCS_t*)enter_addr;
            enter_fn(cs_ptr);
        }
    }

    // 8. SStrCopy(base, lpszPath, 0x104) - inline bounded copy
    uint32_t i = 0;
    while (i < 0x103 && lpszPath[i] != '\0') {
        base[i] = lpszPath[i];
        i++;
    }
    base[i] = '\0';

    // 9. If last char is not '\\', append '\\' (inline CopyStringBounded)
    if (cVar1 != '\\') {
        uint32_t bufLen = 0;
        while (base[bufLen] != '\0') bufLen++;
        if (bufLen < 0x103) {
            base[bufLen] = '\\';
            base[bufLen + 1] = '\0';
        }
    }

    // 10. Leave critical section
    if (cs_ptr) {
        void* leave_addr = D2MOO_Resolve("g_pfnLeaveCritSec");
        if (leave_addr) {
            typedef void (__stdcall *LeaveCS_t)(void*);
            LeaveCS_t leave_fn = *(LeaveCS_t*)leave_addr;
            leave_fn(cs_ptr);
        }
    }

    return 1;
}
