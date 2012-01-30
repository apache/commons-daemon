/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "apxwin.h"
#include "private.h"

APX_OSLEVEL _st_apx_oslevel = APX_WINVER_UNK;

/* Apache's APR stripped Os level detection */
APX_OSLEVEL apxGetOsLevel()
{
    if (_st_apx_oslevel == APX_WINVER_UNK) {
        static OSVERSIONINFO oslev;
        oslev.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&oslev);

        if (oslev.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            if (oslev.dwMajorVersion < 4)
                _st_apx_oslevel = APX_WINVER_UNSUP;
            else if (oslev.dwMajorVersion == 4)
               _st_apx_oslevel = APX_WINVER_NT_4;
            else if (oslev.dwMajorVersion == 5) {
                if (oslev.dwMinorVersion == 0)
                    _st_apx_oslevel = APX_WINVER_2000;
                else
                    _st_apx_oslevel = APX_WINVER_XP;
            }
            else
                _st_apx_oslevel = APX_WINVER_XP;
        }
#ifndef WINNT
        else if (oslev.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
            if (oslev.dwMinorVersion < 10)
                _st_apx_oslevel = APX_WINVER_95;
            else if (oslev.dwMinorVersion < 90)
                _st_apx_oslevel = APX_WINVER_98;
            else
                _st_apx_oslevel = APX_WINVER_ME;
        }
#endif
#ifdef _WIN32_WCE
        else if (oslev.dwPlatformId == VER_PLATFORM_WIN32_CE) {
            if (oslev.dwMajorVersion < 3)
                _st_apx_oslevel = APX_WINVER_UNSUP;
            else
                _st_apx_oslevel = APX_WINVER_CE_3;
        }
#endif
        else
            _st_apx_oslevel = APX_WINVER_UNSUP;
    }

    if (_st_apx_oslevel < APX_WINVER_UNSUP)
        return APX_WINVER_UNK;
    else
        return _st_apx_oslevel;
}

LPWSTR __apxGetEnvironmentVariableW(APXHANDLE hPool, LPCWSTR wsName)
{
    LPWSTR wsRet;
    DWORD  rc;

    rc = GetEnvironmentVariableW(wsName, NULL, 0);
    if (rc == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND)
        return NULL;

    if (!(wsRet = apxPoolAlloc(hPool, (rc + 1) * sizeof(WCHAR))))
        return NULL;
    if (!GetEnvironmentVariableW(wsName, wsRet, rc)) {
        apxLogWrite(APXLOG_MARK_SYSERR);
        apxFree(wsRet);
        return NULL;
    }
    return wsRet;
}

LPSTR __apxGetEnvironmentVariableA(APXHANDLE hPool, LPCSTR szName)
{
    LPSTR szRet;
    DWORD rc;

    rc = GetEnvironmentVariableA(szName, NULL, 0);
    if (rc == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND)
        return NULL;

    if (!(szRet = apxPoolAlloc(hPool, rc + 1)))
        return NULL;
    if (!GetEnvironmentVariableA(szName, szRet, rc)) {
        apxLogWrite(APXLOG_MARK_SYSERR);
        apxFree(szRet);
        return NULL;
    }
    return szRet;
}

BOOL apxAddToPathW(APXHANDLE hPool, LPCWSTR szAdd)
{
    LPWSTR wsAdd;
    DWORD  rc;
    DWORD  al;
    
    rc = GetEnvironmentVariableW(L"PATH", NULL, 0);
    if (rc == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND)
        return FALSE;
    al = lstrlenW(szAdd) + 6;
    if (!(wsAdd = apxPoolAlloc(hPool, (al + rc + 1) * sizeof(WCHAR))))
        return FALSE;
    lstrcpyW(wsAdd, L"PATH=");        
    lstrcatW(wsAdd, szAdd);        
    lstrcatW(wsAdd, L";");        
    if (!GetEnvironmentVariableW(L"PATH", wsAdd + al, rc - al)) {
        apxLogWrite(APXLOG_MARK_SYSERR);
        apxFree(wsAdd);
        return FALSE;
    }
    SetEnvironmentVariableW(L"PATH", wsAdd + 5);
    _wputenv(wsAdd);
    apxFree(wsAdd);
    return TRUE;
}

