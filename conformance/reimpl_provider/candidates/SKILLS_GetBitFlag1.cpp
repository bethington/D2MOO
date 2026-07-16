#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SKILLS_GetBitFlag1
// stdcall(nSkillId) -> byte at skillRow+0x6 AND bitmask[1]; 0 on any guard fail
extern "C" uint32_t __stdcall SKILLS_GetBitFlag1(int nSkillId)
{
	// MOV ECX,[0x6fde9e1c] -- g_pDataTables is a POINTER variable: resolve, deref once.
	void* _g = D2MOO_Resolve("g_pDataTables");
	if (_g == nullptr)
		return 0xDEADBEEF; // resolver missing -> obvious mismatch sentinel
	char* base = (char*)*(void**)_g;
	if (base == nullptr)
		return 0xDEADBEEF;

	// JL: negative id -> 0.  CMP EAX,[ECX+0xBA0] / JGE: id >= count -> 0.
	if (nSkillId < 0 || nSkillId >= *(int*)(base + 0xBA0))
		return 0;

	// MOV EDX,[ECX+0xB98]; IMUL EAX,EAX,0x23C; ADD EAX,EDX; TEST EAX,EAX --
	// the null check is on the SUM (base row ptr + offset), mirror exactly.
	uint32_t entry = (uint32_t)*(void**)(base + 0xB98)
	                     + (uint32_t)nSkillId * 0x23C;
	if (entry == 0)
		return 0;

	// MOVZX EAX, byte [entry+0x6]
	uint32_t b = *(uint8_t*)(entry + 0x6);

	// MOV ECX,[0x6fdd90b0]; AND EAX,[ECX+4] -- bitmask array is behind a pointer var too.
	void* _m = D2MOO_Resolve("g_dat_6fdd90b0");
	if (_m == nullptr)
		return 0xDEADBEEF;
	char* masks = (char*)*(void**)_m;
	return b & *(uint32_t*)(masks + 4);
}
