'use strict';
// pd2_frida_agent — in-process oracle agent for PD2-S12 (loaded by Frida into
// the running Game.exe). Exposes the coord family so the Python driver can call
// each function with CHOSEN inputs and read the outputs, all in-process on the
// game's own address space (no external debugger => no anti-debug flood, no COM
// fragility, no thread-hijack). See reference-frida-inprocess-oracle.
//
// Coord family signature: void __stdcall(int* pX, int* pY). Addressed as
// D2Common.dll base + offset (offset = Ghidra VA - 0x6FD50000), so it is
// ASLR/rebase-proof.

const OFFSETS = {
  DUNGEON_GameTileToClientCoords:    0x4dac0,  // Ghidra 0x6FD9DAC0
  DUNGEON_GameTileToSubtileCoords:   0x4d8a0,  // Ghidra 0x6FD9D8A0
  DUNGEON_ClientToGameCoords:        0x4d8c0,  // Ghidra 0x6FD9D8C0
  DUNGEON_GameToClientCoords:        0x4db40,  // Ghidra 0x6FD9DB40
  DUNGEON_GameSubtileToClientCoords: 0x4db70,  // Ghidra 0x6FD9DB70
};

const mod = Process.getModuleByName('D2Common.dll');
const fns = {};
for (const [name, off] of Object.entries(OFFSETS)) {
  fns[name] = new NativeFunction(mod.base.add(off), 'void',
                                 ['pointer', 'pointer'], { abi: 'stdcall' });
}

rpc.exports = {
  base: function () { return mod.base.toString(); },
  // Call one coord function with a chosen (x, y); return [outX, outY].
  call: function (name, x, y) {
    const px = Memory.alloc(4), py = Memory.alloc(4);
    px.writeS32(x); py.writeS32(y);
    fns[name](px, py);
    return [px.readS32(), py.readS32()];
  },
};
