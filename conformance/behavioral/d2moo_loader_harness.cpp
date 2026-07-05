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
    OpenArchive("pd2data.mpq");
    OpenArchive("pd2assets.mpq");
    OpenArchive("pd2maps.mpq");
    OpenArchive("patch_d2.mpq");

    // Parse experience.txt (not a pre-compiled .bin -- PD2 ships the .txt).
    DATATBLS_LoadFromBin = FALSE;

    // Field schema, exactly as DATATBLS_LoadAllTxts builds it for Experience.
    D2BinFieldStrc pTbl[] = {
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
    D2ExperienceTxt* pRows = (D2ExperienceTxt*)DATATBLS_CompileTxt(
        nullptr, "experience", pTbl, &nRecordCount, sizeof(D2ExperienceTxt));
    fprintf(stderr, "DATATBLS_CompileTxt returned %d records at %p\n", nRecordCount, (void*)pRows);

    if (!pRows || nRecordCount <= 0) {
        fprintf(stderr, "!! experience load failed.\n");
        return 1;
    }

    static const char* kClassNames[7] = {
        "Amazon", "Sorceress", "Necromancer", "Paladin", "Barbarian", "Druid", "Assassin"
    };
    auto printRow = [&](int level, const D2ExperienceTxt& row) {
        printf("{\"level\":%d", level);
        for (int c = 0; c < 7; c++) printf(",\"%s\":%u", kClassNames[c], row.dwClass[c]);
        printf(",\"ExpRatio\":%u}\n", row.dwExpRatio);
    };

    printf("[\n");
    int limit = nRecordCount < 12 ? nRecordCount : 12;
    for (int i = 0; i < limit; i++) {
        printRow(i, pRows[i]);
        printf(i + 1 < limit ? ",\n" : "\n");   // valid JSON: comma between rows
    }
    printf("]\n");
    return 0;
}
