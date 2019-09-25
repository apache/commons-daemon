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
    HMODULE hmodUcrt;
    WPUTENV wputenv_ucrt = NULL;

    rc = GetEnvironmentVariableW(L"PATH", NULL, 0);
    if (rc == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND)
        return FALSE;
    al = 5 + lstrlenW(szAdd) + 1;
    if (!(wsAdd = apxPoolAlloc(hPool, (al + rc) * sizeof(WCHAR))))
        return FALSE;
    lstrcpyW(wsAdd, L"PATH=");
    lstrcatW(wsAdd, szAdd);
    lstrcatW(wsAdd, L";");
    if (GetEnvironmentVariableW(L"PATH", wsAdd + al, rc) != rc - 1) {
        apxLogWrite(APXLOG_MARK_SYSERR);
        apxFree(wsAdd);
        return FALSE;
    }

    hmodUcrt = LoadLibraryExA("ucrtbase.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (hmodUcrt != NULL) {
    	wputenv_ucrt =  (WPUTENV) GetProcAddress(hmodUcrt, "_wputenv");
    }

    SetEnvironmentVariableW(L"PATH", wsAdd + 5);
    _wputenv(wsAdd);
    if (wputenv_ucrt != NULL) {
    	wputenv_ucrt(wsAdd);
    }

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
    AplCopyMemory(p, lpString, (l + 1) * sizeof(WCHAR));
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
    int eol = 0;

    l = lstrlenW(szStr);
    b = rv = apxPoolCalloc(hPool, (l + 2) * sizeof(WCHAR));
    for (c = 0; c < l; c++) {
        if (szStr[c] == L'\r') {
        	if (eol) {
        	    // Blank line. Ignore it. See DAEMON-394.
        	}
        	else {
                *b++ = '\0';
                n++;
                eol = TRUE;
        	}
        }
        else if (szStr[c] != L'\n') {
            *b++ = szStr[c];
            n++;
            eol = FALSE;
        }
    }
    if (lpdwBytes)
        *lpdwBytes = (n + 2) * sizeof(WCHAR);
    return rv;
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