LPWSTR AsciiToWide(LPCSTR s, LPWSTR ws)
{
    LPWSTR pszSave = ws;

    if (!s) {
        *ws = L'\0';
        return pszSave;
    }
    do {
        *ws++ = (WCHAR)*s;
    } while (*s++);
    return pszSave;
}

LPSTR WideToANSI(LPCWSTR ws)
{

    LPSTR s;
    int cch = WideCharToMultiByte(CP_ACP, 0, ws, -1, NULL, 0, NULL, NULL);
    s = (LPSTR)apxAlloc(cch);
    if (!WideCharToMultiByte(CP_ACP, 0, ws, -1, s, cch, NULL, NULL)) {
        apxFree(s);
        return NULL;
    }
    return s;
}

LPWSTR ANSIToWide(LPCSTR cs)
{

    LPWSTR s;
    int cch = MultiByteToWideChar(CP_ACP, 0, cs, -1, NULL, 0);
    s = (LPWSTR)apxAlloc(cch * sizeof(WCHAR));
    if (!MultiByteToWideChar(CP_ACP, 0, cs, -1, s, cch)) {
        apxFree(s);
        return NULL;
    }
    return s;
}

LPSTR MzWideToAscii(LPCWSTR ws, LPSTR s)
{
    LPSTR pszSave = s;

    if (ws) {
        do {
            *s++ = (CHAR)*ws;
            ws++;
        } while( *ws || *(ws + 1));
    }
    /* double terminate */
    *s++ = '\0';
    *s   = '\0';
    return pszSave;
}

LPSTR MzWideToANSI(LPCWSTR ws)
{
    LPSTR str;
    LPSTR s;
    LPCWSTR p = ws;
    int cch = 0;

    for ( ; p && *p; p++) {
        int len = WideCharToMultiByte(CP_ACP, 0, p, -1, NULL, 0, NULL, NULL);
        if (len > 0)
            cch += len;
        while (*p)
            p++;
    }
    cch ++;
    str = s = (LPSTR)apxAlloc(cch + 1);

    p = ws;
    for ( ; p && *p; p++) {
        int len = WideCharToMultiByte(CP_ACP, 0, p, -1, s, cch, NULL, NULL);
        if (len > 0) {
            s = s + len;
            cch -= len;
        }
        while (*p)
            p++;
    }
    /* double terminate */
    *s = '\0';
    return str;
}

DWORD __apxGetMultiSzLengthA(LPCSTR lpStr, LPDWORD lpdwCount)
{
    LPCSTR p = lpStr;
    if (lpdwCount)
        *lpdwCount = 0;
    for ( ; p && *p; p++) {
        if (lpdwCount)
            *lpdwCount += 1;
        while (*p)
            p++;
    }
    return (DWORD)(p - lpStr);
}

DWORD __apxGetMultiSzLengthW(LPCWSTR lpStr, LPDWORD lpdwCount)
{
    LPCWSTR p = lpStr;
    if (lpdwCount)
        *lpdwCount = 0;
    for ( ; p && *p; p++) {
        if (lpdwCount)
            *lpdwCount += 1;
        while (*p)
            p++;
    }
    return (DWORD)((p - lpStr));
}

LPWSTR apxMultiSzCombine(APXHANDLE hPool, LPCWSTR lpStrA, LPCWSTR lpStrB,
                         LPDWORD lpdwLength)
{
    LPWSTR rv;
    DWORD  la = 0, lb = 0;
    if (!lpStrA && !lpStrB)
        return NULL;    /* Nothing to do if both are NULL */

    la = __apxGetMultiSzLengthW(lpStrA, NULL);
    lb = __apxGetMultiSzLengthW(lpStrB, NULL);

    rv = apxPoolCalloc(hPool, (la + lb + 1) * sizeof(WCHAR));
    if (la) {
        AplMoveMemory(rv, lpStrA, la * sizeof(WCHAR));
    }
    if (lb) {
        AplMoveMemory(&rv[la], lpStrB, lb * sizeof(WCHAR));
    }
    if (lpdwLength)
        *lpdwLength = (la + lb + 1) * sizeof(WCHAR);
    return rv;
}

