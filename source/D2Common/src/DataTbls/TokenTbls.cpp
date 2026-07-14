#include "D2DataTbls.h"

#include "D2Composit.h"
#include "D2Skills.h"
#include "Units/Units.h"



//D2Common.0x6FD729C0
void __fastcall DATATBLS_LoadPlrType_ModeTxt(HD2ARCHIVE hArchive)
{
	PlrModeTypeTxt* pPlrModeTypeTxt = NULL;
	PlrModeTypeTxt* pPlrType = NULL;
	PlrModeTypeTxt* pPlrMode = NULL;
	int nTypeRecords = 0;
	int nModeRecords = 0;

	BinField pTbl[] =
	{
		{ "name", TXTFIELD_ASCII, 31, 0, NULL },
		{ "token", TXTFIELD_RAW, 0, 32, NULL },
		{ "end", TXTFIELD_NONE, 0, 0, NULL }
	};

	pPlrType = (PlrModeTypeTxt*)DATATBLS_CompileTxt(hArchive, "plrtype", pTbl, &nTypeRecords, sizeof(PlrModeTypeTxt));
	pPlrMode = (PlrModeTypeTxt*)DATATBLS_CompileTxt(hArchive, "plrmode", pTbl, &nModeRecords, sizeof(PlrModeTypeTxt));

	sgptDataTables->pPlrModeDataTables.nPlrModeTypeTxtRecordCount = nModeRecords + nTypeRecords;

	pPlrModeTypeTxt = (PlrModeTypeTxt*)D2_ALLOC_POOL(nullptr, sizeof(PlrModeTypeTxt) * (nModeRecords + nTypeRecords));
	D2_ASSERT(pPlrModeTypeTxt);

	sgptDataTables->pPlrModeDataTables.pPlrModeTypeTxt = pPlrModeTypeTxt;
	sgptDataTables->pPlrModeDataTables.pPlayerType = pPlrModeTypeTxt;
	sgptDataTables->pPlrModeDataTables.pPlayerMode = pPlrModeTypeTxt + nTypeRecords;

	memcpy(sgptDataTables->pPlrModeDataTables.pPlayerType, pPlrType, sizeof(PlrModeTypeTxt) * nTypeRecords);
	memcpy(sgptDataTables->pPlrModeDataTables.pPlayerMode, pPlrMode, sizeof(PlrModeTypeTxt) * nModeRecords);

	DATATBLS_UnloadBin(pPlrType);
	DATATBLS_UnloadBin(pPlrMode);
}

//D2Common.0x6FD72B30
void __fastcall DATATBLS_LoadMonModeTxt(HD2ARCHIVE hArchive)
{
	BinField pTbl[] =
	{
		{ "name", TXTFIELD_ASCII, 31, 0, NULL },
		{ "token", TXTFIELD_RAW, 0, 32, NULL },
		{ "DT_Dir", TXTFIELD_UNKNOWN2, 0, 36, NULL },
		{ "NU_Dir", TXTFIELD_UNKNOWN2, 0, 37, NULL },
		{ "WL_Dir", TXTFIELD_UNKNOWN2, 0, 38, NULL },
		{ "GH_Dir", TXTFIELD_UNKNOWN2, 0, 39, NULL },
		{ "A1_Dir", TXTFIELD_UNKNOWN2, 0, 40, NULL },
		{ "A2_Dir", TXTFIELD_UNKNOWN2, 0, 41, NULL },
		{ "BL_Dir", TXTFIELD_UNKNOWN2, 0, 42, NULL },
		{ "SC_Dir", TXTFIELD_UNKNOWN2, 0, 43, NULL },
		{ "S1_Dir", TXTFIELD_UNKNOWN2, 0, 44, NULL },
		{ "S2_Dir", TXTFIELD_UNKNOWN2, 0, 45, NULL },
		{ "S3_Dir", TXTFIELD_UNKNOWN2, 0, 46, NULL },
		{ "S4_Dir", TXTFIELD_UNKNOWN2, 0, 47, NULL },
		{ "DD_Dir", TXTFIELD_UNKNOWN2, 0, 48, NULL },
		{ "KB_Dir", TXTFIELD_UNKNOWN2, 0, 49, NULL },
		{ "SQ_Dir", TXTFIELD_UNKNOWN2, 0, 50, NULL },
		{ "RN_Dir", TXTFIELD_UNKNOWN2, 0, 51, NULL },
		{ "end", TXTFIELD_NONE, 0, 0, NULL },
	};

	sgptDataTables->pMonModeDataTables.pMonModeTxt = (MonModeTxt*)DATATBLS_CompileTxt(hArchive, "monmode", pTbl, &sgptDataTables->pMonModeDataTables.nMonModeTxtRecordCount, sizeof(MonModeTxt));

	sgptDataTables->pMonModeDataTables.pMonMode[0] = sgptDataTables->pMonModeDataTables.pMonModeTxt;
	sgptDataTables->pMonModeDataTables.pMonMode[1] = sgptDataTables->pMonModeDataTables.pMonModeTxt;
}

