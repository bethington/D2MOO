'use strict';
// pd2_frida_rng_probe — self-locate PD2's RNG LCG by its unique magic multiplier
// 0x6AC690C5 (D2Seed.h: lSeed = nHighSeed + 0x6AC690C5 * nLowSeed), then find the
// enclosing function entry and (if found) call it fastcall(pSeed)->uint64 with
// chosen seeds. Address-scramble-proof: no ordinals, no D2MOO/vanilla addresses.

const MAGIC_LE = 'c5 90 c6 6a';   // 0x6AC690C5 little-endian

function textRange(mod) {
  // first executable range of the module
  for (const r of mod.enumerateRanges('r-x')) return r;
  return { base: mod.base, size: mod.size };
}

// Walk backwards to a plausible function entry: scan for int3/nop padding (CC/90)
// or a preceding RET (C3), then the entry is the next byte.
function findEntry(hitAddr, floor) {
  let p = hitAddr;
  for (let i = 0; i < 512; i++) {
    const b = p.readU8();
    const prev = p.sub(1).readU8();
    if ((prev === 0xCC || prev === 0x90 || prev === 0xC3) && b !== 0xCC && b !== 0x90) {
      return p;
    }
    p = p.sub(1);
    if (p.compare(floor) <= 0) break;
  }
  return null;
}

function disasm(addr, n) {
  const out = [];
  let p = addr;
  try {
    for (let i = 0; i < n; i++) {
      const insn = Instruction.parse(p);
      out.push(`${p} ${insn.mnemonic} ${insn.opStr}`);
      p = insn.next;
    }
  } catch (e) { out.push('(disasm err: ' + e.message + ')'); }
  return out;
}

function d2common() {
  // lazy: the game may still be loading when we attach
  const m = Process.findModuleByName('D2Common.dll');
  return m || null;
}

rpc.exports = {
  probe: function () {
    const mod = d2common();
    if (!mod) return { loaded: false };
    const rng = textRange(mod);
    const hits = [];
    Memory.scanSync(rng.base, rng.size, MAGIC_LE).forEach(m => hits.push(m.address));
    const report = { base: mod.base.toString(), hits: hits.map(h => h.toString()),
                     candidates: [] };
    for (const h of hits) {
      const entry = findEntry(h, rng.base);
      report.candidates.push({
        magic_at: h.toString(),
        entry: entry ? entry.toString() : null,
        ctx: disasm(entry || h.sub(16), 12),
      });
    }
    return report;
  },
  // Call a located entry as fastcall(pSeed)->uint64 with seed (low,high); return
  // the returned 64-bit value and the seed struct after the call.
  roll: function (entryStr, low, high) {
    const fn = new NativeFunction(ptr(entryStr), 'uint64', ['pointer'], { abi: 'fastcall' });
    const pSeed = Memory.alloc(8);
    pSeed.writeU32(low >>> 0); pSeed.add(4).writeU32(high >>> 0);
    const ret = fn(pSeed);   // uint64 -> a Frida UInt64
    return {
      ret: ret.toString(),
      newLow: pSeed.readU32(),
      newHigh: pSeed.add(4).readU32(),
    };
  },
};
