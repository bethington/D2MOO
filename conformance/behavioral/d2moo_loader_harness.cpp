// d2moo_loader_harness -- Milestone 3 of ../DATATABLE_CONFORMANCE_PLAN.md.
//
// Links the REAL compiled D2Common (statically) and drives its REAL data-table
// compiler (DATATBLS_CompileTxt) against PD2's REAL MPQ archives on disk, via
// D2MOO's Storm import (SFileOpenArchive). No game process needed -- D2MOO acts
// as its own oracle for the data-table subsystem.
//
// Runtime deps (copied next to this .exe): the REAL Storm.dll/Fog.dll/D2CMP.dll/
// D2Lang.dll from ProjectD2 (D2MOO's own rebuilt Storm/Fog are stubs that don't
// parse MPQs), PLUS a PD2_EXT.dll that is really the Windows version.dll renamed:
// PD2's Storm.dll imports 3 harmless version.dll funcs (GetFileVersionInfoA,
// GetFileVersionInfoSizeA, VerQueryValueA) from "PD2_EXT.dll" -- a version.dll
// DLL-hijack the mod uses to bootstrap itself. The real PD2_EXT.dll's DllMain
// fails outside Game.exe; a plain version.dll satisfies those 3 imports with no
// mod bootstrap, so Storm.dll loads and parses MPQs cleanly.
//
// PD2 uses its own consolidated archive set (pd2data.mpq etc.), NOT the vanilla
// d2data.mpq/d2exp.mpq split. Experience.txt lives in pd2data.mpq.
//
// Rather than DATATBLS_LoadAllTxts (which loads ~30 tables and stack-overflows
// deep in the chain on some PD2-modified table), we call DATATBLS_CompileTxt for
// just "experience" -- the exact call the game makes -- and read the result.

#include <cstdio>
#include <cstdint>
#include <windows.h>
#include <Storm.h>
#include <Fog.h>
#include <D2DataTbls.h>

static const char* kMpqPath = "C:\\Diablo2\\ProjectD2\\";

static bool OpenArchive(const char* name) {
    char path[MAX_PATH];
    wsprintfA(path, "%s%s", kMpqPath, name);
    HSARCHIVE hArchive = nullptr;
    BOOL ok = SFileOpenArchive(path, 0, 0, &hArchive);
    fprintf(stderr, "  SFileOpenArchive(%s) -> %s\n", path, ok ? "OK" : "FAILED");
    return ok != 0;
}

