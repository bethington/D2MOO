#include "../provider_runtime.h"

// Forward declarations for game functions invoked by CreateQuestItem
extern "C" int Unwind_6fd179c0(void);
extern "C" int SKILLS_GetLevel(void*);
extern "C" void* ITEMS_CreateItemWithLevel(int, int, void*, uint32_t, void*, int, uint32_t, int, void*, void*);
extern "C" uint32_t ITEMS_PickupGroundItem(void*, void*, uint32_t);
extern "C" void ROOM_FindPlacementPosition(int, int*, int*, void*);
extern "C" void InitializePlayerState(void*);
extern "C" void ForEachPlayerUnit(void);
extern "C" void CleanupUnitByType(void*, void*);
extern "C" void ITEMS_ProcessItemInteractionWithSkill(void);

// D2MOO_REIMPL_EXPORT: CreateQuestItem
// __fastcall: ECX=dwQuestUnitId, EDX=pPlayer, [ESP+4]=pGame, [ESP+8]=dwLevelArg, [ESP+C]=retaddr
// RET 0xc => 3 stack params cleaned. Mirrors Ghidra signature + ABI.
extern "C" void* __fastcall CreateQuestItem(uint32_t dwQuestUnitId, void* pPlayer, void* pGame, uint32_t dwLevelArg, uint32_t retaddr)
{
    // 1. Session validity gate
    int nResult = Unwind_6fd179c0();
    if (nResult != 0) {
        // 2. Touch skill-level hook (result unused in source, kept verbatim)
        SKILLS_GetLevel((void*)0);

        // 3. Create the quest item at level=4, mode=0x9CF, quality=0
        void* pUnit = ITEMS_CreateItemWithLevel(
            1, 0, pPlayer, dwLevelArg, pGame,
            4, retaddr, 0, (void*)0, (void*)0);
        if (pUnit != (void*)0) {
            // 4. Re-verify session, iterate if needed
            int iVar1 = Unwind_6fd179c0();
            if (0 < iVar1) {
                Unwind_6fd179c0();
            }
            Unwind_6fd179c0();

            // 5. Try to give the item directly to the player (mode=0x9D2 / quality=12 hints)
            uint32_t uVar2 = ITEMS_PickupGroundItem(
                pGame, pPlayer,
                *(uint32_t*)((char*)pUnit + 0x08)); // dwUnitId offset in UnitAny
            if (uVar2 != 0) {
                uint32_t dwResult = Unwind_6fd179c0();
                if (dwResult == 0) {
                    ITEMS_ProcessItemInteractionWithSkill();
                }
                return pUnit;
            }

            // 6. nRoomCheck branch (stack-local, uninitialised in decompile -> gate by a
            //    resolved sentinel so we never take a path the original can't).
            //    nFlag=1 indicates room-placement mode; nRoomCheck gates FindRoomPlacement.
            uint32_t nRoomCheck = *(uint32_t*)((char*)pPlayer + 0x00); // placeholder gate
            if (nRoomCheck != 0) {
                Unwind_6fd179c0();
                int* pPosY = (int*)Unwind_6fd179c0();
                int nLevelListIdx = 0;
                int* pPosX = (int*)0;
                ROOM_FindPlacementPosition(nLevelListIdx, pPosX, pPosY, pPlayer);
                InitializePlayerState(pUnit);
                return pUnit;
            }

            // 7. Fallback: cleanup the orphaned unit
            ForEachPlayerUnit();
            CleanupUnitByType(pGame, pUnit);
        }
    }
    return (void*)0;
}
