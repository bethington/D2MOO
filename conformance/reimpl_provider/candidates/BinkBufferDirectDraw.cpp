#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: BinkBufferDirectDraw
// NEEDS GLOBAL: g_dwDisplayInitFlag
// NEEDS GLOBAL: g_pBinkRenderParams
// NEEDS GLOBAL: g_pDirectDraw
extern "C" void __stdcall BinkBufferDirectDraw(void)
{
	// g_dwDisplayInitFlag -> data DWORD counter (data symbol, no extra deref)
	char* baseDw = (char*)D2MOO_Resolve("g_dwDisplayInitFlag");
	if (!baseDw) return;

	// g_pBinkRenderParams -> pointer variable (deref resolved addr ONCE to get the COM iface value)
	char* baseBink = (char*)*(void**)D2MOO_Resolve("g_pBinkRenderParams");
	// g_pDirectDraw -> pointer variable (deref resolved addr ONCE to get the DirectDraw iface value)
	char* baseDD = (char*)*(void**)D2MOO_Resolve("g_pDirectDraw");

	// g_dwDisplayInitFlag = g_dwDisplayInitFlag - 1;
	*(uint32_t*)baseDw = *(uint32_t*)baseDw - 1;

	// if (g_dwDisplayInitFlag == 0)
	if (*(uint32_t*)baseDw == 0) {
		// (**(code **)(*(int *)g_pBinkRenderParams + 8))(g_pBinkRenderParams);
		// vtable slot +8 = index 2 (IUnknown::Release)
		if (baseBink) {
			void (*releaseBink)(void*) = *(void(**)(void*))((*(int*)baseBink) + 8);
			releaseBink(baseBink);
		}

		// (**(code **)(*(int *)g_pDirectDraw + 8))(g_pDirectDraw);
		if (baseDD) {
			void (*releaseDD)(void*) = *(void(**)(void*))((*(int*)baseDD) + 8);
			releaseDD(baseDD);
		}

		// g_pDirectDraw = (void *)0x0;
		*(void**)D2MOO_Resolve("g_pDirectDraw") = (void*)0;
		// g_pBinkRenderParams = (void *)0x0;
		*(void**)D2MOO_Resolve("g_pBinkRenderParams") = (void*)0;
	}
}
