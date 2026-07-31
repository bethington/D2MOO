#include "../provider_runtime.h"

extern "C" void __cdecl DATATBLS_TruncateString(char* str, uint32_t len);
extern "C" char* __cdecl STRING_CopyString(char* dst, const char* src);

// D2MOO_REIMPL_EXPORT: DATATBLS_FormatScientificNotation
extern "C" void* __cdecl DATATBLS_FormatScientificNotation(
    char* szBuffer, int nFormat, int nPrecision, int* pnCharCode, char cSignFlag)
{
    char* szWriteHead;
    char* szDecimalPos;
    char* szExpNotation;
    int nExponentAdj;

    char* decPtBase = (char*)D2MOO_Resolve("g_bLocaleDecimalPoint");
    if (!decPtBase) return (void*)(intptr_t)0;
    char g_cDecimalPoint = *decPtBase;

    if (cSignFlag != '\0') {
        DATATBLS_TruncateString(szBuffer + (*pnCharCode == 0x2d), (uint32_t)(0 < nFormat));
    }
    szWriteHead = szBuffer;
    if (*pnCharCode == 0x2d) {
        *szBuffer = '-';
        szWriteHead = szBuffer + 1;
    }
    szDecimalPos = szWriteHead;
    if (0 < nFormat) {
        szDecimalPos = szWriteHead + 1;
        *szWriteHead = szWriteHead[1];
        *szDecimalPos = g_cDecimalPoint;
    }
    szExpNotation = STRING_CopyString(szDecimalPos + nFormat + (uint32_t)(cSignFlag == '\0'), "e+000");
    if (nPrecision != 0) {
        *szExpNotation = 'E';
    }
    if (*((char*)pnCharCode + 12) != '0') {
        nExponentAdj = pnCharCode[1] + -1;
        if (nExponentAdj < 0) {
            nExponentAdj = -nExponentAdj;
            szExpNotation[1] = '-';
        }
        if (99 < nExponentAdj) {
            szExpNotation[2] = szExpNotation[2] + (char)(nExponentAdj / 100);
            nExponentAdj = nExponentAdj % 100;
        }
        if (9 < nExponentAdj) {
            szExpNotation[3] = szExpNotation[3] + (char)(nExponentAdj / 10);
            nExponentAdj = nExponentAdj % 10;
        }
        szExpNotation[4] = szExpNotation[4] + (char)nExponentAdj;
    }
    return (void*)szBuffer;
}