BOOL
apxSetEnvironmentVariable(APXHANDLE hPool, LPCTSTR szName, LPCTSTR szValue,
                          BOOL bAppend)
{
    LPTSTR szNew = (LPTSTR)szValue;

    if (bAppend) {
        DWORD l = GetEnvironmentVariable(szName, NULL, 0);
        if (l > 0) {
            BOOL rv;
            if (IS_INVALID_HANDLE(hPool))
                szNew = apxAlloc(l + lstrlen(szValue) + 3);
            else
                szNew = apxPoolAlloc(hPool, l + lstrlen(szValue) + 3);
            GetEnvironmentVariable(szName, szNew, l + 1);
            lstrcat(szNew, TEXT(";"));
            lstrcat(szNew, szValue);
            rv = SetEnvironmentVariable(szName, szNew);
            apxFree(szNew);
            return rv;
        }
    }
    return SetEnvironmentVariable(szName, szNew);
}


/** Convert null separated double null terminated string to LPTSTR array)
 * returns array size
 */
DWORD
apxMultiSzToArrayW(APXHANDLE hPool, LPCWSTR lpString, LPWSTR **lppArray)
{
    DWORD i, n, l;
    char *buff;
    LPWSTR p;

    l = __apxGetMultiSzLengthW(lpString, &n);
    if (!n || !l)
        return 0;
    if (IS_INVALID_HANDLE(hPool))
        buff = apxPoolAlloc(hPool, (n + 2) * sizeof(LPWSTR) + (l + 1) * sizeof(WCHAR));
    else
        buff = apxAlloc((n + 2) * sizeof(LPWSTR) + (l + 1) * sizeof(WCHAR));

    *lppArray = (LPWSTR *)buff;
    p = (LPWSTR)(buff + (n + 2) * sizeof(LPWSTR));
    AplCopyMemory(p, lpString, (l + 1) * sizeof(WCHAR) + sizeof(WCHAR));
    for (i = 0; i < n; i++) {
        (*lppArray)[i] = p;
        while (*p)
            p++;
        p++;
    }
    (*lppArray)[++i] = NULL;

    return n;
}

DWORD
apxMultiSzToArrayA(APXHANDLE hPool, LPCSTR lpString, LPSTR **lppArray)
{
    DWORD i, n, l;
    char *buff;
    LPSTR p;

    l = __apxGetMultiSzLengthA(lpString, &n);
    if (!n || !l)
        return 0;
    if (IS_INVALID_HANDLE(hPool))
        buff = apxPoolAlloc(hPool, (n + 2) * sizeof(LPTSTR) + (l + 1));
    else
        buff = apxAlloc((n + 2) * sizeof(LPSTR) + (l + 1) * sizeof(CHAR));

    *lppArray = (LPSTR *)buff;
    p = (LPSTR)(buff + (n + 2) * sizeof(LPSTR));
    AplCopyMemory(p, lpString, (l + 1) * sizeof(CHAR) + sizeof(CHAR));
    for (i = 0; i < n; i++) {
        (*lppArray)[i] = p;
        while (*p)
            p++;
        p++;
    }
    (*lppArray)[++i] = NULL;

    return n;
}

#define QSTR_BOUNDARY 127
#define QSTR_ALIGN(size) \
    (((size) + QSTR_BOUNDARY + sizeof(APXMULTISZ) + 2) & ~(QSTR_BOUNDARY))

#define QSTR_SIZE(size) \
    ((QSTR_ALIGN(size)) - sizeof(APXMULTISZ))

