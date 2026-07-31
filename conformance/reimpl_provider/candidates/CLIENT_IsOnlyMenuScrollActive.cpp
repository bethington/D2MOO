#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_IsOnlyMenuScrollActive
extern "C" int __fastcall CLIENT_IsOnlyMenuScrollActive(int nSliderIndex)
{
	int* base = (int*)D2MOO_Resolve("g_nSliderPressed");
	if (!base)
		return -1; // resolver missing -> obvious mismatch

	int nSliderIdx = 0;
	while ((base[nSliderIdx] == 0) || (nSliderIdx == nSliderIndex)) {
		nSliderIdx = nSliderIdx + 1;
		if (6 < nSliderIdx) {
			return 1;
		}
	}
	return 0;
}
