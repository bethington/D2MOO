#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: entry
// NEEDS GLOBAL: g_nRefCount
// NEEDS GLOBAL: g_pfnDllHook

extern "C" uint32_t __stdcall INIT_ExecCommand(void* hModule, int nReason);
extern "C" uint32_t __stdcall GetModuleQuery(void* hModule, int nReason);

extern "C" int __stdcall entry(void* hModule, int nReason, uint32_t dwReserved)
{
	void* resolveRefCount = D2MOO_Resolve("g_nRefCount");
	if (!resolveRefCount) return -0x7FFFFFFF;

	void* resolveHook = D2MOO_Resolve("g_pfnDllHook");
	char* hookBase = resolveHook ? (char*)*(void**)resolveHook : NULL;

	char* refCountBase = (char*)resolveRefCount;

	int iVar1 = nReason;
	uint32_t uVar2 = *(uint32_t*)refCountBase;
	int nResult;
	uint32_t dwReturn;
	uint32_t dwReturn2;
	int nReasonSaved;

	if (nReason != 0) {
		if ((nReason != 1) && (nReason != 2)) goto exit_with_failure;
		if ((hookBase != (void*)0x0) &&
		   (nResult = ((int(__stdcall*)(void*, int, uint32_t))hookBase)(hModule, nReason, dwReserved), nResult == 0)) {
			return 0;
		}
		uVar2 = INIT_ExecCommand(hModule, nReason);
	}
	if (uVar2 == 0) {
		return 0;
	}
exit_with_failure:
	dwReturn = GetModuleQuery(hModule, nReason);
	if (nReason == 1) {
		if (dwReturn != 0) {
			return dwReturn;
		}
		INIT_ExecCommand(hModule, 0);
	}
	if ((nReason != 0) && (nReason != 3)) {
		return dwReturn;
	}
	dwReturn2 = INIT_ExecCommand(hModule, nReason);
	nReason = (int)dwReturn;
	if (dwReturn2 == 0) {
		nReason = 0;
	}
	if (nReason != 0) {
		if (hookBase != (void*)0x0) {
			nReasonSaved = ((int(__stdcall*)(void*, int, uint32_t))hookBase)(hModule, iVar1, dwReserved);
			return nReasonSaved;
		}
		return nReason;
	}
	return 0;
}
