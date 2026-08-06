// D2MOO_REIMPL_EXPORT: CLIENT_SetWorldView
#include "../provider_runtime.h"

#include <windows.h>

// D2MOO_REIMPL_EXPORT: CLIENT_SetWorldView
//
// SGD2FreeRes.dll + 0x1bcc0 -- void __cdecl(pViewports, nLeft, nTop, nRight, nBottom)
//
// Initialises two RECTs in a caller-owned struct and sets bit 0 of a flags word,
// applying an isometric perspective correction when a perspective flag is set.
//
// DERIVED FROM THE DISASSEMBLY, NOT FROM SOURCE. We hold the source for this
// binary (it is the conformance lab's subject precisely because we do), and
// proving a copy of that source against a binary built from it would prove
// nothing. Every constant and every operation below was read off the listing at
// 0x1001bcc0..0x1001bdce; the source is reserved for GRADING this afterwards.
//
// STRUCT LAYOUT (36 bytes, the class-B outbuf extent):
//     +0x00  uint32   flags        bit 0 = initialised
//     +0x04  RECT     panel        left, top, right, bottom
//     +0x14  RECT     world        left, top, right, bottom
//
// CALLEE RESOLUTION. The three internal callees live in SGD2FreeRes.dll, which
// prefers base 0x10000000 but does NOT get it -- ProjectDiablo.dll claims that
// address first, so the module is relocated (0x6cfb0000 in the measured
// process). An absolute address from Ghidra would call unmapped memory; that
// exact mistake cost 104 false terminal verdicts on D2Client.
//
// It also cannot go through D2MOO_Resolve: gen_resolve_table deliberately
// EXCLUDES 0x10000000-based modules ("relocatable helpers aren't in the table"),
// because ProjectDiablo.dll, PD2_EXT.dll, SGD2FreeDisplayFix.dll and
// SGD2FreeRes.dll all prefer that base and address->module attribution is a true
// tie there, which _module_for_address is fatal on by design.
//
// So this asks the LOADER for the module by name and adds the RVA. That is
// unambiguous, needs no table entry, and re-reads correctly across relaunches.

namespace {

// RVAs read from the Ghidra program (image base 0x10000000).
const uint32_t kRvaGetDisplayWidth   = 0x203b0;   // CALL 0x100203b0
const uint32_t kRvaGetDisplayHeight  = 0x202f0;   // CALL 0x100202f0
const uint32_t kRvaIsPerspectiveMode = 0x5ca0;    // CALL 0x10005ca0

void* SgdFn(uint32_t rva)
{
    HMODULE h = GetModuleHandleA("SGD2FreeRes.dll");
    if (!h)
        return nullptr;
    return (void*)((uint8_t*)h + rva);
}

typedef int(__cdecl* IntFn)(void);

struct Viewports
{
    uint32_t flags;   // +0x00
    RECT     panel;   // +0x04
    RECT     world;   // +0x14
};

} // namespace

extern "C" void __cdecl CLIENT_SetWorldView(
    void* pViewports, int nLeft, int nTop, int nRight, int nBottom)
{
    // 0x1001bcc5..0x1001bcca: MOV EDI,[EBP+8] / TEST EDI,EDI / JZ -> early out.
    // The original does NOT set the flag on the null path (the OR is past the
    // branch target only for the non-null case reaching 0x1001bdc6 via fallthrough
    // -- the JZ at 0x1001bcca targets 0x1001bdca, AFTER the OR).
    if (!pViewports)
        return;

    Viewports* v = (Viewports*)pViewports;

    IntFn GetWidth  = (IntFn)SgdFn(kRvaGetDisplayWidth);
    IntFn GetHeight = (IntFn)SgdFn(kRvaGetDisplayHeight);
    IntFn IsPersp   = (IntFn)SgdFn(kRvaIsPerspectiveMode);
    if (!GetWidth || !GetHeight || !IsPersp)
        return;   // module missing -> leave the buffer untouched, diverge loudly

    // 0x1001bce1: SetRect(pViewports+0x04, nLeft, nTop, nRight, nBottom)
    SetRect(&v->panel, nLeft, nTop, nRight, nBottom);

    // 0x1001bcea / 0x1001bcf2: width then height, in that order.
    const int w = GetWidth();
    const int h = GetHeight();

    // 0x1001bd0a: SetRect(pViewports+0x14, -0x50, -0x50, w+0x50, h+0x28)
    SetRect(&v->world, -0x50, -0x50, w + 0x50, h + 0x28);

    // 0x1001bd10..0x1001bd17: perspective gate. Zero -> straight to the flag.
    if (IsPersp() != 0)
    {
        // 0x1001bd1d..0x1001bd4b. BOTH halves are truncated to int FIRST
        // (CVTTSD2SI), and only halfH is converted BACK to double for the
        // arithmetic. Doing the maths in double throughout would round
        // differently, and class B compares the written bytes.
        const int halfWi = (int)((double)w * 0.5);   // ECX
        const int halfHi = (int)((double)h * 0.5);   // EAX
        const double halfH = (double)halfHi;         // XMM3

        // 0x1001bd4f..0x1001bd7e: COMISD 866.0, halfH / JBE.
        // Jump taken when 866.0 <= halfH, i.e. scale = 1.0 at or above 866.
        double scale;
        if (866.0 <= halfH)
            scale = 1.0;
        else
            scale = 1732.0 / (1732.0 - halfH) - 1.0;

        // 0x1001bd81..0x1001bd90: world.top -= (int)(halfH * scale)
        v->world.top -= (int)(halfH * scale);

        // 0x1001bd8c..0x1001bdb0: dx = (int)(halfW * scale); left -= dx; right += dx.
        const int dx = (int)((double)halfWi * scale);
        v->world.left  -= dx;
        v->world.right += dx;

        // 0x1001bd9b..0x1001bdc3:
        //   XMM1 = 1500.0 / (halfH + 1500.0); XMM2 = 1.0 - XMM1; XMM2 *= halfH
        //   world.bottom -= (int)XMM2
        v->world.bottom -= (int)((1.0 - 1500.0 / (halfH + 1500.0)) * halfH);
    }

    // 0x1001bdc6: OR dword ptr [EDI],0x1
    v->flags |= 1u;
}
