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
 
#ifndef _APXWIN_H_INCLUDED_
#define _APXWIN_H_INCLUDED_


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <objbase.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <zmouse.h>
#include <richedit.h>
#include <lm.h>

#ifndef _INTPTR_T_DEFINED
#ifdef  _WIN64
typedef __int64             intptr_t;
#else
typedef _W64 int            intptr_t;
#endif
#define _INTPTR_T_DEFINED
#endif

#define APXMACRO_BEGIN                  do {
#define APXMACRO_END                    } while(0)

#ifdef  __cplusplus
#define __APXBEGIN_DECLS    extern "C" {
#define __APXEND_DECLS  }
#else
#define __APXBEGIN_DECLS
#define __APXEND_DECLS
#endif

#define SET_BIT_FLAG(x, b) ((x) |= (1 << b))
#define CLR_BIT_FLAG(x, b) ((x) &= ~(1 << b))
#define TST_BIT_FLAG(x, b) ((x) & (1 << b))

#define IS_INVALID_HANDLE(h) (((h) == NULL || (h) == INVALID_HANDLE_VALUE))
#define IS_VALID_STRING(s)   ((s) != NULL && *(s) != 0)
#define IS_EMPTY_STRING(s)   ((s) == NULL || *(s) == 0)

#define DYNOLAD_TYPE_DECLARE(fnName, callconv, retType)             \
    typedef retType (callconv *PFN_##fnName)                        \

#define DYNLOAD_FPTR_DECLARE(fnName)                                \
    PFN_##fnName FP_##fnName

#define DYNLOAD_FPTR(fnName)  FP_##fnName

#define DYNLOAD_FPTR_ADDRESS(fnName, dllName)                       \
    FP_##fnName = (PFN_##fnName)GetProcAddress(                     \
                                GetModuleHandle(TEXT(#dllName)),    \
                                #fnName)    

#define DYNLOAD_FPTR_LOAD(fnName, dllHandle)                        \
    FP_##fnName = (PFN_##fnName)GetProcAddress(                     \
                                dllHandle,                          \
                                #fnName)    

#define DYNLOAD_CALL(fnName)    (*FP_##fnName)

#ifndef ABS
#define ABS(x)       (((x) > 0) ? (x) : (x) * (-1))
#endif

#define SIZ_RESLEN         256
#define SIZ_RESMAX         (SIZ_RESLEN -1)
#define SIZ_BUFLEN         512
#define SIZ_BUFMAX         (SIZ_BUFLEN -1)
#define SIZ_DESLEN         1024
#define SIZ_DESMAX         (SIZ_DESLEN -1)
#define SIZ_HUGLEN         8192
#define SIZ_HUGMAX         (SIZ_HUGLEN -1)
#define SIZ_PATHLEN        4096
#define SIZ_PATHMAX        4095

#include "handles.h"
#include "log.h"
#include "cmdline.h"
#include "console.h"
#include "rprocess.h"
#include "registry.h"
#include "service.h"
#include "javajni.h"
#include "gui.h"

__APXBEGIN_DECLS

typedef enum {
        APX_WINVER_UNK =       0,
        APX_WINVER_UNSUP =     1,
        APX_WINVER_95 =       10,
        APX_WINVER_95_B =     11,
        APX_WINVER_95_OSR2 =  12,
        APX_WINVER_98 =       14,
        APX_WINVER_98_SE =    16,
        APX_WINVER_ME =       18,

        APX_WINVER_UNICODE =  20, /* Prior versions support only narrow chars */

        APX_WINVER_CE_3 =     23, /* CE is an odd beast, not supporting */
                               /* some pre-NT features, such as the    */
        APX_WINVER_NT =       30, /* narrow charset APIs (fooA fns), while  */
        APX_WINVER_NT_3_5 =   35, /* not supporting some NT-family features.  */
        APX_WINVER_NT_3_51 =  36,

        APX_WINVER_NT_4 =     40,
        APX_WINVER_NT_4_SP2 = 42,
        APX_WINVER_NT_4_SP3 = 43,
        APX_WINVER_NT_4_SP4 = 44,
        APX_WINVER_NT_4_SP5 = 45,
        APX_WINVER_NT_4_SP6 = 46,

        APX_WINVER_2000 =     50,
        APX_WINVER_2000_SP1 = 51,
        APX_WINVER_2000_SP2 = 52,
        APX_WINVER_XP =       60
} APX_OSLEVEL;

APX_OSLEVEL apxGetOsLevel();
LPWSTR      AsciiToWide(LPCSTR s, LPWSTR ws);
LPSTR       MzWideToAscii(LPCWSTR ws, LPSTR s);
LPSTR       WideToANSI(LPCWSTR ws);
LPSTR       MzWideToANSI(LPCWSTR ws);

typedef struct APXMULTISZ APXMULTISZ;
typedef APXMULTISZ*       LPAPXMULTISZ;

DWORD   apxMultiSzToArrayA(APXHANDLE hPool, LPCSTR lpString, LPSTR **lppArray);
DWORD   apxMultiSzToArrayW(APXHANDLE hPool, LPCWSTR lpString, LPWSTR **lppArray);
LPWSTR  apxMultiSzCombine(APXHANDLE hPool, LPCWSTR lpStrA, LPCWSTR lpStrB,
                          LPDWORD lpdwLength);

LPAPXMULTISZ    apxMultiSzStrdup(LPCTSTR szSrc);
LPTSTR          apxMultiSzStrcat(LPAPXMULTISZ lpmSz, LPCTSTR szSrc);
LPCTSTR         apxMultiSzGet(LPAPXMULTISZ lpmSz);
/** Remove the specified character from the string
 * Replacement is done in-place.
 */
LPTSTR          apxStrCharRemove(LPTSTR szString, TCHAR chSkip);
DWORD           apxStrUnQuoteInplaceA(LPSTR szString);
DWORD           apxStrUnQuoteInplaceW(LPWSTR szString);

BOOL            apxUltohex(ULONG n, LPTSTR lpBuff, DWORD dwBuffLength);
BOOL            apxUptohex(ULONG_PTR n, LPTSTR lpBuff, DWORD dwBuffLength);
ULONG           apxStrToul(LPCTSTR szNum);
ULONG           apxStrToulW(LPCWSTR szNum);
ULONG           apxAtoulW(LPCWSTR szNum);

BOOL            apxMakeResourceName(LPCTSTR szPrefix, LPTSTR lpBuff,
                                    DWORD dwBuffLength);

INT             apxStrMatchA(LPCSTR szString, LPCSTR szPattern, BOOL bIgnoreCase);
INT             apxStrMatchW(LPCWSTR szString, LPCWSTR szPattern, BOOL bIgnoreCase);
INT             apxMultiStrMatchW(LPCWSTR szString, LPCWSTR szPattern,
                                  WCHAR chSeparator, BOOL bIgnoreCase);
LPSTR           apxArrayToMultiSzA(APXHANDLE hPool, DWORD nArgs, LPCSTR *lpArgs);
void            apxStrQuoteInplaceW(LPWSTR szString);
LPWSTR          apxMszToCRLFW(APXHANDLE hPool, LPCWSTR szStr);
LPWSTR          apxCRLFToMszW(APXHANDLE hPool, LPCWSTR szStr, LPDWORD lpdwBytes);
LPSTR           apxExpandStrA(APXHANDLE hPool, LPCSTR szString);
LPWSTR          apxExpandStrW(APXHANDLE hPool, LPCWSTR szString);
void            apxStrCharReplaceA(LPSTR szString, CHAR chReplace, CHAR chReplaceWith);
void            apxStrCharReplaceW(LPWSTR szString, WCHAR chReplace, WCHAR chReplaceWith);
BOOL            apxAddToPathW(APXHANDLE hPool, LPCWSTR szAdd);


LPVOID  AplFillMemory(PVOID Destination, SIZE_T Length, BYTE Fill);
void    AplZeroMemory(PVOID Destination, SIZE_T Length);
LPVOID  AplCopyMemory(PVOID Destination, const VOID* Source, SIZE_T Length);
/*
 * Find the first occurrence of lpFind in lpMem.
 * dwLen:   The length of lpFind
 * dwSize:  The length of lpMem
 */
LPBYTE  ApcMemSearch(LPCVOID lpMem, LPCVOID lpFind, SIZE_T dwLen, SIZE_T dwSize);
LPSTR   AplRindexA(LPCSTR lpStr, int ch);

#define AplMoveMemory   AplCopyMemory

LPSTR   lstrlcatA(LPSTR dst, int siz, LPCSTR src);
LPWSTR  lstrlcatW(LPWSTR dst, int siz, LPCWSTR src);
LPSTR   lstrlcpyA(LPSTR dst, int siz, LPCSTR src);
LPWSTR  lstrlcpyW(LPWSTR dst, int siz, LPCWSTR src);
LPWSTR  lstrlocaseW(LPWSTR str);

PSECURITY_ATTRIBUTES GetNullACL();
void CleanNullACL(void *sa);

__APXEND_DECLS

#endif /* _APXWIN_H_INCLUDED_ */

