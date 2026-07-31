#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: HasEntryById
extern "C" int __stdcall HasEntryById(int nEntityId)
{
	void** base = (void**)D2MOO_Resolve("g_pDataTables");
	if (!base) return 0;
	if (nEntityId < 0) return 0;
	void* pvVar1 = base[nEntityId];
	return pvVar1 != (void*)0;
}
