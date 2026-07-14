#pragma once

#include <cstdint>

#include "DrawMode.h"


struct GfxData;



//D2Gfx.0x6FA74270 (#10071)
D2GFX_DLL_DECL void __stdcall TEXTURE_CelFlatSpriteDraw(GfxData* pData, int32_t nXPos, int32_t nYPos, uint32_t dwGamma, DrawMode eDrawMode, int32_t nScreenMode, uint8_t* pPalette);
//D2Gfx.0x6FA742A0 (#10072)
D2GFX_DLL_DECL void __stdcall TEXTURE_CelDraw(GfxData* pData, int32_t nXPos, int32_t nYPos, uint32_t dwGamma, DrawMode eDrawMode, uint8_t* pPalette);
//D2Gfx.0x6FA742D0 (#10073)
D2GFX_DLL_DECL void __stdcall TEXTURE_CelDrawColor(GfxData* pData, int32_t nXPos, int32_t nYPos, uint32_t dwGamma, DrawMode eDrawMode, int32_t nGlobalPaletteShift);
//D2Gfx.0x6FA74300 (#10074)
D2GFX_DLL_DECL void __stdcall TEXTURE_CelDrawEx(GfxData* pData, int32_t nXPos, int32_t nYPos, int32_t nSkipLines, int32_t nDrawLines, DrawMode eDrawMode);
//D2Gfx.0x6FA74330 (#10077)
D2GFX_DLL_DECL void __stdcall TEXTURE_CelDrawClipped(GfxData* pData, int32_t nXPos, int32_t nYPos, void* pCropRect, DrawMode eDrawMode);
//D2Gfx.0x6FA74360 (#10075)
D2GFX_DLL_DECL void __stdcall TEXTURE_CelDrawShadow(GfxData* pData, int32_t nXPos, int32_t nYPos);
//D2Gfx.0x6FA74380 (#10076)
D2GFX_DLL_DECL void __stdcall TEXTURE_CelDrawHilight(GfxData* pData, int32_t nXPos, int32_t nYPos, uint8_t nPaletteIndex);
//D2Gfx.0x6FA743A0 (#10078)
D2GFX_DLL_DECL void __stdcall TEXTURE_DebugFillBackBuffer(int32_t nXPos, int32_t nYPos);
