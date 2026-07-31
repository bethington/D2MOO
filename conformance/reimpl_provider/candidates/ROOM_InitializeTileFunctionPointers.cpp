#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ROOM_InitializeTileFunctionPointers
// NEEDS GLOBAL: g_pfnClearTileData, g_pfnDrawTileClear, g_pfnTileEncodeDispatch,
// NEEDS GLOBAL: g_pfnTileEncodeCallback, g_pfnTileLayerHandler3, g_pfnTileLayerHandler4,
// NEEDS GLOBAL: g_pfnTileLayerHandler5, g_pfnTileLayerHandler6, g_pfnTileLayerHandlerB,
// NEEDS GLOBAL: g_pfnTileLayerHandlerE, g_pfnRoomTileHandler3, g_pfnRoomTileHandler4,
// NEEDS GLOBAL: g_pfnRoomTileHandler1, g_pfnRoomTileHandler2, g_pfnRoomTileHandler5,
// NEEDS GLOBAL: g_pfnRoomTileHandler6, g_pfnRoomTileHandler8, g_pfnRoomTileHandlerB,
// NEEDS GLOBAL: g_pfnRoomTileHandlerD, g_pfnRoomTileHandlerE, g_pfnRenderTileLayer,
// NEEDS GLOBAL: g_pfnReadTileData, g_pfnInitTileLayer, g_pfnPreDrawTile, g_pfnRenderFlush

extern "C" void __stdcall ROOM_InitializeTileFunctionPointers(void)
{
	void* pfn;

	pfn = D2MOO_Resolve("TransformMatrix4x4Saturating");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnClearTileData") = pfn;

	pfn = D2MOO_Resolve("ROOM_ProcessTileButterfly4x4");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnDrawTileClear") = pfn;

	pfn = D2MOO_Resolve("IJL_InitializeTableEntry");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnTileEncodeDispatch") = pfn;

	pfn = D2MOO_Resolve("AppendEncodedBytes");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnTileEncodeCallback") = pfn;

	pfn = D2MOO_Resolve("TileLayerHandler3_Entry");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnTileLayerHandler3") = pfn;

	pfn = D2MOO_Resolve("TileLayerHandler4_Entry");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnTileLayerHandler4") = pfn;

	pfn = D2MOO_Resolve("TileLayerHandler5_Entry");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnTileLayerHandler5") = pfn;

	pfn = D2MOO_Resolve("IJL_ProcessYCbCrTile");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnTileLayerHandler6") = pfn;

	pfn = D2MOO_Resolve("IJL_DecodeImageBlock");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnTileLayerHandlerB") = pfn;

	pfn = D2MOO_Resolve("ROOM_SelectTileDataHandler");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnTileLayerHandlerE") = pfn;

	pfn = D2MOO_Resolve("DRLG_TransformTileColorData4");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnRoomTileHandler3") = pfn;

	pfn = D2MOO_Resolve("INTERP_DecodeYuvFrame");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnRoomTileHandler4") = pfn;

	pfn = D2MOO_Resolve("ConvertDeltaToRGB888");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnRoomTileHandler1") = pfn;

	pfn = D2MOO_Resolve("DELTA_ValidateAndApplyTransform");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnRoomTileHandler2") = pfn;

	pfn = D2MOO_Resolve("RoomTileHandler5_Entry");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnRoomTileHandler5") = pfn;

	pfn = D2MOO_Resolve("DELTA_DecodeYCbCrOrPaletteBlock");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnRoomTileHandler6") = pfn;

	pfn = D2MOO_Resolve("RoomTileHandler8_Entry");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnRoomTileHandler8") = pfn;

	pfn = D2MOO_Resolve("PIXEL_TransformDeltaPixelBlocks");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnRoomTileHandlerB") = pfn;

	pfn = D2MOO_Resolve("DATATBLS_ExpandDeltaBlock");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnRoomTileHandlerD") = pfn;

	pfn = D2MOO_Resolve("ANIM_CopyDeltaToPixelBuffers");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnRoomTileHandlerE") = pfn;

	pfn = D2MOO_Resolve("HUFF_DecompressBlock");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnRenderTileLayer") = pfn;

	pfn = D2MOO_Resolve("IJL_DecodeHuffmanSymbols");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnReadTileData") = pfn;

	pfn = D2MOO_Resolve("HUFF_DecodeSymbol");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnInitTileLayer") = pfn;

	pfn = D2MOO_Resolve("ROOM_PreDrawTileTransform");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnPreDrawTile") = pfn;

	pfn = D2MOO_Resolve("IjlClearMmxState");
	if (pfn) *(void**)D2MOO_Resolve("g_pfnRenderFlush") = pfn;
}
