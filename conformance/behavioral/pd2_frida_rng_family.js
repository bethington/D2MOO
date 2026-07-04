'use strict';
// pd2_frida_rng_family — call the whole PD2-S12 SEED_* RNG family via trampolines.
// Proves the trampoline technique generalizes across different Blizzard register
// conventions. All entries are Ghidra-VERIFIED (offsets from D2Common base
// 0x6FD50000). Each trampoline preserves any callee-saved reg it uses (esi/edi).
//
//   SEED_GetRandomNumber    +0x10B0  ecx=pSeed, eax=max            -> uint [0,max)
//   SEED_GetRandomNumberAlt +0x1100  ecx=pSeed, esi=max            -> uint [0,max)
//   SEED_GetRandomInRange   +0x1180  ecx=pSeed, edi=nMin, eax=nMax -> int [nMin,nMax]

function le32(p) {
  const n = parseInt(p.toString(), 16) >>> 0;
  return [n & 0xff, (n >>> 8) & 0xff, (n >>> 16) & 0xff, (n >>> 24) & 0xff];
}

const STUBS = [];   // retain stub memory: Frida GCs Memory.alloc when the last
                    // NativePointer ref drops, which would free live trampolines.

function stub(prologue, target, epilogue) {
  const s = Memory.alloc(Process.pageSize);
  const bytes = [...prologue, 0xba, ...le32(target), 0xff, 0xd2, ...epilogue]; // ...; mov edx,target; call edx; ...
  s.writeByteArray(bytes);
  Memory.protect(s, bytes.length, 'rwx');
  STUBS.push(s);
  return s;
}

let FN = null;

function ensure() {
  const m = Process.findModuleByName('D2Common.dll');
  if (!m) return null;
  if (!FN) {
    const b = m.base;
    FN = {
      // ecx=[esp+4]=pSeed, eax=[esp+8]=max ; ret
      grn: new NativeFunction(stub(
        [0x8b, 0x4c, 0x24, 0x04, 0x8b, 0x44, 0x24, 0x08], b.add(0x10b0), [0xc3]),
        'uint32', ['pointer', 'uint32'], { abi: 'mscdecl' }),
      // push esi; ecx=[esp+8]=pSeed, esi=[esp+12]=max ; pop esi; ret
      alt: new NativeFunction(stub(
        [0x56, 0x8b, 0x4c, 0x24, 0x08, 0x8b, 0x74, 0x24, 0x0c], b.add(0x1100), [0x5e, 0xc3]),
        'uint32', ['pointer', 'uint32'], { abi: 'mscdecl' }),
      // push edi; ecx=[esp+8]=pSeed, edi=[esp+12]=nMin, eax=[esp+16]=nMax ; pop edi; ret
      inr: new NativeFunction(stub(
        [0x57, 0x8b, 0x4c, 0x24, 0x08, 0x8b, 0x7c, 0x24, 0x0c, 0x8b, 0x44, 0x24, 0x10],
        b.add(0x1180), [0x5f, 0xc3]),
        'int32', ['pointer', 'int32', 'int32'], { abi: 'mscdecl' }),
    };
  }
  return m.base.toString();
}

function seed(low, high) {
  const p = Memory.alloc(8);
  p.writeU32(low >>> 0);
  p.add(4).writeU32(high >>> 0);
  return p;
}

rpc.exports = {
  ready: function () { return ensure(); },
  grn: function (low, high, max) {
    const p = seed(low, high);
    const ret = FN.grn(p, max >>> 0);
    return { ret: ret >>> 0, newLow: p.readU32(), newHigh: p.add(4).readU32() };
  },
  alt: function (low, high, max) {
    const p = seed(low, high);
    const ret = FN.alt(p, max >>> 0);
    return { ret: ret >>> 0, newLow: p.readU32(), newHigh: p.add(4).readU32() };
  },
  inr: function (low, high, nMin, nMax) {
    const p = seed(low, high);
    const ret = FN.inr(p, nMin | 0, nMax | 0);
    return { ret: ret | 0, newLow: p.readU32(), newHigh: p.add(4).readU32() };
  },
};
