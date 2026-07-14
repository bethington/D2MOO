#pragma once

#include "D2CommonDefinitions.h"

struct UnitAny;

#pragma pack(1)

struct MissileStream
{
	int32_t* unk0x00;							//0x00
	int32_t unk0x04;							//0x04
};

#pragma pack()

// Note: MISSSTREAM functions are all unreferenced except for MISSTREAM_FreeMissileStream which is probably just a security.

//D2Common.0x6FDBC230 (#11213)
D2COMMON_DLL_DECL void __stdcall MISSTREAM_AllocMissileStream(UnitAny* pMissile);
//D2Common.0x6FDBC280 (#11214)
D2COMMON_DLL_DECL void __stdcall MISSTREAM_FreeMissileStream(UnitAny* pMissile);
//D2Common.0x6FDBC2E0 (#11215)
D2COMMON_DLL_DECL void __stdcall MISSTREAM_ExecuteHit(UnitAny* pUnit, int nCollisionMask, int nCollisionPattern, void (__fastcall* pfnHit)(UnitAny*, UnitAny*));
//D2Common.0x6FDBC3B0
int __fastcall MISSTREAM_Return1(UnitAny* pUnit1, void* pUnit2);
//D2Common.0x6FDBC3C0 (#11216)
D2COMMON_DLL_DECL void __stdcall MISSTREAM_Update(UnitAny* a1, UnitAny* pMissile, int a3, int a4, int (__fastcall* pfCreate)(UnitAny*, int, int, int, int, int), void (__fastcall* a6)(int));
