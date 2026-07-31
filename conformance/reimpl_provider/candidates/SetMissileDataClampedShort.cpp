#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SetMissileDataClampedShort
extern "C" void __stdcall SetMissileDataClampedShort(void* pUnit, int nValue)
{
	if (!pUnit)
		return;
	if (*(uint32_t*)((char*)pUnit + 0x0) != 3u)
		return;
	void* pPVar1 = *(void**)((char*)pUnit + 0x14);
	if (!pPVar1)
		return;
	if (nValue < 1) {
		*(char*)((char*)pPVar1 + 6) = '\0';
		*(char*)((char*)pPVar1 + 7) = '\0';
		return;
	}
	if (0x7FFE < nValue) {
		nValue = 0x7FFF;
	}
	*(short*)((char*)pPVar1 + 6) = (short)nValue;
}
