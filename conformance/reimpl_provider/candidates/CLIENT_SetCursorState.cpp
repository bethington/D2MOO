#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_SetCursorState
extern "C" void __cdecl CLIENT_SetCursorState(int in_EAX)
{
	int* pResult = (int*)D2MOO_Resolve("g_dwItemEventResult");
	int* pFlags  = (int*)D2MOO_Resolve("g_dwItemEventFlags");
	int* pType   = (int*)D2MOO_Resolve("g_dwItemEventType");
	int* pDelay  = (int*)D2MOO_Resolve("g_dwItemEventDelay");
	if (!pResult || !pFlags || !pType || !pDelay)
		return;

	*pResult = in_EAX;
	*pFlags  = 0;
	*pType   = 5;
	*pDelay  = 1;
	if (in_EAX != 0) {
		*pDelay = 4;
	}
}