int main() {
    // patch_d2.mpq FIRST so it wins the priority search, matching the real game
    // (empirically, Storm searches first-opened-first here; patch_d2 is D2's
    // override layer). difficultylevels.txt differs between patch_d2 and pd2data,
    // and the game reads patch_d2's copy -- open order is load-bearing.
    OpenArchive("patch_d2.mpq");
    OpenArchive("pd2data.mpq");
    OpenArchive("pd2assets.mpq");
    OpenArchive("pd2maps.mpq");

    // Parse experience.txt (not a pre-compiled .bin -- PD2 ships the .txt).
    DATATBLS_LoadFromBin = FALSE;

    // Field schema, exactly as DATATBLS_LoadAllTxts builds it for Experience.
    BinField pTbl[] = {
        { "Amazon",      TXTFIELD_DWORD, 0, 0,  nullptr },
        { "Sorceress",   TXTFIELD_DWORD, 0, 4,  nullptr },
        { "Necromancer", TXTFIELD_DWORD, 0, 8,  nullptr },
        { "Paladin",     TXTFIELD_DWORD, 0, 12, nullptr },
        { "Barbarian",   TXTFIELD_DWORD, 0, 16, nullptr },
        { "Druid",       TXTFIELD_DWORD, 0, 20, nullptr },
        { "Assassin",    TXTFIELD_DWORD, 0, 24, nullptr },
        { "ExpRatio",    TXTFIELD_DWORD, 0, 28, nullptr },
        { "end",         TXTFIELD_NONE,  0, 0,  nullptr },
    };

    int nRecordCount = 0;
    fprintf(stderr, "Calling DATATBLS_CompileTxt(\"experience\")...\n");
    ExperienceTxt* pRows = (ExperienceTxt*)DATATBLS_CompileTxt(
        nullptr, "experience", pTbl, &nRecordCount, sizeof(ExperienceTxt));
    fprintf(stderr, "DATATBLS_CompileTxt returned %d records at %p\n", nRecordCount, (void*)pRows);

    if (!pRows || nRecordCount <= 0) {
        fprintf(stderr, "!! experience load failed.\n");
        return 1;
    }

    // --- Second table: DifficultyLevels (link-free, 22 DWORDs, 3 records) --
    // Proves the harness generalizes; PD2 modifies difficulty scaling so this
    // catches real PD2 (non-vanilla) content, unlike Experience (matched vanilla).
    BinField pDiff[] = {
        { "ResistPenalty", TXTFIELD_DWORD, 0, 0, nullptr },
        { "DeathExpPenalty", TXTFIELD_DWORD, 0, 4, nullptr },
        { "UberCodeOddsNormal", TXTFIELD_DWORD, 0, 8, nullptr },
        { "UberCodeOddsGood", TXTFIELD_DWORD, 0, 12, nullptr },
        { "UltraCodeOddsNormal", TXTFIELD_DWORD, 0, 32, nullptr },
        { "UltraCodeOddsGood", TXTFIELD_DWORD, 0, 36, nullptr },
        { "LifeStealDivisor", TXTFIELD_DWORD, 0, 40, nullptr },
        { "ManaStealDivisor", TXTFIELD_DWORD, 0, 44, nullptr },
        { "MonsterSkillBonus", TXTFIELD_DWORD, 0, 16, nullptr },
        { "MonsterFreezeDivisor", TXTFIELD_DWORD, 0, 20, nullptr },
        { "MonsterColdDivisor", TXTFIELD_DWORD, 0, 24, nullptr },
        { "AiCurseDivisor", TXTFIELD_DWORD, 0, 28, nullptr },
        { "UniqueDamageBonus", TXTFIELD_DWORD, 0, 48, nullptr },
        { "ChampionDamageBonus", TXTFIELD_DWORD, 0, 52, nullptr },
        { "HireableBossDamagePercent", TXTFIELD_DWORD, 0, 56, nullptr },
        { "MonsterCEDamagePercent", TXTFIELD_DWORD, 0, 60, nullptr },
        { "StaticFieldMin", TXTFIELD_DWORD, 0, 64, nullptr },
        { "GambleRare", TXTFIELD_DWORD, 0, 68, nullptr },
        { "GambleSet", TXTFIELD_DWORD, 0, 72, nullptr },
        { "GambleUnique", TXTFIELD_DWORD, 0, 76, nullptr },
        { "GambleUber", TXTFIELD_DWORD, 0, 80, nullptr },
        { "GambleUltra", TXTFIELD_DWORD, 0, 84, nullptr },
        { "end", TXTFIELD_NONE, 0, 0, nullptr },
    };
    int nDiffCount = 0;
    fprintf(stderr, "Calling DATATBLS_CompileTxt(\"difficultylevels\")...\n");
    DifficultyLevelsTxt* pDiffRows = (DifficultyLevelsTxt*)DATATBLS_CompileTxt(
        nullptr, "difficultylevels", pDiff, &nDiffCount, sizeof(DifficultyLevelsTxt));
    fprintf(stderr, "  -> %d difficulty records at %p\n", nDiffCount, (void*)pDiffRows);

    static const char* kClassNames[7] = {
        "Amazon", "Sorceress", "Necromancer", "Paladin", "Barbarian", "Druid", "Assassin"
    };
    auto printExp = [&](int level, const ExperienceTxt& row) {
        printf("  {\"level\":%d", level);
        for (int c = 0; c < 7; c++) printf(",\"%s\":%u", kClassNames[c], row.dwClass[c]);
        printf(",\"ExpRatio\":%u}", row.dwExpRatio);
    };

    printf("{\n\"experience\": [\n");
    int limit = nRecordCount < 12 ? nRecordCount : 12;
    for (int i = 0; i < limit; i++) {
        printExp(i, pRows[i]);
        printf(i + 1 < limit ? ",\n" : "\n");
    }
    printf("],\n\"difficultylevels\": [\n");
    // raw 22-DWORD dump per record (struct == 22 x uint32, verbatim)
    for (int r = 0; r < nDiffCount; r++) {
        const uint32_t* v = (const uint32_t*)&pDiffRows[r];
        printf("  [");
        for (int f = 0; f < 22; f++) printf(f ? ",%u" : "%u", v[f]);
        printf(r + 1 < nDiffCount ? "],\n" : "]\n");
    }
    printf("]\n}\n");
    return 0;
}
