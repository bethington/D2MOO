#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_HaveColdResBonus
extern "C" uint32_t __stdcall DATATBLS_HaveColdResBonus(int nRecordIndex)
{
	void* pDataTablesAddr = D2MOO_Resolve("g_pDataTables");
	if (!pDataTablesAddr)
		return 0xDEADBEEFu;

	char* base = (char*)*(void**)pDataTablesAddr;
	if (!base)
		return 0xDEADBEEFu;

	int count = *(int*)(base + 0xC5C);
	if ((0 < nRecordIndex) && (nRecordIndex < count))
	{
		char* pSkillsTbl = *(char**)(base + 0xC58);
		uint32_t offset = (uint32_t)(nRecordIndex * 0x220);
		return (offset & 0xFFFFFF00u) | (uint8_t)pSkillsTbl[offset + 6];
	}
	return (uint32_t)(nRecordIndex & 0xffffff00);
}