#define QSTR_DATA(q)    ((char *)(q) + sizeof(APXMULTISZ))

#if 0
LPAPXMULTISZ apxMultiSzStrdup(LPCTSTR szSrc)
{
    LPAPXMULTISZ q;

    if (szSrc) {
        DWORD l = lstrlen(szSrc);
        q = (LPAPXMULTISZ)apxAlloc(QSTR_ALIGN(l));
        q->dwAllocated = QSTR_SIZE(l);
        q->dwInsert    = l + 1;
        AplMoveMemory(QSTR_DATA(q), szSrc, l);
        RtlZeroMemory(QSTR_DATA(q) + l, q->dwAllocated - l);
    }
    else {
        q = (LPAPXMULTISZ)apxCalloc(QSTR_ALIGN(0));
        q->dwAllocated = QSTR_SIZE(0);
    }

    return q;
}

LPTSTR  apxMultiSzStrcat(LPAPXMULTISZ lpmSz, LPCTSTR szSrc)
{
    DWORD l = lstrlen(szSrc);
    LPTSTR p;

    if (lpmSz->dwInsert + l + 2 > lpmSz->dwAllocated) {
        if ((lpmSz = (LPAPXMULTISZ )apxRealloc(lpmSz, QSTR_ALIGN(lpmSz->dwInsert + l))) == NULL)
            return NULL;

        lpmSz->dwAllocated = QSTR_SIZE(lpmSz->dwInsert + l);
        AplZeroMemory(QSTR_DATA(lpmSz) + lpmSz->dwInsert + l,
                   lpmSz->dwAllocated - (lpmSz->dwInsert + l));
    }
    p = (LPTSTR)QSTR_DATA(lpmSz) + lpmSz->dwInsert;
    AplMoveMemory(p, szSrc, l);

    lpmSz->dwInsert += (l + 1);
    return p;
}

DWORD apxMultiSzLen(LPAPXMULTISZ lpmSz)
{
    if (lpmSz->dwInsert)
        return lpmSz->dwInsert - 1;
    else
        return 0;
}

LPCTSTR apxMultiSzGet(LPAPXMULTISZ lpmSz)
{
    return (LPCTSTR)QSTR_DATA(lpmSz);
}
#endif

LPTSTR apxStrCharRemove(LPTSTR szString, TCHAR chSkip)
{
  LPTSTR p = szString;
  LPTSTR q = szString;
  if (IS_EMPTY_STRING(szString))
    return szString;
  while (*p) {
    if(*p != chSkip)
      *q++ = *p;
    ++p;
  }
  *q = TEXT('\0');

  return szString;
}

DWORD apxStrCharRemoveA(LPSTR szString, CHAR chSkip)
{
  LPSTR p = szString;
  LPSTR q = szString;
  DWORD c = 0;
  if (IS_EMPTY_STRING(szString))
    return c;
  while (*p) {
    if(*p != chSkip)
      *q++ = *p;
    else
        ++c;
    ++p;
  }
  *q = '\0';

  return c;
}

DWORD apxStrCharRemoveW(LPWSTR szString, WCHAR chSkip)
{
  LPWSTR p = szString;
  LPWSTR q = szString;
  DWORD  c = 0;
  if (IS_EMPTY_STRING(szString))
    return c;
  while (*p) {
    if(*p != chSkip)
      *q++ = *p;
    else
        ++c;
    ++p;
  }
  *q = L'\0';

  return c;
}

void
apxStrCharReplaceA(LPSTR szString, CHAR chReplace, CHAR chReplaceWith)
{
  LPSTR p = szString;
  LPSTR q = szString;
  
  if (IS_EMPTY_STRING(szString))
    return;
  while (*p) {
    if(*p == chReplace)
      *q++ = chReplaceWith;
    else
      *q++ = *p;
    ++p;
  }
  *q = '\0';
}

