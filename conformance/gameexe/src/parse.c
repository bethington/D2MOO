/* Byte-verified reconstructions of PD2-S12 Game.exe launcher functions.
 *
 * Every function here compiles, under VS2003 cl /O2, to bytes IDENTICAL to
 * the original (relocation sites masked) -- see manifest.json and verify
 * with verify_bytematch.py. This is a reconstruction of 1.13c, kept
 * separate from D2MOO's source/Game, which is a 1.10f semantic port.
 *
 * Do not "clean up" the shapes below. Each one is load-bearing: the
 * signedness of a counter, the position of an increment, and whether a
 * helper exists at all are all visible in the emitted code.
 */

typedef unsigned int size_t;
char * __cdecl strcpy(char *, const char *);
size_t __cdecl strlen(const char *);
#pragma intrinsic(strcpy, strlen)

/* The separator set is a 4-element char array, NOT a string literal: the
 * original's inner loop ends `CMP EDX,4`, and a literal would carry its
 * NUL and scan 5. Keeping this as a separate function matters -- it is
 * inlined at both call sites below, and the array initialisation is then
 * hoisted out of each enclosing loop, which is why the original stores the
 * four bytes TWICE and holds ':' in AL across both. A single shared
 * initialisation compiles to 145 bytes against the original's 170.
 */
static int IsSeparator(char c)
{
    char sep[4];
    int k;
    sep[0] = ' '; sep[1] = '\t'; sep[2] = '\n'; sep[3] = ':';
    for (k = 0; k < 4; k++) if (sep[k] == c) return 1;
    return 0;
}

/* Game.exe 0x004079d0 -- 170 bytes, 0 relocations, EXACT.
 *
 * Trims leading separators and copies through to the first trailing one.
 * Note the two scans read DIFFERENT buffers: the first walks the caller's
 * string, the second the local copy. Both bounds are the signed strlen
 * (`JL`/`JGE`, not `JB`/`JAE`), and `i++` happens BEFORE the store -- a
 * for-increment lands after the body and costs 5 bytes of ordering.
 */
void __stdcall TrimWhitespaceDelimiters(char *s)
{
    char szBuf[24];
    int len, i, j;

    strcpy(szBuf, s);
    len = (int)strlen(s);

    for (i = 0; i < len; i++) {
        if (!IsSeparator(s[i])) break;
    }

    j = 0;
    while (i < len) {
        char c = szBuf[i];
        if (IsSeparator(c)) break;
        i++;
        s[j++] = c;
    }
    s[j] = 0;
}
