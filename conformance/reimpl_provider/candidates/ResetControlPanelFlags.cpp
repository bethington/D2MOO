// provider reimpl of ResetControlPanelFlags at 0x6facc830 (D2Client.dll)
//
// Resets multiple client control-panel and overlay UI state flags to their
// default values. Each global is read by NAME through the injected
// D2MOO_Resolve so both the live game and this reimpl write the EXACT same
// live addresses. Using names (not hardcoded addresses) ensures the proof
// survives client relocations and re-runs across patches.

#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ResetControlPanelFlags
extern "C" void __stdcall ResetControlPanelFlags(void)
{
	void* p;

	// g_bUiFlagA = false;
	p = D2MOO_Resolve("g_bUiFlagA");
	if (!p) return;
	*(uint8_t*)p = 0;

	// g_bUiFlagB = false;
	p = D2MOO_Resolve("g_bUiFlagB");
	if (!p) return;
	*(uint8_t*)p = 0;

	// g_fSkillBarHover = 0;   (single-precision float 0.0f)
	p = D2MOO_Resolve("g_fSkillBarHover");
	if (!p) return;
	*(float*)p = 0.0f;

	// g_nSelectedSkillBarIndex = -1;   (sentinel 0xFFFFFFFF)
	p = D2MOO_Resolve("g_nSelectedSkillBarIndex");
	if (!p) return;
	*(int*)p = -1;

	// g_bUiFlagD = 0;
	p = D2MOO_Resolve("g_bUiFlagD");
	if (!p) return;
	*(uint8_t*)p = 0;

	// g_bOverlayFlagA = 0;
	p = D2MOO_Resolve("g_bOverlayFlagA");
	if (!p) return;
	*(uint8_t*)p = 0;

	// g_bOverlayFlagB = 0;
	p = D2MOO_Resolve("g_bOverlayFlagB");
	if (!p) return;
	*(uint8_t*)p = 0;

	// g_bVideoInitializationEnabled = 0;
	p = D2MOO_Resolve("g_bVideoInitializationEnabled");
	if (!p) return;
	*(uint8_t*)p = 0;

	// g_bCritterSpawned = 0;
	p = D2MOO_Resolve("g_bCritterSpawned");
	if (!p) return;
	*(uint8_t*)p = 0;
}
