#include "../provider_runtime.h"

// Forward declaration - this CLIENT_ function is reimpl'd elsewhere in the provider
extern "C" uint8_t __stdcall CLIENT_GetCursorStateByGameMode(void);

// D2MOO_REIMPL_EXPORT: CLIENT_IsCursorInSkillBarButton
extern "C" int __stdcall CLIENT_IsCursorInSkillBarButton(int nCursorX, int nCursorY)
{
    int* pArrangeMode = (int*)D2MOO_Resolve("g_nArrangeMode");
    if (!pArrangeMode) return 0;
    uint32_t* pScreenWidth = (uint32_t*)D2MOO_Resolve("g_dwScreenWidth");
    if (!pScreenWidth) return 0;
    int* pButtonPosX = (int*)D2MOO_Resolve("g_dwButtonPosX");
    if (!pButtonPosX) return 0;
    int* pRightBtnX = (int*)D2MOO_Resolve("g_dwRightBtnX");
    if (!pRightBtnX) return 0;
    int* pRightBtnRowY = (int*)D2MOO_Resolve("g_anRightBtnRowY");
    if (!pRightBtnRowY) return 0;
    int* pRightBtnRow0Y = (int*)D2MOO_Resolve("g_nRightBtnRow0Y");
    if (!pRightBtnRow0Y) return 0;
    uint32_t* pBottomPanelBaseY = (uint32_t*)D2MOO_Resolve("g_dwBottomPanelBaseY");
    if (!pBottomPanelBaseY) return 0;

    int iVar3 = *pArrangeMode;
    uint8_t bVar1 = CLIENT_GetCursorStateByGameMode();

    int iVar2;
    if (iVar3 == 2) { iVar2 = (int)(*pScreenWidth - 0x280u) / 2; }
    else { iVar2 = 0; }

    int iVar4 = (int)bVar1 * 0x14;

    int iVar3_re;
    if (iVar3 == 2) { iVar3_re = (int)(*pScreenWidth - 0x280u) / 2; }
    else { iVar3_re = 0; }

    // X bounds check (strict)
    int leftX = *(int*)(pButtonPosX + iVar4) + iVar2;
    if (!(leftX < nCursorX)) return 0;
    int rightX = (int)(*(int*)(pRightBtnX + (int)bVar1 * 5)) + iVar3_re;
    if (!(nCursorX < rightX)) return 0;

    // Y bounds check (strict) - matches (int)(int_val + (-0x1e0) + DWORD_val) via unsigned promotion
    int bottomY_base = *(int*)(pRightBtnRowY + iVar4) + (-0x1e0);
    int bottomY = (int)((uint32_t)bottomY_base + *pBottomPanelBaseY);
    if (!(bottomY < nCursorY)) return 0;
    int topY_base = *(int*)(pRightBtnRow0Y + iVar4) + (-0x1e0);
    int topY = (int)((uint32_t)topY_base + *pBottomPanelBaseY);
    if (!(nCursorY < topY)) return 0;

    return 1;
}