//D2Common.0x6FD72E50
void __fastcall DATATBLS_LoadObjType_ModeTxt(HD2ARCHIVE hArchive)
{
	ObjModeTypeTxt* pObjModeTypeTxt = NULL;

	ObjModeTypeTxt* pObjType = NULL;
	ObjModeTypeTxt* pObjMode = NULL;
	int nTypeRecords = 0;
	int nModeRecords = 0;

	BinField pTbl[] =
	{
		{ "name", TXTFIELD_ASCII, 31, 0, NULL },
		{ "token", TXTFIELD_RAW, 0, 32, NULL },
		{ "end", TXTFIELD_NONE, 0, 0, NULL }
	};

	pObjType = (ObjModeTypeTxt*)DATATBLS_CompileTxt(hArchive, "objtype", pTbl, &nTypeRecords, sizeof(ObjModeTypeTxt));
	pObjMode = (ObjModeTypeTxt*)DATATBLS_CompileTxt(hArchive, "objmode", pTbl, &nModeRecords, sizeof(ObjModeTypeTxt));

	sgptDataTables->pObjModeDataTables.nObjModeTypeTxtRecordCount = nModeRecords + nTypeRecords;

	pObjModeTypeTxt = (ObjModeTypeTxt*)D2_ALLOC_POOL(nullptr, sizeof(ObjModeTypeTxt) * (nModeRecords + nTypeRecords));
	D2_ASSERT(pObjModeTypeTxt);

	sgptDataTables->pObjModeDataTables.pObjModeTypeTxt = pObjModeTypeTxt;
	sgptDataTables->pObjModeDataTables.pObjType = pObjModeTypeTxt;
	sgptDataTables->pObjModeDataTables.pObjMode = (ObjModeTypeTxt*)((char*)pObjModeTypeTxt + sizeof(ObjModeTypeTxt) * nTypeRecords);

	memcpy(sgptDataTables->pObjModeDataTables.pObjType, pObjType, sizeof(ObjModeTypeTxt) * nTypeRecords);
	memcpy(sgptDataTables->pObjModeDataTables.pObjMode, pObjMode, sizeof(ObjModeTypeTxt) * nModeRecords);
	DATATBLS_UnloadBin(pObjType);
	DATATBLS_UnloadBin(pObjMode);
}

//D2Common.0x6FD72FC0
void __fastcall DATATBLS_LoadCompositTxt(HD2ARCHIVE hArchive)
{
	BinField pTbl[] =
	{
		{ "name", TXTFIELD_ASCII, 31, 0, NULL },
		{ "token", TXTFIELD_RAW, 0, 32, NULL },
		{ "end", TXTFIELD_NONE, 0, 0, NULL }
	};

	sgptDataTables->pCompositTxt = (CompositTxt*)DATATBLS_CompileTxt(hArchive, "composit", pTbl, NULL, sizeof(CompositTxt));
}

//D2Common.0x6FD73040
void __fastcall DATATBLS_LoadArmTypeTxt(HD2ARCHIVE hArchive)
{
	BinField pTbl[] =
	{
		{ "name", TXTFIELD_ASCII, 31, 0, NULL },
		{ "token", TXTFIELD_RAW, 0, 32, NULL },
		{ "end", TXTFIELD_NONE, 0, 0, NULL }
	};

	sgptDataTables->pArmTypeTxt = (ArmTypeTxt*)DATATBLS_CompileTxt(hArchive, "armtype", pTbl, NULL, sizeof(ArmTypeTxt));
}

