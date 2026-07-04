'use strict';
// pd2_frida_datatbl -- read PD2-S12's LIVE data tables (already loaded from its
// own MPQs at game init) via Frida. This is the "PD2 golden" half of data-table
// conformance (see ../DATATABLE_CONFORMANCE_PLAN.md): no MPQ parsing needed on
// our side -- the running game already parsed Experience.txt into a flat record
// array, we just read it back.
//
// g_pExperienceTxtRecords @ D2Common+0xA0B50 (Ghidra-VERIFIED, VA 0x6FDF0B50 -
// base 0x6FD50000). Record stride 0x20 (32 bytes): 7 class DWORDs (Amazon,
// Sorceress, Necromancer, Paladin, Barbarian, Druid, Assassin) + ExpRatio DWORD.
// Matches D2MOO's D2ExperienceTxt (source/D2Common/include/D2DataTbls.h).

const OFF_PTR = 0xa0b50;   // &g_pExperienceTxtRecords (a pointer variable)
const STRIDE = 0x20;
const CLASSES = ['Amazon', 'Sorceress', 'Necromancer', 'Paladin', 'Barbarian', 'Druid', 'Assassin'];

rpc.exports = {
  ready: function () {
    const m = Process.findModuleByName('D2Common.dll');
    return m ? m.base.toString() : null;
  },
  // Dump `count` experience-table rows (level 0..count-1).
  dump: function (count) {
    const m = Process.findModuleByName('D2Common.dll');
    if (!m) return null;
    const pRecords = m.base.add(OFF_PTR).readPointer();
    if (pRecords.isNull()) return { error: 'g_pExperienceTxtRecords is NULL (not loaded yet)' };
    const rows = [];
    for (let i = 0; i < count; i++) {
      const row = { level: i };
      const base = pRecords.add(i * STRIDE);
      CLASSES.forEach((name, idx) => { row[name] = base.add(idx * 4).readU32(); });
      row.ExpRatio = base.add(7 * 4).readU32();
      rows.push(row);
    }
    return { ptr: pRecords.toString(), rows: rows };
  },
};
