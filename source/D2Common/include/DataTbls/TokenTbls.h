#pragma once 

#include <D2BasicTypes.h>

#pragma pack(1)

struct ArmTypeTxt
{
	char szName[32];					//0x00
	char szToken[20];					//0x20
};

struct PlrModeTypeTxt
{
	char szName[32];					//0x00
	char szToken[20];					//0x20
};

struct PlrModeDataTbl								//sgptDataTable + 0x10D4
{
	int nPlrModeTypeTxtRecordCount;					//0x00
	PlrModeTypeTxt* pPlrModeTypeTxt;				//0x04
	PlrModeTypeTxt* pPlayerType;					//0x08
	PlrModeTypeTxt* pPlayerMode;					//0x0C
};

struct MonModeTxt
{
	char szName[32];					//0x00
	uint32_t dwToken;					//0x20
	uint8_t nDTDir;						//0x24
	uint8_t nNUDir;						//0x25
	uint8_t nWLDir;						//0x26
	uint8_t nGHDir;						//0x27
	uint8_t nA1Dir;						//0x28
	uint8_t nA2Dir;						//0x29
	uint8_t nBLDir;						//0x2A
	uint8_t nSCDir;						//0x2B
	uint8_t nS1Dir;						//0x2C
	uint8_t nS2Dir;						//0x2D
	uint8_t nS3Dir;						//0x2E
	uint8_t nS4Dir;						//0x2F
	uint8_t nDDDir;						//0x30
	uint8_t nKBDir;						//0x31
	uint8_t nSQDir;						//0x32
	uint8_t nRNDir;						//0x33
};

struct MonModeDataTbl								//sgptDataTable + 0x10B0
{
	int nMonModeTxtRecordCount;						//0x00
	MonModeTxt* pMonModeTxt;						//0x04
	MonModeTxt* pMonMode[2];						//0x08
};

struct ObjModeTypeTxt
{
	char szName[32];					//0x00
	char szToken[20];					//0x20
};

struct ObjModeDataTbl								//sgptDataTable + 0x10C4
{
	int nObjModeTypeTxtRecordCount;					//0x00
	ObjModeTypeTxt* pObjModeTypeTxt;				//0x04
	ObjModeTypeTxt* pObjType;						//0x08
	ObjModeTypeTxt* pObjMode;						//0x0C
};


struct CompositTxt
{
	char szName[32];						//0x00
	char szToken[20];						//0x20
};

struct CompCodeTxt
{
	uint32_t dwCode;						//0x00
};

#pragma pack()

//D2Common.0x6FD729C0
void __fastcall DATATBLS_LoadPlrType_ModeTxt(HD2ARCHIVE hArchive);
//D2Common.0x6FD72B30
void __fastcall DATATBLS_LoadMonModeTxt(HD2ARCHIVE hArchive);
//D2Common.0x6FD72E50
void __fastcall DATATBLS_LoadObjType_ModeTxt(HD2ARCHIVE hArchive);
//D2Common.0x6FD72FC0
void __fastcall DATATBLS_LoadCompositTxt(HD2ARCHIVE hArchive);
//D2Common.0x6FD73040
void __fastcall DATATBLS_LoadArmTypeTxt(HD2ARCHIVE hArchive);
//D2Common.0x6FD730C0
void __fastcall DATATBLS_UnloadPlrMode_Type_MonMode_ObjMode_Type_Composit_ArmtypeTxt();
//D2Common.0x6FD73150 (#10643)
D2COMMON_DLL_DECL PlrModeDataTbl* __fastcall DATATBLS_GetPlrMode_TypeDataTables();
//D2Common.0x6FD73160 (#10644)
D2COMMON_DLL_DECL MonModeDataTbl* __fastcall DATATBLS_GetMonModeDataTables();
//D2Common.0x6FD73170 (#10645)
D2COMMON_DLL_DECL ObjModeDataTbl* __fastcall DATATBLS_GetObjMode_TypeDataTables();
//D2Common.0x6FD73180 (#10646)
D2COMMON_DLL_DECL PlrModeTypeTxt* __stdcall DATATBLS_GetPlrModeTypeTxtRecord(int nIndex, int bGetMode);
//D2Common.0x6FD73230 (#10647)
D2COMMON_DLL_DECL MonModeTxt* __stdcall DATATBLS_GetMonModeTxtRecord(int nIndex, int bGetMode);
//D2Common.0x6FD732B0 (#10648)
D2COMMON_DLL_DECL ObjModeTypeTxt* __stdcall DATATBLS_GetObjModeTypeTxtRecord(int nIndex, int bGetMode);
//D2Common.0x6FD73330 (#10649)
D2COMMON_DLL_DECL CompositTxt* __stdcall DATATBLS_GetCompositTxtRecord(int nComposit);
//D2Common.0x6FD73370 (#10650)
D2COMMON_DLL_DECL ArmTypeTxt* __stdcall DATATBLS_GetArmTypeTxtRecord(int nId);