void
apxStrCharReplaceW(LPWSTR szString, WCHAR chReplace, WCHAR chReplaceWith)
{
  LPWSTR p = szString;
  LPWSTR q = szString;
  if (IS_EMPTY_STRING(szString))
    return;
  while (*p) {
    if(*p == chReplace)
      *q++ = chReplaceWith;
    else
      *q++ = *p;
    ++p;
  }
  *q = L'\0';
}

static const LPCTSTR _st_hex = TEXT("0123456789abcdef");
#define XTOABUFFER_SIZE (sizeof(ULONG) * 2 + 2)
#define UTOABUFFER_SIZE (sizeof(ULONG_PTR) * 2 + 2)
#define LO_NIBLE(x)     ((BYTE)((x) & 0x0000000F))

BOOL apxUltohex(ULONG n, LPTSTR lpBuff, DWORD dwBuffLength)
{
    LPTSTR p;
    DWORD  i;
    *lpBuff = 0;
    if (dwBuffLength < XTOABUFFER_SIZE)
        return FALSE;
    p = lpBuff + XTOABUFFER_SIZE;
    *p-- = 0;
    for (i = 0; i < sizeof(ULONG) * 2; i++) {
        *p-- = _st_hex[LO_NIBLE(n)];
        n = n >> 4;
    }
    *p-- = TEXT('x');
    *p = TEXT('0');
    return TRUE;
}

BOOL apxUptohex(ULONG_PTR n, LPTSTR lpBuff, DWORD dwBuffLength)
{
    LPTSTR p;
    DWORD  i;
    *lpBuff = 0;
    if (dwBuffLength < UTOABUFFER_SIZE)
        return FALSE;
    p = lpBuff + UTOABUFFER_SIZE;
    *p-- = 0;
    for (i = 0; i < sizeof(ULONG_PTR) * 2; i++) {
        *p-- = _st_hex[LO_NIBLE(n)];
        n = n >> 4;
    }
    *p-- = TEXT('x');
    *p = TEXT('0');
    return TRUE;
}

ULONG apxStrToul(LPCTSTR szNum)
{
    ULONG rv = 0;
    DWORD sh = 0;
    LPCTSTR p = szNum;
    ++p;
    while (*p && (*p != TEXT('x')) && (*(p - 1) != TEXT('0')))
        p++;
    if (*p != 'x')
        return 0;
    /* go to the last digit */
    while (*(p + 1)) p++;

    /* go back to 'x' */
    while (*p != TEXT('x')) {
        ULONG v = 0;
        switch (*p--) {
            case TEXT('0'): v = 0UL; break;
            case TEXT('1'): v = 1UL; break;
            case TEXT('2'): v = 2UL; break;
            case TEXT('3'): v = 3UL; break;
            case TEXT('4'): v = 4UL; break;
            case TEXT('5'): v = 5UL; break;
            case TEXT('6'): v = 6UL; break;
            case TEXT('7'): v = 7UL; break;
            case TEXT('8'): v = 8UL; break;
            case TEXT('9'): v = 9UL; break;
            case TEXT('a'): case TEXT('A'): v = 10UL; break;
            case TEXT('b'): case TEXT('B'): v = 11UL; break;
            case TEXT('c'): case TEXT('C'): v = 12UL; break;
            case TEXT('d'): case TEXT('D'): v = 13UL; break;
            case TEXT('e'): case TEXT('E'): v = 14UL; break;
            case TEXT('f'): case TEXT('F'): v = 15UL; break;
            default:
                return 0;
            break;
        }
        rv |= rv + (v << sh);
        sh += 4;
    }
    return rv;
}

