#include "../provider_runtime.h"

// NEEDS GLOBAL: g_dwMemAlignMask
// NEEDS GLOBAL: g_dwFillDebugMode
// NEEDS GLOBAL: g_dwAllocGranularity
// NEEDS GLOBAL: g_dwStormDebugEnabled

// D2MOO_REIMPL_EXPORT: ExtractConfigurationBitFields
extern "C" void __stdcall ExtractConfigurationBitFields(uint32_t dwConfigValue, uint8_t byFieldMask)
{
	uint32_t* pMemAlign      = (uint32_t*)D2MOO_Resolve("g_dwMemAlignMask");
	uint32_t* pFillDebug     = (uint32_t*)D2MOO_Resolve("g_dwFillDebugMode");
	uint32_t* pAllocGran     = (uint32_t*)D2MOO_Resolve("g_dwAllocGranularity");
	uint32_t* pStormDebug    = (uint32_t*)D2MOO_Resolve("g_dwStormDebugEnabled");

	if (!pMemAlign || !pFillDebug || !pAllocGran || !pStormDebug)
		return;

	/* Check if bit 0 (MemAlignMask) is enabled in mask */
	if ((byFieldMask & 1) != 0) {
		*pMemAlign = dwConfigValue & 1u;
	}
	/* Check if bit 3 (FillDebugMode) is enabled in mask */
	if ((byFieldMask & 8) != 0) {
		*pFillDebug = (dwConfigValue >> 3) & 1u;
	}
	/* Check if bit 2 (AllocGranularity) is enabled in mask */
	if ((byFieldMask & 4) != 0) {
		*pAllocGran = (dwConfigValue >> 2) & 1u;
	}
	/* Check if bit 1 (StormDebugFlag) is enabled in mask */
	if ((byFieldMask & 2) != 0) {
		*pStormDebug = (dwConfigValue >> 1) & 1u;
	}
}
