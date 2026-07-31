#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_SetCursorItemRenderFlag
extern "C" void __fastcall CLIENT_SetCursorItemRenderFlag(uint32_t dwFlag)
{
	char* base = (char*)D2MOO_Resolve("g_dwClientErrorCode");
	if (!base) return;
	*(uint32_t*)base = dwFlag;
}