ULONG apxStrToulW(LPCWSTR szNum)
{
    ULONG rv = 0;
    DWORD sh = 0;
    LPCWSTR p = szNum;
    ++p;
    while (*p && (*p != L'x') && (*(p - 1) != L'0'))
        p++;
    if (*p != L'x')
        return 0;
    /* go to the last digit */
    while (*(p + 1)) p++;

    /* go back to 'x' */
    while (*p != L'x') {
        ULONG v = 0;
        switch (*p--) {
            case L'0': v = 0UL; break;
            case L'1': v = 1UL; break;
            case L'2': v = 2UL; break;
            case L'3': v = 3UL; break;
            case L'4': v = 4UL; break;
            case L'5': v = 5UL; break;
            case L'6': v = 6UL; break;
            case L'7': v = 7UL; break;
            case L'8': v = 8UL; break;
            case L'9': v = 9UL; break;
            case L'a': case L'A': v = 10UL; break;
            case L'b': case L'B': v = 11UL; break;
            case L'c': case L'C': v = 12UL; break;
            case L'd': case L'D': v = 13UL; break;
            case L'e': case L'E': v = 14UL; break;
            case L'f': case L'F': v = 15UL; break;
            default:
                return 0;
            break;
        }
        rv |= rv + (v << sh);
        sh += 4;
    }
    return rv;
}

ULONG apxAtoulW(LPCWSTR szNum)
{
    ULONG rv = 0;
    DWORD sh = 1;
    int   s  = 1;
    LPCWSTR p = szNum;

    /* go to the last digit */
    if (!p || !*p)
        return 0;
    if (*p == L'-') {
        s = -1;
        ++p;
    }
    while (*(p + 1)) p++;

    /* go back */
    while (p >= szNum) {
        ULONG v = 0;
        switch (*p--) {
            case L'0': v = 0UL; break;
            case L'1': v = 1UL; break;
            case L'2': v = 2UL; break;
            case L'3': v = 3UL; break;
            case L'4': v = 4UL; break;
            case L'5': v = 5UL; break;
            case L'6': v = 6UL; break;
            case L'7': v = 7UL; break;
            case L'8': v = 8UL; break;
            case L'9': v = 9UL; break;
            default:
                return rv * s;
            break;
        }
        rv = rv + (v * sh);
        sh = sh * 10;
    }
    return rv * s;
}

/* Make the unique system resource name from prefix and process id
 *
 */
BOOL
apxMakeResourceName(LPCTSTR szPrefix, LPTSTR lpBuff, DWORD dwBuffLength)
{
    DWORD pl = lstrlen(szPrefix);
    if (dwBuffLength < (pl + 11))
        return FALSE;
    lstrcpy(lpBuff, szPrefix);
    return apxUltohex(GetCurrentProcessId(), lpBuff + pl, dwBuffLength - pl);
}
/** apxStrMatchA ANSI string pattern matching
 * Match = 0, NoMatch = 1, Abort = -1
 * Based loosely on sections of wildmat.c by Rich Salz
 */
INT apxStrMatchA(LPCSTR szString, LPCSTR szPattern, BOOL bIgnoreCase)
{
    int x, y;

    for (x = 0, y = 0; szPattern[y]; ++y, ++x) {
        if (!szPattern[x] && (szPattern[y] != '*' || szPattern[y] != '?'))
            return -1;
        if (szPattern[y] == '*') {
            while (szPattern[++y] == '*');
            if (!szPattern[y])
                return 0;
            while (szString[x]) {
                INT rc;
                if ((rc = apxStrMatchA(&szString[x++], &szPattern[y],
                                       bIgnoreCase)) != 1)
                    return rc;
            }
            return -1;
        }
        else if (szPattern[y] != '?') {
            if (bIgnoreCase) {
                if (CharLowerA((LPSTR)((SIZE_T)szString[x])) !=
                    CharLowerA((LPSTR)((SIZE_T)szPattern[y])))
                    return 1;
            }
            else {
                if (szString[x] != szPattern[y])
                    return 1;
            }
        }
    }
    return (szString[x] != '\0');
}

