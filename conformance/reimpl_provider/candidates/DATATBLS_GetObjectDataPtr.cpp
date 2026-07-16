// D2MOO_REIMPL_EXPORT: DATATBLS_GetObjectDataPtr
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall DATATBLS_GetObjectDataPtr(void* pObject)
{
	if (pObject == (void*)0x0) {
		return 0;
	}
	return *(uint32_t*)((uint32_t)pObject + 0xc);
}
