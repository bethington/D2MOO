#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: QUEST_DispatchSkillHandler

// Helper functions - provided by game runtime / resolver
extern "C" void* __fastcall CreateUnitAtPosition(uint32_t skillId, void* unk, void* pGame, void* pUnit, int nPosX, int nPosY, int inEAX);
extern "C" void __fastcall ProcessMonsterAICallbacks(int arg, void* pGame);
extern "C" void __fastcall SKILLS_SelectRandomSkillForMonster(void* pThis, int arg);
extern "C" void __fastcall QUEST_AddToQuestList(int arg1, void* arg2);
extern "C" void __fastcall QUEST_ProcessSkillEvent();
extern "C" void __fastcall Unwind_6fd179c0();

// Offset of pQuestChainList in Game struct (best guess - typical D2Game layout)
#define QUEST_CHAIN_LIST_OFFSET 0x110
// Offset of pNext in QuestListNode (piVar1[0x3d] = piVar1 + 0x3d*4 = piVar1 + 0xF4)
#define QUEST_NODE_NEXT_OFFSET 0xF4

extern "C" void* __fastcall QUEST_DispatchSkillHandler(
    int nPosY, int nPosX,
    void* pGame, void* pUnit,
    uint32_t dwSkillId, uint16_t wQuestId,
    int nFlag1, int nFlag2,
    uint16_t wData, char* szSkillName)
{
    // Step 1: Create skill state unit at position
    void* pThis = CreateUnitAtPosition(dwSkillId, (void*)0, pGame, pUnit, nPosX, nPosY, 0);
    if (pThis == (void*)0) return (void*)0;

    // Step 2: Check creation success flag
    if (*(int*)pThis != 1) return pThis;

    // Step 3: Check pSkillModBuffer exists and +0x1c is non-null
    void* pSkillModBuf = *(void**)((char*)pThis + 0x14);
    if (pSkillModBuf == (void*)0) return pThis;
    if ((char*)((int)pSkillModBuf + 0x1c) == (char*)0) return pThis;

    // Step 4: Set/clear bit 2 of skill flags byte at +0x16 based on nFlag1
    if (pSkillModBuf != (void*)0) {
        if (nFlag1 == 0) {
            *(uint8_t*)((char*)pSkillModBuf + 0x16) = *(uint8_t*)((char*)pSkillModBuf + 0x16) & (uint8_t)0xfb;
        } else {
            *(uint8_t*)((char*)pSkillModBuf + 0x16) = *(uint8_t*)((char*)pSkillModBuf + 0x16) | (uint8_t)0x04;
        }
    }

    // Step 5: Copy up to 9 skill name chars to +0x1c..+0x24 (0x1E sentinel terminates shorter names)
    char c0 = szSkillName[0];
    *(char*)((char*)pSkillModBuf + 0x1c) = c0;
    char c1 = szSkillName[1];
    *(char*)((char*)pSkillModBuf + 0x1d) = c1;
    char c2 = szSkillName[2];
    *(char*)((char*)pSkillModBuf + 0x1e) = c2;
    char c3 = szSkillName[3];
    *(char*)((char*)pSkillModBuf + 0x1f) = c3;
    char c4 = szSkillName[4];
    *(char*)((char*)pSkillModBuf + 0x20) = c4;
    char c5 = szSkillName[5];
    *(char*)((char*)pSkillModBuf + 0x21) = c5;
    char c6 = szSkillName[6];
    *(char*)((char*)pSkillModBuf + 0x22) = c6;
    char c7 = szSkillName[7];
    *(char*)((char*)pSkillModBuf + 0x23) = c7;
    char c8 = szSkillName[8];
    *(char*)((char*)pSkillModBuf + 0x24) = c8;

    // Step 6: Call ProcessMonsterAICallbacks(0, pGame)
    ProcessMonsterAICallbacks(0, pGame);

    // Set wQuestId into pSkillModBuffer+0x14
    if ((*(int*)pThis == 1) && (*(void**)((char*)pThis + 0x14) != (void*)0)) {
        *(uint16_t*)((char*)pSkillModBuf + 0x14) = wQuestId;
    }

    // Step 7: If any name char == 0x1E, call SelectRandomSkillForMonster
    if ((c0 == (char)0x1e) || (c1 == (char)0x1e) || (c2 == (char)0x1e) || (c3 == (char)0x1e) ||
        (c4 == (char)0x1e) || (c5 == (char)0x1e) || (c6 == (char)0x1e) || (c7 == (char)0x1e) ||
        (c8 == (char)0x1e)) {
        SKILLS_SelectRandomSkillForMonster(pThis, 1);
    }

    // Step 8: If nFlag2 == 0, return early
    if (nFlag2 == 0) return pThis;

    // nFlag2 != 0: set bit 1 of skill flags and write wData at +0x26
    if (*(int*)pThis == 1) {
        if (*(void**)((char*)pThis + 0x14) != (void*)0) {
            uint8_t* pbSkillFlags = (uint8_t*)((char*)pSkillModBuf + 0x16);
            *pbSkillFlags = *pbSkillFlags | (uint8_t)0x02;
        }
        if ((*(int*)pThis == 1) && (*(void**)((char*)pThis + 0x14) != (void*)0)) {
            *(uint16_t*)((char*)pSkillModBuf + 0x26) = wData;
        }
    }

    // Step 9: Switch on (wData - 6) with valid range 0..54 (wData: 6..60)
    void* pChainList;
    int* piVar1;

    switch (wData) {
        case 6: {  // Den of Evil (quest ID 5)
            Unwind_6fd179c0();
            pChainList = *(void**)((char*)pGame + QUEST_CHAIN_LIST_OFFSET);
            if (pChainList != (void*)0) {
                piVar1 = *(int**)((char*)pChainList + 0);
                if (piVar1 != (int*)0) {
                    while (*piVar1 != 5) {
                        piVar1 = *(int**)((char*)piVar1 + QUEST_NODE_NEXT_OFFSET);
                        if (piVar1 == (int*)0) break;
                    }
                    if (piVar1 != (int*)0 && *piVar1 == 5) {
                        QUEST_AddToQuestList(0, pThis);
                        return pThis;
                    }
                }
            }
            break;
        }

        case 0x1a:
        case 0x1b:
        case 0x1d: {  // Sisters (quest ID 0x13)
            pChainList = *(void**)((char*)pGame + QUEST_CHAIN_LIST_OFFSET);
            if (pChainList != (void*)0) {
                piVar1 = *(int**)((char*)pChainList + 0);
                if (piVar1 != (int*)0) {
                    while (*piVar1 != 0x13) {
                        piVar1 = *(int**)((char*)piVar1 + QUEST_NODE_NEXT_OFFSET);
                        if (piVar1 == (int*)0) break;
                    }
                    if (piVar1 != (int*)0 && *piVar1 == 0x13) {
                        QUEST_AddToQuestList((int)pGame, 0);
                        return pThis;
                    }
                }
            }
            break;
        }

        case 0x24:
        case 0x25:
        case 0x26: {  // Khalim's Way (quest ID 0x17)
            pChainList = *(void**)((char*)pGame + QUEST_CHAIN_LIST_OFFSET);
            if (pChainList == (void*)0) return pThis;
            piVar1 = *(int**)((char*)pChainList + 0);
            if (piVar1 == (int*)0) return pThis;
            while (*piVar1 != 0x17) {
                piVar1 = *(int**)((char*)piVar1 + QUEST_NODE_NEXT_OFFSET);
                if (piVar1 == (int*)0) return pThis;
            }
            QUEST_AddToQuestList(0, pThis);
            return pThis;
        }

        case 0x27: {  // ProcessSkillEvent
            QUEST_ProcessSkillEvent();
            return pThis;
        }

        case 0x2a: {  // Arcane Sanctuary (quest ID 0x1F)
            Unwind_6fd179c0();
            pChainList = *(void**)((char*)pGame + QUEST_CHAIN_LIST_OFFSET);
            if (pChainList == (void*)0) return pThis;
            piVar1 = *(int**)((char*)pChainList + 0);
            if (piVar1 == (int*)0) return pThis;
            while (*piVar1 != 0x1f) {
                piVar1 = *(int**)((char*)piVar1 + QUEST_NODE_NEXT_OFFSET);
                if (piVar1 == (int*)0) return pThis;
            }
            QUEST_AddToQuestList(0, pThis);
            break;
        }

        case 0x2b:
        case 0x2c:
        case 0x2d: {  // Hollows (quest ID 0x23)
            pChainList = *(void**)((char*)pGame + QUEST_CHAIN_LIST_OFFSET);
            if (pChainList != (void*)0) {
                piVar1 = *(int**)((char*)pChainList + 0);
                if (piVar1 != (int*)0) {
                    while (*piVar1 != 0x23) {
                        piVar1 = *(int**)((char*)piVar1 + QUEST_NODE_NEXT_OFFSET);
                        if (piVar1 == (int*)0) break;
                    }
                    if (piVar1 != (int*)0 && *piVar1 == 0x23) {
                        QUEST_AddToQuestList(0, pThis);
                        return pThis;
                    }
                }
            }
            break;
        }

        case 0x3c: {  // Temple of the Triad (quest ID 0x22)
            pChainList = *(void**)((char*)pGame + QUEST_CHAIN_LIST_OFFSET);
            if (pChainList != (void*)0) {
                piVar1 = *(int**)((char*)pChainList + 0);
                if (piVar1 != (int*)0) {
                    while (*piVar1 != 0x22) {
                        piVar1 = *(int**)((char*)piVar1 + QUEST_NODE_NEXT_OFFSET);
                        if (piVar1 == (int*)0) break;
                    }
                    if (piVar1 != (int*)0 && *piVar1 == 0x22) {
                        QUEST_AddToQuestList(0, pThis);
                        return pThis;
                    }
                }
            }
            break;
        }
    }

    // Step 10: Return skill state node
    return pThis;
}
