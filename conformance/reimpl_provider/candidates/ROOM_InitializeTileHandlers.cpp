#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ROOM_InitializeTileHandlers
extern "C" void __stdcall ROOM_InitializeTileHandlers(void)
{
    // Pure initializer: assigns function pointer values to global tile handler slots.
    // All target globals and their function symbols are missing from the resolver.
    // NEEDS GLOBAL: g_pfnClearTileData, g_pfnDrawTileClear, g_pfnTileEncodeDispatch,
    //                g_pfnTileEncodeCallback, g_pfnTileLayerHandler3, g_pfnTileLayerHandler4,
    //                g_pfnTileLayerHandler5, g_pfnTileLayerHandler6, g_pfnTileLayerHandlerB,
    //                g_pfnTileLayerHandlerE, g_pfnRoomTileHandler3, g_pfnRoomTileHandler4,
    //                g_pfnRoomTileHandler1, g_pfnRoomTileHandler2, g_pfnRoomTileHandler5,
    //                g_pfnRoomTileHandler6, g_pfnRoomTileHandler8, g_pfnRoomTileHandlerB,
    //                g_pfnRoomTileHandlerD, g_pfnRoomTileHandlerE, g_pfnRenderTileLayer,
    //                g_pfnReadTileData, g_pfnInitTileLayer, g_pfnPreDrawTile, g_pfnRenderFlush
    // NEEDS GLOBAL: ROOM_ApplyTileBicubicFilter, IJL_ProcessDCT8x8Block, InitializeContext,
    //                WriteBufferedStream, ROOM_ConvertTileData, TileLayerHandler4_Entry,
    //                TileLayerHandler5_Entry, IJL_ProcessYCbCrTile, ApplyDCT8x8Transform,
    //                ROOM_SelectTileDataHandler, DRLG_TransformTile2x2ColorData,
    //                ROOM_DecodeTileDeltaColor, DELTA_ConvertYCbCrBlock, RoomTileHandler2_Entry,
    //                RoomTileHandler5_Entry, DELTA_DecodeYCbCrOrPaletteBlock, DELTA_YCbCrToBGRA,
    //                RoomTileHandlerB_Entry, DATATBLS_ExpandDeltaBlock, ANIM_CopyDeltaToPixelBuffers,
    //                HUFF_DecodeSymbols, HUFFMAN_DecodeTileData, InitTileLayer_Entry,
    //                MultiplyArraysFixedPoint, g_pfnRenderFlush_Stub

    void** slot;
    void* fn;

    slot = (void**)D2MOO_Resolve("g_pfnClearTileData");
    fn = (void*)D2MOO_Resolve("ROOM_ApplyTileBicubicFilter");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnDrawTileClear");
    fn = (void*)D2MOO_Resolve("IJL_ProcessDCT8x8Block");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnTileEncodeDispatch");
    fn = (void*)D2MOO_Resolve("InitializeContext");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnTileEncodeCallback");
    fn = (void*)D2MOO_Resolve("WriteBufferedStream");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnTileLayerHandler3");
    fn = (void*)D2MOO_Resolve("ROOM_ConvertTileData");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnTileLayerHandler4");
    fn = (void*)D2MOO_Resolve("TileLayerHandler4_Entry");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnTileLayerHandler5");
    fn = (void*)D2MOO_Resolve("TileLayerHandler5_Entry");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnTileLayerHandler6");
    fn = (void*)D2MOO_Resolve("IJL_ProcessYCbCrTile");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnTileLayerHandlerB");
    fn = (void*)D2MOO_Resolve("ApplyDCT8x8Transform");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnTileLayerHandlerE");
    fn = (void*)D2MOO_Resolve("ROOM_SelectTileDataHandler");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnRoomTileHandler3");
    fn = (void*)D2MOO_Resolve("DRLG_TransformTile2x2ColorData");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnRoomTileHandler4");
    fn = (void*)D2MOO_Resolve("ROOM_DecodeTileDeltaColor");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnRoomTileHandler1");
    fn = (void*)D2MOO_Resolve("DELTA_ConvertYCbCrBlock");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnRoomTileHandler2");
    fn = (void*)D2MOO_Resolve("RoomTileHandler2_Entry");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnRoomTileHandler5");
    fn = (void*)D2MOO_Resolve("RoomTileHandler5_Entry");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnRoomTileHandler6");
    fn = (void*)D2MOO_Resolve("DELTA_DecodeYCbCrOrPaletteBlock");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnRoomTileHandler8");
    fn = (void*)D2MOO_Resolve("DELTA_YCbCrToBGRA");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnRoomTileHandlerB");
    fn = (void*)D2MOO_Resolve("RoomTileHandlerB_Entry");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnRoomTileHandlerD");
    fn = (void*)D2MOO_Resolve("DATATBLS_ExpandDeltaBlock");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnRoomTileHandlerE");
    fn = (void*)D2MOO_Resolve("ANIM_CopyDeltaToPixelBuffers");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnRenderTileLayer");
    fn = (void*)D2MOO_Resolve("HUFF_DecodeSymbols");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnReadTileData");
    fn = (void*)D2MOO_Resolve("HUFFMAN_DecodeTileData");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnInitTileLayer");
    fn = (void*)D2MOO_Resolve("InitTileLayer_Entry");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnPreDrawTile");
    fn = (void*)D2MOO_Resolve("MultiplyArraysFixedPoint");
    if (slot && fn) *slot = fn;

    slot = (void**)D2MOO_Resolve("g_pfnRenderFlush");
    fn = (void*)D2MOO_Resolve("g_pfnRenderFlush_Stub");
    if (slot && fn) *slot = fn;
}
