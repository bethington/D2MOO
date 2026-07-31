#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetLevelTileYProperty
extern "C" int __stdcall DATATBLS_GetLevelTileYProperty(int nLevelTileIndex, int nUnitLevel)
{
    char* pDataTables = (char*)*(void**)D2MOO_Resolve("g_pDataTables");
    if (!pDataTables)
        return -0x7FFFFFFF;

    int nRecordCount = *(int*)(pDataTables + 0x00);
    char* pSkillsTxt = *(char**)(pDataTables + 0x04);

    if ((-1 < nLevelTileIndex) && (nLevelTileIndex < nRecordCount))
    {
        char* pEntry = pSkillsTxt + (int64_t)nLevelTileIndex * 0x23c;
        if (pEntry != NULL)
        {
            char bShiftBits = *(char*)(pEntry + 0x188);
            short nField18A = *(short*)(pEntry + 0x18A);
            short nField18C = *(short*)(pEntry + 0x18C);

            int nPropertyValue = ((int)nField18C * (nUnitLevel + -1) + (int)nField18A << (bShiftBits & 0x1f)) >> 8;
            if (nPropertyValue < 0)
                nPropertyValue = 0;
            return nPropertyValue;
        }
    }

    return 0;
}
