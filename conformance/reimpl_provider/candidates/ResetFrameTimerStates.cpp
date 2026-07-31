#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ResetFrameTimerStates
extern "C" void __stdcall ResetFrameTimerStates(void)
{
	uint32_t* pSavedLastError   = (uint32_t*)D2MOO_Resolve("g_dwSavedLastError");
	uint32_t* pFrameTickCounter = (uint32_t*)D2MOO_Resolve("g_dwFrameTickCounter");
	uint32_t* pLastError_9808   = (uint32_t*)D2MOO_Resolve("g_dwLastError_6fbc9808");
	uint32_t* pLastError_9810   = (uint32_t*)D2MOO_Resolve("g_dwLastError_6fbc9810");
	uint32_t* pFrameTimer0      = (uint32_t*)D2MOO_Resolve("g_dwFrameTimer0");
	uint32_t* pFrameTimer1      = (uint32_t*)D2MOO_Resolve("g_dwFrameTimer1");

	/* resolver missing / unknown symbol -> bail loudly (void return, so just return) */
	if (!pSavedLastError || !pFrameTickCounter || !pLastError_9808 ||
	    !pLastError_9810 || !pFrameTimer0     || !pFrameTimer1)
		return;

	/* Literal translation of the decompile:
	     g_dwSavedLastError        = g_dwFrameTickCounter;
	     g_dwLastError_6fbc9808    = 0x19;
	     g_dwLastError_6fbc9810    = 0;
	     g_dwFrameTimer0           = 0;
	     g_dwFrameTimer1           = 0;
	*/
	*pSavedLastError = *pFrameTickCounter;
	*pLastError_9808 = 0x19u;
	*pLastError_9810 = 0u;
	*pFrameTimer0    = 0u;
	*pFrameTimer1    = 0u;
}
