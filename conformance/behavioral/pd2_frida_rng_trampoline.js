'use strict';
// pd2_frida_rng_trampoline — call PD2-S12 SEED_GetRandomNumber with chosen inputs.
//
// SEED_GetRandomNumber @ D2Common+0x510B0 (Ghidra-VERIFIED entry) uses a custom
// register ABI: pSeed in ECX, max IMPLICITLY in EAX, returns uint in EAX, updates
// *pSeed (8 bytes {low@0,high@4}) in place. Frida's stock ABIs can't express
// "max in EAX", so we build a tiny raw-machine-code trampoline callable as
// cdecl(pSeed, max) that loads ECX/EAX and calls the verified entry.
//
// SAFE: we only ever call the Ghidra-VERIFIED entry (0x510B0), never a heuristic.

const OFF = 0x10b0;    // SEED_GetRandomNumber @ Ghidra 0x6FD510B0 - base 0x6FD50000

function le32(p) {
  const n = parseInt(p.toString(), 16) >>> 0;
  return [n & 0xff, (n >>> 8) & 0xff, (n >>> 16) & 0xff, (n >>> 24) & 0xff];
}

function makeTrampoline(target) {
  const stub = Memory.alloc(Process.pageSize);
  const bytes = [
    0x8b, 0x4c, 0x24, 0x04,          // mov ecx, [esp+4]   ; pSeed  (arg1)
    0x8b, 0x44, 0x24, 0x08,          // mov eax, [esp+8]   ; max    (arg2)
    0xba, ...le32(target),           // mov edx, <target>
    0xff, 0xd2,                      // call edx           ; SEED_GetRandomNumber
    0xc3,                            // ret                ; return EAX (cdecl)
  ];
  stub.writeByteArray(bytes);
  Memory.protect(stub, bytes.length, 'rwx');
  return stub;
}

let TRAMP = null;

rpc.exports = {
  ready: function () {
    const m = Process.findModuleByName('D2Common.dll');
    if (!m) return null;
    if (!TRAMP) TRAMP = makeTrampoline(m.base.add(OFF));
    return { base: m.base.toString(), entry: m.base.add(OFF).toString() };
  },
  // call SEED_GetRandomNumber(seed={low,high}, max) -> {ret, newLow, newHigh}
  call: function (low, high, max) {
    const fn = new NativeFunction(TRAMP, 'uint32', ['pointer', 'uint32'], { abi: 'mscdecl' });
    const pSeed = Memory.alloc(8);
    pSeed.writeU32(low >>> 0);
    pSeed.add(4).writeU32(high >>> 0);
    const ret = fn(pSeed, max >>> 0);
    return { ret: ret >>> 0, newLow: pSeed.readU32(), newHigh: pSeed.add(4).readU32() };
  },
};
