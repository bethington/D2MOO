#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ResetQuestStateArrays
extern "C" void __stdcall ResetQuestStateArrays(void)
{
	char* base1 = (char*)D2MOO_Resolve("g_abSkillTreeTooltipVariants_6fbd2496");
	char* base2 = (char*)D2MOO_Resolve("g_abSkillTooltipVariantIndex");
	if (!base1 || !base2)
		return; // resolver missing / unknown name -> obvious wrong-state sentinel (return without writes)

	int iVar1 = 0;
	do {
		base1[iVar1] = 0;
		base2[iVar1] = 0;
		iVar1 = iVar1 + 1;
	} while (iVar1 < 0x29);
}
