// D2MOO_REIMPL_EXPORT: FindRoomByCoordinates
#include "../provider_runtime.h"

// Shadow-leaf: pRoom is a live struct base; offsets double-dereferenced for sub-room array.
// Layout (offsets from disassembly):
//   +0x00: ppSubRoomArray (FindRoomByCoordinatesRoom **)
//   +0x24: dwSubRoomCount (uint)
//   +0x4C: nPosX    (int)
//   +0x50: nPosY    (int)
//   +0x54: nSizeX   (int)
//   +0x58: nSizeY   (int)
#pragma pack(push, 1)
struct FindRoomByCoordinatesRoom {
    FindRoomByCoordinatesRoom** ppSubRoomArray;   // +0x00
    uint8_t                _pad1[0x20];     // +0x04..+0x23
    uint32_t                 dwSubRoomCount;  // +0x24
    uint8_t                _pad2[0x24];     // +0x28..+0x4B
    int                          nPosX;           // +0x4C
    int                          nPosY;           // +0x50
    int                          nSizeX;          // +0x54
    int                          nSizeY;          // +0x58
};
#pragma pack(pop)

// ABI (register-explicit from ground-truth disassembly):
//   EAX = pRoom (this)
//   EBX = nSearchX
//   EDI = nSearchY
//   ECX, ESI are callee-saved scratch (PUSH/POP at entry/exit; their input values are unused).
//   No stack args. Returns EAX (room ptr) or NULL.
extern "C" FindRoomByCoordinatesRoom* __cdecl FindRoomByCoordinates(
    FindRoomByCoordinatesRoom* pRoom,
    int nSearchX,
    int nSearchY)
{
    // 1. NULL guard (mirrors TEST EAX,EAX / JNZ early)
    if (pRoom == (FindRoomByCoordinatesRoom*)0) {
        return (FindRoomByCoordinatesRoom*)0;
    }

    // 2. Main-room bounds: four SIGNED compares (JL/JGE on disasm)
    //    In-bounds iff nSearchX >= nPosX && nSearchX < (nPosX+nSizeX)
    //                   && nSearchY >= nPosY && nSearchY < (nPosY+nSizeY)
    if (nSearchX >= pRoom->nPosX && nSearchX < pRoom->nPosX + pRoom->nSizeX &&
        nSearchY >= pRoom->nPosY && nSearchY < pRoom->nPosY + pRoom->nSizeY) {
        // Falls through JL at 0x6fd51355 -> 0x6fd5139e (POP ECX; RET) with EAX=pRoom
        return pRoom;
    }

    // 3. Sub-room scan (mirrors MOV EDX,[EAX+0x24]; MOV ECX,[EAX]; loop)
    uint32_t count = pRoom->dwSubRoomCount;
    FindRoomByCoordinatesRoom** arr = pRoom->ppSubRoomArray;

    // JBE on TEST EDX,EDX -> skip loop if count == 0
    for (uint32_t i = 0; i < count; i++) {
        FindRoomByCoordinatesRoom* sub = arr[i];   // MOV ESI,[ECX+EAX*4]
        if (sub == (FindRoomByCoordinatesRoom*)0) continue;  // TEST ESI,ESI; JZ

        // Sub-room bounds: same four signed compares
        if (nSearchX >= sub->nPosX && nSearchX < sub->nPosX + sub->nSizeX &&
            nSearchY >= sub->nPosY && nSearchY < sub->nPosY + sub->nSizeY) {
            // Matched: disasm returns [ECX+EAX*4] = arr[i] = sub. Same value.
            return sub;
        }
        // else INC EAX, CMP with saved count, JC loop
    }

    // 4. No match: XOR EAX,EAX path at 0x6fd5139b
    return (FindRoomByCoordinatesRoom*)0;
}
