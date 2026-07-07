// unit_getmode.cpp -- LIVE GAME-OBJECT demo (stateful frontier). Prove a getter
// against a REAL live unit captured from gameplay.
//
// UNIT_GetMode (PD2 D2Common 0x34870) -- __stdcall(UnitAny* pUnit) -> byte at
// +0x90 (asm ground truth: MOV EAX,[ESP+4]; MOV AL,[EAX+0x90]; RET 0x4 -- so
// stdcall, arg on stack, BYTE return). The oracle passes the live unit captured
// by D2Debugger.capture.cpp as a "handle" arg; original and reimpl both read
// +0x90 of the SAME pointer back-to-back, so they agree regardless of which unit
// it is or its mode value -- proving the oracle can drive a function with a live
// game object. (The byte return leaves stale upper EAX bits in the original; the
// oracle compares ret as "u8", low byte only.)

// D2MOO_REIMPL_EXPORT: UNIT_GetMode
extern "C" int __stdcall UNIT_GetMode(void* pUnit)
{
	return *((unsigned char*)pUnit + 0x90);
}
