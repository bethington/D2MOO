// get_coord_pair_from_struct.cpp -- 15th production-loop reimpl (2026-07-07).
//
// GetCoordPairFromStruct (PD2 0x36710, ordinal #10320): void __fastcall(CoordPair*
// pStruct, int* pnOutX, int* pnOutY). fastcall arity-3 (pStruct->ECX, pnOutX->EDX,
// pnOutY->stack). Reads nX/nY from the source struct, writes to the two SEPARATE
// output pointers -- read-only through pStruct, pure otherwise. The getter
// counterpart to SetCoordPair (which packs x/y INTO a struct); this unpacks.
// D2MOO_REIMPL_EXPORT: GetCoordPairFromStruct
extern "C" void __fastcall GetCoordPairFromStruct(int* pStruct, int* pnOutX, int* pnOutY)
{
	*pnOutX = pStruct[0];
	*pnOutY = pStruct[1];
}
