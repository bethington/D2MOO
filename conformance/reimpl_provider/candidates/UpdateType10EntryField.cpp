// D2MOO_REIMPL_EXPORT: UpdateType10EntryField
//   Function at 0x6fd02db0 (D2Game.dll).
//
//   Live conformance notes:
//   * Original signature is `void __stdcall Fn(int nDelta)` -- 1 stack
//     parameter at [ESP+4], callee-cleans 4 bytes (RET 0x4).
//   * An implicit game-context base arrives in EAX.  The reimpl
//     reproduces the same context by reading the live global via the
//     injected resolver D2MOO_Resolve -- see NEEDS GLOBAL below.
//   * Ghidra's decompile uses SYMBOLIC UnitAny field names (pAct,
//     dwSeed) and an Act anonymous inner field (_1) WITHOUT providing
//     their structural offsets.  Without binary disassembly those
//     offsets are not recoverable from this view; the placeholders
//     below match common D2 layouts and must be verified.
//
//   UpdateType10EntryNode layout (from the decompile plate comment):
//       +0x00  nField00   int   type discriminator (matched against 0x10)
//       +0x18  pField18   UnitAny*
//       +0xF4  pFieldF4   UpdateType10EntryNode*   next-node pointer
//
//   Algorithm:
//       1. ctx = in_EAX                          (implicit, resolved here)
//       2. ppList = *(int**)(ctx + 0x10F4)
//       3. pNode  = *ppList
//       4. if (NULL) return
//       5. while (pNode->nField00 != 0x10) {
//              pNode = pNode->pFieldF4;
//              if (!pNode) return;
//          }
//       6. pUnit   = pNode->pField18            (UnitAny *)
//       7. pAct    = pUnit->pAct
//       8. *(byte *)((int)pUnit->dwSeed + 7) = 1
//       9. pUnit->pAct = (Act *)((int)pAct->_1 + nDelta)
//
// NEEDS GLOBAL: g_dwGameContext  -- the implicit EAX game-context base.
//   Function reads the list-container pointer at +0x10F4 from it.
#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: UpdateType10EntryField
extern "C" void __stdcall UpdateType10EntryField(int nDelta)
{
    // STEP 1 -- resolve the implicit EAX game-context base by NAME.
    char* pContext = (char*)D2MOO_Resolve("g_dwGameContext");
    if (!pContext) return;                 // resolver not injected / name unknown

    // STEP 2 -- translate *_g_pFoo + 0xNN literally.
    //   *(int **)(in_EAX + 0x10f4)  -- pointer to list container
    int** ppList = *(int***)(pContext + 0x10F4);
    if (!ppList) return;

    // **ppList  -- head node (UpdateType10EntryNode*)
    char* pNode = (char*)*ppList;
    if (!pNode) return;

    // Walk pFieldF4 until nField00 == 0x10.
    while (*(int*)pNode != 0x10) {
        pNode = *(char**)(pNode + 0xF4);
        if (!pNode) return;
    }

    // pUVar1 = pNode->pField18   -- UnitAny *
    char* pUnit = *(char**)(pNode + 0x18);
    if (!pUnit) return;

    // ---- UnitAny field offsets: not surfaced by Ghidra (placeholders) ----
    //   The decompile's symbolic names 'pAct' and 'dwSeed' do not carry
    //   their offsets; they MUST be filled in from the binary's
    //   disassembly. The values below are best-guesses against common
    //   D2 layouts and are NOT verified.
    const int UNIT_PACT_OFFSET   = 0x10;   // placeholder: UnitAny.pAct
    const int UNIT_DWSEED_OFFSET = 0x5C;   // placeholder: UnitAny.dwSeed

    // pAVar2 = pUVar1->pAct;
    char* pAct = *(char**)(pUnit + UNIT_PACT_OFFSET);

    // *(undefined1 *)((int)pUVar1->dwSeed + 7) = 1;
    int dwSeed = *(int*)(pUnit + UNIT_DWSEED_OFFSET);
    *(char*)((char*)dwSeed + 7) = 1;

    // pUVar1->pAct = (Act *)((int)pAVar2->_1 + nDelta);
    //   `_1` is Ghidra's anonymous-inner-field notation; the offset is
    //   unknown.  Common case: offset 0x1 within Act -- verify.
    int newPact = *(int*)(pAct + 0x1) + nDelta;
    *(int*)(pUnit + UNIT_PACT_OFFSET) = newPact;
}
