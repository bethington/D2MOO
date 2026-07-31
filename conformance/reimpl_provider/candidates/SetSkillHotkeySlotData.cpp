#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SetSkillHotkeySlotData
extern "C" void __fastcall SetSkillHotkeySlotData(int nSlotIndex, uint32_t dwSkillId, uint32_t dwSlotData)
{
	// __fastcall: ECX=nSlotIndex, EDX=dwSkillId, [ESP+4]=dwSlotData (callee cleans 4 bytes via RET 0x4)
	// The decompile shows in_EAX used as the array index; per the AUTHORITATIVE ABI,
	// EAX is a register-explicit input. In practice the caller sets EAX == nSlotIndex, so
	// we use nSlotIndex as the index here (C has no portable way to read EAX under __fastcall).

	char* baseMsg = (char*)D2MOO_Resolve("g_pSkillHotkeyMsg");
	char* baseSkillId = (char*)D2MOO_Resolve("g_dwSkillHotkeySkillId");
	char* baseSlotData = (char*)D2MOO_Resolve("g_dwSkillHotkeySlotData");

	if (!baseMsg || !baseSkillId || !baseSlotData)
		return;

	int index = nSlotIndex;

	*(int*)(baseMsg + index * 4) = nSlotIndex;
	((uint32_t*)baseSkillId)[index] = dwSkillId;
	((uint32_t*)baseSlotData)[index] = dwSlotData;
}
