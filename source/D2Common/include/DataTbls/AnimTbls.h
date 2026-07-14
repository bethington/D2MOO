#pragma once 

#include <D2Common.h>
#include <D2BasicTypes.h>
#include <Archive.h>

#pragma pack(1)

struct AnimDataRecord
{
	static const int MAX_FRAME_FLAGS = 144;
	char szAnimDataName[8];						//0x00
	uint32_t dwFrames;							//0x08
	int32_t dwAnimSpeed;						//0x0C
	uint8_t pFrameFlags[MAX_FRAME_FLAGS];		//0x10
};

struct AnimDataBucket
{
	int32_t nbEntries;
	AnimDataRecord aEntries[]; // Dynamically sized
};

struct AnimDataTable
{
	void* pBinaryData;							//0x00
	AnimDataBucket* pHashTableBucket[256];//0x04
	AnimDataRecord tDefaultRecord;		//0x404
};

struct UnitAny;
struct Inventory;

#pragma pack()

//D2Common.0x6FD473C0
AnimDataTable* __fastcall DATATBLS_LoadAnimDataD2(HD2ARCHIVE hArchive);
//D2Common.0x6FD47460
void __fastcall DATATBLS_UnloadAnimDataD2(AnimDataTable* pAnimData);
//D2Common.0x6FD474A0
const AnimDataRecord* __fastcall DATATBLS_GetAnimDataRecord(UnitAny* pUnit, int nClassId, int nMode, int nUnitType, Inventory* pInventory);
//D2Common.0x6FD475D0 (#10640)
D2COMMON_DLL_DECL void __stdcall DATATBLS_UnitAnimInfoDebugSet(UnitAny* pUnit, int nAnimSpeed);
//D2Common.0x6FD47700 (#10641)
D2COMMON_DLL_DECL BOOL __stdcall DATATBLS_GetAnimDataInfo(char* szPath, int* pOutLength, int* pOutAnimSpeed, int* pOutFirstFrameTagged);