INT apxStrMatchW(LPCWSTR szString, LPCWSTR szPattern, BOOL bIgnoreCase)
{
    int x, y;

    for (x = 0, y = 0; szPattern[y]; ++y, ++x) {
        if (!szPattern[x] && (szPattern[y] != L'*' || szPattern[y] != L'?'))
            return -1;
        if (szPattern[y] == L'*') {
            while (szPattern[++y] == L'*');
            if (!szPattern[y])
                return 0;
            while (szString[x]) {
                INT rc;
                if ((rc = apxStrMatchW(&szString[x++], &szPattern[y],
                                       bIgnoreCase)) != 1)
                    return rc;
            }
            return -1;
        }
        else if (szPattern[y] != L'?') {
            if (bIgnoreCase) {
                if (CharLowerW((LPWSTR)((SIZE_T)szString[x])) !=
                    CharLowerW((LPWSTR)((SIZE_T)szPattern[y])))
                    return 1;
            }
            else {
                if (szString[x] != szPattern[y])
                    return 1;
            }
        }
    }
    return (szString[x] != L'\0');
}

INT apxMultiStrMatchW(LPCWSTR szString, LPCWSTR szPattern,
                      WCHAR chSeparator, BOOL bIgnoreCase)
{
    WCHAR szM[SIZ_HUGLEN];
    DWORD i = 0;
    LPCWSTR p = szPattern;
    INT   m = -1;

    if (chSeparator == 0)
        return apxStrMatchW(szString, szPattern, bIgnoreCase);
    while (*p != L'\0') {
        if (*p == chSeparator) {
            m = apxStrMatchW(szString, szM, bIgnoreCase);
            if (m == 0)
                return 0;
            p++;
            i = 0;
            szM[0] = L'\0';
        }
        else {
            if (i < SIZ_HUGMAX)
                szM[i++] = *p++;
            else
                return -1;
        }
    }
    szM[i] = L'\0';
    if (szM[0])
        return apxStrMatchW(szString, szM, bIgnoreCase);
    else
        return m;
}

LPSTR apxArrayToMultiSzA(APXHANDLE hPool, DWORD nArgs, LPCSTR *lpArgs)
{
    DWORD  i, l = 0;
    LPSTR lpSz, p;
    if (!nArgs)
        return NULL;
    for (i = 0; i < nArgs; i++)
        l += lstrlenA(lpArgs[i]);
    l += (nArgs + 2);

    p = lpSz = (LPSTR)apxPoolAlloc(hPool, l);
    for (i = 0; i < nArgs; i++) {
        lstrcpyA(p, lpArgs[i]);
        p += lstrlenA(lpArgs[i]);
        *p++ = '\0';
    }
    *p++ = '\0';
    *p++ = '\0';
    return lpSz;
}

void apxStrQuoteInplaceW(LPWSTR szString)
{
    LPWSTR p = szString;
    BOOL needsQuote = FALSE;
    while (*p) {
        if (*p++ == L' ') {
            needsQuote = TRUE;
            break;
        }
    }
    if (needsQuote) {
        DWORD l = lstrlenW(szString);
        AplMoveMemory(&szString[1], szString, l  * sizeof(WCHAR));
        szString[0]   = L'"';
        szString[++l] = L'"';
        szString[++l] = L'\0';
    }
}

DWORD apxStrUnQuoteInplaceA(LPSTR szString)
{
    LPSTR p = szString;
    BOOL needsQuote = FALSE;
    BOOL inQuote = FALSE;
    while (*p) {
        if (*p == '"') {
            if (inQuote)
                break;
            else
                inQuote = TRUE;
        }
        else if (*p == ' ') {
            if (inQuote) {
                needsQuote = TRUE;
                break;
            }
        }
        ++p;
    }
    if (!needsQuote)
        return apxStrCharRemoveA(szString, '"');
    else
        return 0;
}

DWORD apxStrUnQuoteInplaceW(LPWSTR szString)
{
    LPWSTR p = szString;
    BOOL needsQuote = FALSE;
    BOOL inQuote = FALSE;
    while (*p) {
        if (*p == L'"') {
            if (inQuote)
                break;
            else
                inQuote = TRUE;
        }
        else if (*p == L' ') {
            if (inQuote) {
                needsQuote = TRUE;
                break;
            }
        }
        ++p;
    }
    if (!needsQuote)
        return apxStrCharRemoveW(szString, L'"');
    else
        return 0;
}

