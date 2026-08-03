#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CheckMouseInLeftPanelButtonBounds
extern "C" int __stdcall CheckMouseInLeftPanelButtonBounds(void)
{
    char* p_screen_w = (char*)D2MOO_Resolve("g_dwScreenWidth");
    char* p_bottom_y = (char*)D2MOO_Resolve("g_dwBottomPanelBaseY");
    char* p_cursor_x = (char*)D2MOO_Resolve("g_dwCursorScreenX");
    char* p_cursor_y = (char*)D2MOO_Resolve("g_dwCursorScreenY");

    if (!p_screen_w || !p_bottom_y || !p_cursor_x || !p_cursor_y)
        return -1;

    int screen_w = (int)*(uint32_t*)p_screen_w;
    int half_w = screen_w / 2;
    int x_min = half_w + 0xa3;
    int x_max = half_w + 0xc5;

    uint32_t bottom_y_val = *(uint32_t*)p_bottom_y;
    int y_min = (int)(bottom_y_val - 0x2au);
    int y_max = (int)(bottom_y_val - 8u);

    int cursor_x = *(int*)p_cursor_x;
    int cursor_y = *(int*)p_cursor_y;

    if ((x_min < cursor_x) && (cursor_x < x_max)) {
        if ((y_min < cursor_y) && (cursor_y < y_max)) {
            return 1;
        }
    }
    return 0;
}
