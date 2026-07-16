#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: DATATBLS_GetBinkBufferErrorCode
extern "C" uint32_t __stdcall DATATBLS_GetBinkBufferErrorCode(void* pBufferCtx)
{
	int iVar1;

	if (pBufferCtx == nullptr) return 0;

	if (*(int*)(*(int*)((char*)pBufferCtx + 0x10) + 0x48) == 2) {
		iVar1 = *(int*)(*(int*)(*(int*)((char*)pBufferCtx + 0x10) + 0x20) + 8);
		return *(int*)(iVar1 + 4) * 0x3c + 0x44 + *(int*)(iVar1 + 8);
	}

	return 0x6fde3338u;
}
