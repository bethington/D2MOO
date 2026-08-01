#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: InitializeMenuOptionType3
extern "C" void __cdecl InitializeMenuOptionType3(void)
{
	int nIndex;
#if defined(_MSC_VER)
	__asm { mov nIndex, eax }
#else
	nIndex = 0;
#endif

	int idx = nIndex * 0x27;

	char* base1 = (char*)D2MOO_Resolve("g_anPartyEntryCount");
	if (base1) *(int*)(base1 + idx) = 3;

	char* base2 = (char*)D2MOO_Resolve("g_awPartyMenuOptionFlags");
	if (base2) *(short*)(base2 + idx) = 0;

	char* base3 = (char*)D2MOO_Resolve("g_anPartyMenuOptionStates");
	if (base3) *(int*)(base3 + idx) = 0;
}