//D2Common.0x6FD730C0
void __fastcall DATATBLS_UnloadPlrMode_Type_MonMode_ObjMode_Type_Composit_ArmtypeTxt()
{
	if (sgptDataTables->pPlrModeDataTables.pPlrModeTypeTxt)
	{
		D2_FREE_POOL(nullptr, sgptDataTables->pPlrModeDataTables.pPlrModeTypeTxt);
		sgptDataTables->pPlrModeDataTables.pPlrModeTypeTxt = NULL;
	}

	if (sgptDataTables->pMonModeDataTables.pMonModeTxt)
	{
		DATATBLS_UnloadBin(sgptDataTables->pMonModeDataTables.pMonModeTxt);
		sgptDataTables->pMonModeDataTables.pMonModeTxt = NULL;
	}

	if (sgptDataTables->pObjModeDataTables.pObjModeTypeTxt)
	{
		D2_FREE_POOL(nullptr, sgptDataTables->pObjModeDataTables.pObjModeTypeTxt);
		sgptDataTables->pObjModeDataTables.pObjModeTypeTxt = NULL;
	}

	if (sgptDataTables->pCompositTxt)
	{
		DATATBLS_UnloadBin(sgptDataTables->pCompositTxt);
		sgptDataTables->pCompositTxt = NULL;
	}

	if (sgptDataTables->pArmTypeTxt)
	{
		DATATBLS_UnloadBin(sgptDataTables->pArmTypeTxt);
		sgptDataTables->pArmTypeTxt = NULL;
	}
}

//D2Common.0x6FD73150 (#10643)
PlrModeDataTbl* __fastcall DATATBLS_GetPlrMode_TypeDataTables()
{
	return &sgptDataTables->pPlrModeDataTables;
}

//D2Common.0x6FD73160 (#10644)
MonModeDataTbl* __fastcall DATATBLS_GetMonModeDataTables()
{
	return &sgptDataTables->pMonModeDataTables;
}

//D2Common.0x6FD73170 (#10645)
ObjModeDataTbl* __fastcall DATATBLS_GetObjMode_TypeDataTables()
{
	return &sgptDataTables->pObjModeDataTables;
}

//D2Common.0x6FD73180 (#10646)
PlrModeTypeTxt* __stdcall DATATBLS_GetPlrModeTypeTxtRecord(int nIndex, int bGetMode)
{
	if (nIndex >= sgptDataTables->pPlrModeDataTables.nPlrModeTypeTxtRecordCount)
	{
		return NULL;
	}

	D2_ASSERT(sgptDataTables->pPlrModeDataTables.pPlrModeTypeTxt);

	if (bGetMode)
	{
		if (bGetMode == 1)
		{
			D2_ASSERT(&sgptDataTables->pPlrModeDataTables.pPlayerMode[nIndex]);
			return &sgptDataTables->pPlrModeDataTables.pPlayerMode[nIndex];
		}
		return NULL;
	}

	D2_ASSERT(&sgptDataTables->pPlrModeDataTables.pPlayerType[nIndex]);

	return &sgptDataTables->pPlrModeDataTables.pPlayerType[nIndex];
}

//D2Common.0x6FD73230 (#10647)
MonModeTxt* __stdcall DATATBLS_GetMonModeTxtRecord(int nIndex, int bGetMode)
{
	if (nIndex >= sgptDataTables->pMonModeDataTables.nMonModeTxtRecordCount)
	{
		return NULL;
	}

	if (bGetMode)
	{
		if (bGetMode == 1)
		{
			D2_ASSERT(&sgptDataTables->pMonModeDataTables.pMonMode[1][nIndex]);
			return &sgptDataTables->pMonModeDataTables.pMonMode[1][nIndex];
		}
		return NULL;
	}

	D2_ASSERT(&sgptDataTables->pMonModeDataTables.pMonMode[0][nIndex]);

	return &sgptDataTables->pMonModeDataTables.pMonMode[0][nIndex];
}

//D2Common.0x6FD732B0 (#10648)
ObjModeTypeTxt* __stdcall DATATBLS_GetObjModeTypeTxtRecord(int nIndex, int bGetMode)
{
	if (nIndex >= sgptDataTables->pObjModeDataTables.nObjModeTypeTxtRecordCount)
	{
		return NULL;
	}

	if (bGetMode)
	{
		if (bGetMode == 1)
		{
			D2_ASSERT(&sgptDataTables->pObjModeDataTables.pObjMode[nIndex]);
			return &sgptDataTables->pObjModeDataTables.pObjMode[nIndex];
		}
		return NULL;
	}

	D2_ASSERT(&sgptDataTables->pObjModeDataTables.pObjType[nIndex]);
	return &sgptDataTables->pObjModeDataTables.pObjType[nIndex];
}

//D2Common.0x6FD73330 (#10649)
CompositTxt* __stdcall DATATBLS_GetCompositTxtRecord(int nComposit)
{
	D2_ASSERT(&sgptDataTables->pCompositTxt[nComposit]);

	return &sgptDataTables->pCompositTxt[nComposit];
}

//D2Common.0x6FD73370 (#10650)
ArmTypeTxt* __stdcall DATATBLS_GetArmTypeTxtRecord(int nId)
{
	D2_ASSERT(&sgptDataTables->pArmTypeTxt[nId]);
	return &sgptDataTables->pArmTypeTxt[nId];
}