LPWSTR
apxMszToCRLFW(APXHANDLE hPool, LPCWSTR szStr)
{
    DWORD l, c;
    LPWSTR rv, b;
    LPCWSTR p = szStr;

    l = __apxGetMultiSzLengthW(szStr, &c);
    b = rv = apxPoolCalloc(hPool, (l + c + 2) * sizeof(WCHAR));

    while (c > 0) {
        if (*p)
            *b++ = *p;
        else {
            *b++ = L'\r';
            *b++ = L'\n';
            c--;
        }
        p++;
    }
    return rv;
}

LPWSTR
apxCRLFToMszW(APXHANDLE hPool, LPCWSTR szStr, LPDWORD lpdwBytes)
{
    DWORD l, c, n = 0;
    LPWSTR rv, b;

    l = lstrlenW(szStr);
    b = rv = apxPoolCalloc(hPool, (l + 2) * sizeof(WCHAR));
    for (c = 0; c < l; c++) {
        if (szStr[c] == L'\r') {
            *b++ = '\0';
            n++;
        }
        else if (szStr[c] != L'\n') {
            *b++ = szStr[c];
            n++;
        }
    }
    if (lpdwBytes)
        *lpdwBytes = (n + 2) * sizeof(WCHAR);
    return rv;
}

LPSTR
apxExpandStrA(APXHANDLE hPool, LPCSTR szString)
{
    LPCSTR p = szString;
    while (*p) {
        if (*p == '%') {
            p = szString;
            break;
        }
        ++p;
    }
    if (p != szString)
        return apxPoolStrdupA(hPool, szString);
    else {
        DWORD l = ExpandEnvironmentStringsA(szString, NULL, 0);
        if (l) {
            LPSTR rv = apxPoolAlloc(hPool, l);
            l = ExpandEnvironmentStringsA(szString, rv, l);
            if (l)
                return rv;
            else {
                apxFree(rv);
                return NULL;
            }
        }
        else
            return NULL;
    }
}

LPWSTR
apxExpandStrW(APXHANDLE hPool, LPCWSTR szString)
{
    LPCWSTR p = szString;
    while (*p) {
        if (*p == L'%') {
            p = szString;
            break;
        }
        ++p;
    }
    if (p != szString)
        return apxPoolStrdupW(hPool, szString);
    else {
        DWORD l = ExpandEnvironmentStringsW(szString, NULL, 0);
        if (l) {
            LPWSTR rv = apxPoolAlloc(hPool, l * sizeof(WCHAR));
            l = ExpandEnvironmentStringsW(szString, rv, l);
            if (l)
                return rv;
            else {
                apxFree(rv);
                return NULL;
            }
        }
        else
            return NULL;
    }
}

/* To share the semaphores with other processes, we need a NULL ACL
 * Code from MS KB Q106387
 */
PSECURITY_ATTRIBUTES GetNullACL()
{
    PSECURITY_DESCRIPTOR pSD;
    PSECURITY_ATTRIBUTES sa;

    sa  = (PSECURITY_ATTRIBUTES) LocalAlloc(LPTR, sizeof(SECURITY_ATTRIBUTES));
    sa->nLength = sizeof(sizeof(SECURITY_ATTRIBUTES));

    pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
    sa->lpSecurityDescriptor = pSD;

    if (pSD == NULL || sa == NULL) {
        return NULL;
    }
    SetLastError(0);
    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)
    || GetLastError()) {
        LocalFree( pSD );
        LocalFree( sa );
        return NULL;
    }
    if (!SetSecurityDescriptorDacl(pSD, TRUE, (PACL) NULL, FALSE)
    || GetLastError()) {
        LocalFree( pSD );
        LocalFree( sa );
        return NULL;
    }

    sa->bInheritHandle = FALSE;
    return sa;
}


void CleanNullACL(void *sa)
{
    if (sa) {
        LocalFree(((PSECURITY_ATTRIBUTES)sa)->lpSecurityDescriptor);
        LocalFree(sa);
    }
}
