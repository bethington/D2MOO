// town_levelid.cpp -- WS-6a capstone: a REAL non-coord D2Common reimpl proven
// live via the general oracle (design detail B), demonstrating the pipeline
// generalizes beyond the coord family.
//
// DUNGEON_GetTownLevelIdFromActNo(byte nAct) -> town level id. In PD2's D2Common
// this is at offset 0x3b1e0 (Ghidra name now DUNGEON_GetTownLevelIdFromActNo, renamed
// 2026-07-07 from the decompiler's generic auto-name DATATBLS_GetBoundedArrayValue);
// __stdcall, one byte arg, int return -- so it exercises the marshaller's
// scalar-arg + non-void-return path the coord family never touches.
//
// BOUNDS: the ORIGINAL aborts the whole process for nAct > 4 (CleanupAndAbort,
// error 0x59A), so the oracle vectors MUST stay in 0..4 (town_levelid.spec.json).
// The reimpl mirrors D2MOO's gnTownLevelIds. Values verified three ways: D2MOO
// source (source/D2Common/src/D2Dungeon.cpp), the LevelsIds.h enum line trace,
// and the PD2 .rdata array bytes (01 28 4B 67 6D = 1 40 75 103 109).

// D2MOO_REIMPL_EXPORT: DUNGEON_GetTownLevelIdFromActNo
extern "C" int __stdcall DUNGEON_GetTownLevelIdFromActNo(unsigned char nAct)
{
	// { LEVEL_ROGUEENCAMPMENT, LEVEL_LUTGHOLEIN, LEVEL_KURASTDOCKTOWN,
	//   LEVEL_THEPANDEMONIUMFORTRESS, LEVEL_HARROGATH }
	static const int gnTownLevelIds[] = { 1, 40, 75, 103, 109 };
	if (nAct > 4)
		return 0; // original aborts here; the oracle only ever feeds 0..4
	return gnTownLevelIds[nAct];
}
