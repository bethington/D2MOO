// d2moo_loader_harness -- Milestone 3 of ../DATATABLE_CONFORMANCE_PLAN.md.
//
// Links the REAL compiled D2Common (statically) and drives its REAL data-table
// loader (DATATBLS_LoadAllTxts) against PD2's REAL MPQ archives on disk, via
// D2MOO's Storm import (STORM_SFileOpenArchive). No game process needed --
// this is D2MOO acting as its own oracle for the data-table subsystem.
//
// PD2 does not use the vanilla d2data.mpq/d2exp.mpq/etc set (ProjectD2's
// directory has no such files); it's a total-conversion mod with its own
// consolidated archives: patch_d2.mpq, pd2data.mpq, pd2assets.mpq, pd2maps.mpq.
// Experience.txt is Excel/data, so pd2data.mpq should suffice; patch_d2.mpq is
// opened too in case it carries an override (empirically verified below by
// diffing against the live-game golden capture, pd2_experience.json).
//
// Emits the same shape as pd2_experience.json so compare_fp.py-style diffing
// works directly: level 0 = the tMax marker row, then aLevels[0..].

#include <cstdio>
#include <cstdint>
#include <windows.h>
#include <Storm.h>
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
    // Open in an order that lets patch_d2.mpq override pd2data.mpq if Storm's
    // LIFO search applies (most-recently-opened searched first).
    OpenArchive("pd2data.mpq");
    OpenArchive("pd2assets.mpq");
    OpenArchive("pd2maps.mpq");
    OpenArchive("patch_d2.mpq");

    fprintf(stderr, "Calling DATATBLS_LoadAllTxts...\n");
    DATATBLS_LoadAllTxts(nullptr, 0, 0);
    fprintf(stderr, "DATATBLS_LoadAllTxts returned.\n");

    if (!sgptDataTables || !sgptDataTables->pExperienceTxt) {
        fprintf(stderr, "!! pExperienceTxt is NULL -- load failed.\n");
        return 1;
    }

    static const char* kClassNames[7] = {
        "Amazon", "Sorceress", "Necromancer", "Paladin", "Barbarian", "Druid", "Assassin"
    };

    auto printRow = [&](int level, const D2ExperienceTxt& row) {
        printf("{\"level\":%d", level);
        for (int c = 0; c < 7; c++) {
            printf(",\"%s\":%u", kClassNames[c], row.dwClass[c]);
        }
        printf(",\"ExpRatio\":%u}\n", row.dwExpRatio);
    };

    printf("[\n");
    printRow(0, sgptDataTables->pExperienceTxt->tMax);
    for (int lvl = 1; lvl <= 11; lvl++) {
        printRow(lvl, sgptDataTables->pExperienceTxt->aLevels[lvl - 1]);
    }
    printf("]\n");
    return 0;
}
