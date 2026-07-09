#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetClassSkillIdByIndex
// int __stdcall(nCharClass 0..6, nSkillIndex) -> class-skill table in g_pDataTables:
//   count[]  = *(int**)(dt+0xba4);  stride = *(int*)(dt+0xba8);  base = *(short**)(dt+0xbac)
//   -> (int)(short)base[stride*nCharClass + nSkillIndex], else -1.
// Reads ONLY g_pDataTables (resolvable) -- pure scalar-arg getter, provable via prove_spec
// (live oracle) with a (class,index) sweep. Verified from disasm 0x6fd9e550 (RET 8; MOVSX word).
extern "C" int __stdcall DATATBLS_GetClassSkillIdByIndex(int nCharClass, int nSkillIndex)
{
    if (nCharClass < 0 || nCharClass > 6) return -1;
    if (nSkillIndex < 0) return -1;
    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return -1;
    char* dt = (char*)*(void**)_g;
    if (dt == nullptr) return -1;
    int* pCount = *(int**)(dt + 0xba4);
    if (nSkillIndex >= pCount[nCharClass]) return -1;
    int stride = *(int*)(dt + 0xba8);
    short* pTable = *(short**)(dt + 0xbac);
    return (int)pTable[stride * nCharClass + nSkillIndex];
}
