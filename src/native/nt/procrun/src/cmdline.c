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

#define EXE_SUFFIX      L".EXE"
#define EXE_SUFFIXLEN   (sizeof(EXE_SUFFIX) / sizeof(WCHAR) - 1)

#define X86_SUFFIX      L".X86"
#define X64_SUFFIX      L".X64"
#define A64_SUFFIX      L".I64"

/* Those two are declared in handles.c */
extern LPWSTR   *_st_sys_argvw;
extern int      _st_sys_argc;

static WCHAR    _st_sys_appexe[MAX_PATH];
/*
 * argv parsing.
 * Parse the argv[0] and split to ExePath and
 * Executable name. Strip the extension ('.exe').
 * Check for command in argv[1] //CMD//Application
 * Parse the options --option value or --option==value
 * break on first argument that doesn't start with '--'
 */
LPAPXCMDLINE apxCmdlineParse(
    APXHANDLE hPool,
    APXCMDLINEOPT   *lpOptions,
    LPCWSTR         *lpszCommands)
{

    LPAPXCMDLINE lpCmdline = NULL;
    DWORD l, i = 0, s = 1;
    LPWSTR p;
    WCHAR  cmd[4];
    DWORD  match;
    BOOL   add;
    if (_st_sys_argc < 1)
        return NULL;

    l = lstrlenW(_st_sys_argvw[0]);
    if (l < EXE_SUFFIXLEN)
        return NULL;
    if (!(lpCmdline = (LPAPXCMDLINE)apxPoolCalloc(hPool, sizeof(APXCMDLINE))))
        return NULL;
    lpCmdline->hPool       = hPool;
    lpCmdline->lpOptions   = lpOptions;
    lpCmdline->szExePath   = _st_sys_argvw[0];
    p = lpCmdline->szExePath + l;

    while (p > lpCmdline->szExePath) {
        if (*p == L'\\') {
            *p++ = L'\0';
            break;
        }
        p--;
    }
    /* Find the path if it wasn't specified in the argv[0] */
    if (p == lpCmdline->szExePath) {
        WCHAR  mh[4096];
        LPWSTR m;
        if (GetModuleFileNameW(GetModuleHandle(NULL), mh, 4096)) {
            lpCmdline->szExePath = apxPoolStrdupW(hPool, mh);
            m = lpCmdline->szExePath + lstrlenW(lpCmdline->szExePath);
            while (m > lpCmdline->szExePath) {
                if (*(m--) == L'\\') {
                    *(++m) = L'\0';
                    break;
                }
            }
        }
    }
    lpCmdline->szExecutable = p;
    p = _st_sys_argvw[0] + l - EXE_SUFFIXLEN;
    if (lstrcmpiW(p, EXE_SUFFIX) == 0)
        *p = L'\0';
    /* Strip CPU specific suffixes */
    l = lstrlenW(_st_sys_argvw[0]);
    if (l > EXE_SUFFIXLEN) {
        p = _st_sys_argvw[0] + l - EXE_SUFFIXLEN;
        if (lstrcmpiW(p, X86_SUFFIX) == 0) {
            *p = L'\0';
        }
        else if (lstrcmpiW(p, X64_SUFFIX) == 0) {
            *p = L'\0';
        }
        else if (lstrcmpiW(p, A64_SUFFIX) == 0) {
            *p = L'\0';
        }
    }
    if (lpszCommands && _st_sys_argc > 1 && lstrlenW(_st_sys_argvw[1]) > 5) {
        if (_st_sys_argvw[1][0] == L'/' &&
            _st_sys_argvw[1][1] == L'/' &&
            _st_sys_argvw[1][5] == L'/') { /* allow max tree char command */
            l = 2;
            while (_st_sys_argvw[1][l] != L'/') {
                cmd[l - 2] = _st_sys_argvw[1][l];
                if (l++ > 3)
                    break;
            }
            cmd[l - 2] = L'\0';
            while (lpszCommands[i]) {
                if (lstrcmpW(lpszCommands[i++], cmd) == 0) {
                    lpCmdline->dwCmdIndex = i;
                    s = 2;
                    break;
                }
            }
            if (lpCmdline->dwCmdIndex) {
                _st_sys_argvw[1][l+1] = L'\0';
                lpCmdline->szApplication = &(_st_sys_argvw[1][l+2]);
                if (!lstrlenW(lpCmdline->szApplication))
                    lpCmdline->szApplication = _st_sys_argvw[0];
            }
            else {
                apxLogWrite(APXLOG_MARK_ERROR "Unrecognized cmd option %s", cmd);
                s = 2;
            }
        }
    }
    else {
        lpCmdline->szApplication = lpCmdline->szExecutable;
        lpCmdline->dwCmdIndex = 1;
        return lpCmdline;
    }
    for (i = s; i < (DWORD)_st_sys_argc; i++) {
        LPWSTR e = NULL;
        if (_st_sys_argvw[i][0] != L'-' ||
            _st_sys_argvw[i][1] != L'-') {
            if (_st_sys_argvw[i][0] != L'+' ||
                _st_sys_argvw[i][1] != L'+')
                break;
            else
                add = TRUE;
        }
        else
            add = FALSE;
        p = &(_st_sys_argvw[i][2]);
        /* Find if the option has '=' char
         * for --option==value or --option value cases.
         */
        while (*p) {
            if (*p == L'=') {
                *p = L'\0';
                e = p + 1;
                break;
            }
            else
                p++;
        }
        match = 0;
        for (l = 0; lpOptions[l].szName; l++) {
            if (lstrcmpW(lpOptions[l].szName, &(_st_sys_argvw[i][2])) == 0) {
                LPWSTR val;
                /* check if arg is needed */
                if (e)
                    val = e;
                else if ((i + 1) < (DWORD)_st_sys_argc)
                    val = _st_sys_argvw[++i];
                else {
                    lpOptions[l].dwValue = 0;
                    lpOptions[l].szValue = NULL;
                    lpOptions[l].dwType |= APXCMDOPT_FOUND;
                    break;
                }
                if (lpOptions[l].dwType & APXCMDOPT_STR)
                    lpOptions[l].szValue = val;
                else if (lpOptions[l].dwType & APXCMDOPT_INT)
                    lpOptions[l].dwValue = (DWORD)apxAtoulW(val);
                else if (lpOptions[l].dwType & APXCMDOPT_MSZ) {
                    LPWSTR pp;
                    BOOL insquote = FALSE, indquote=FALSE;
                    DWORD sp = 0;
                    LPWSTR ov = lpOptions[l].szValue;
                    if (lpOptions[l].dwValue > 2) {
                        sp = (lpOptions[l].dwValue - sizeof(WCHAR)) / sizeof(WCHAR);
                    }
                    lpOptions[l].dwValue = (sp + lstrlenW(val) + 2) * sizeof(WCHAR);
                    lpOptions[l].szValue = (LPWSTR)apxPoolCalloc(hPool,
                                                lpOptions[l].dwValue);
                    if (sp) {
                        AplMoveMemory(lpOptions[l].szValue, ov, sp * sizeof(WCHAR));
                        apxFree(ov);
                    }
                    pp = val;
                    while(*pp) {
                        if (*pp == L'\'')
                            insquote = !insquote;
                        else if (*pp == L'"') {
                            indquote = !indquote;
                            lpOptions[l].szValue[sp++] = L'"';
                        }
                        else if ((*pp == L'#' || *pp == L';') && !insquote && !indquote)
                            lpOptions[l].szValue[sp++] = L'\0';
                        else
                            lpOptions[l].szValue[sp++] = *pp;
                        pp++;
                    }
                }
                lpOptions[l].dwType |= APXCMDOPT_FOUND;
                if (add)
                    lpOptions[l].dwType |= APXCMDOPT_ADD;
                match = l + 1;
                break;
            }
        }
    }
    if (i < (DWORD)_st_sys_argc) {
        lpCmdline->dwArgc = _st_sys_argc - i;
        lpCmdline->lpArgvw = &_st_sys_argvw[i];
    }
    return lpCmdline;
}

/* Used for future expansion */
void apxCmdlineFree(
    LPAPXCMDLINE lpCmdline)
{

    apxFree(lpCmdline);
}

/*
 * Environment variables parsing
 * Each variable is prfixed with PR_
 * for example 'set PR_JVM=auto' has a same meaning as providing '--Jvm auto'
 * on the command line.
 * Multistring varisables are added to the present conf.
 */
void apxCmdlineLoadEnvVars(
    LPAPXCMDLINE lpCmdline)
{
    WCHAR szEnv[64];
    int i = 0;
    if (!lpCmdline || !lpCmdline->lpOptions)
        return;

    while (lpCmdline->lpOptions[i].szName) {
        DWORD l;
        WCHAR szVar[SIZ_HUGLEN];
        lstrlcpyW(szEnv, SIZ_HUGLEN, L"PR_");
        lstrlcatW(szEnv, SIZ_HUGLEN, lpCmdline->lpOptions[i].szName);
        l = GetEnvironmentVariableW(szEnv, szVar, SIZ_HUGMAX);
        if (l == 0 && GetLastError() !=  ERROR_ENVVAR_NOT_FOUND) {
            apxLogWrite(APXLOG_MARK_ERROR "Error geting environment variable %S",
                        szEnv);
            ++i;
            continue;
        }
        if (l && (lpCmdline->lpOptions[i].dwType & APXCMDOPT_STR)) {
            lpCmdline->lpOptions[i].szValue = apxPoolStrdupW(lpCmdline->hPool, szVar);
            lpCmdline->lpOptions[i].dwType |= APXCMDOPT_FOUND;
        }
        else if (l && (lpCmdline->lpOptions[i].dwType & APXCMDOPT_INT)) {
            lpCmdline->lpOptions[i].dwValue = (DWORD)apxAtoulW(szVar);
            lpCmdline->lpOptions[i].dwType |= APXCMDOPT_FOUND;
        }
        else if (l && (lpCmdline->lpOptions[i].dwType & APXCMDOPT_MSZ)) {
            LPWSTR pp;
            BOOL insquote = FALSE, indquote = FALSE;
            DWORD sp = 0;
            lpCmdline->lpOptions[i].dwValue = (lstrlenW(szVar) + 2) * sizeof(WCHAR);
            lpCmdline->lpOptions[i].szValue = apxPoolCalloc(lpCmdline->hPool,
                                                    lpCmdline->lpOptions[i].dwValue);
            pp = szVar;
            while(*pp) {
                if (*pp == L'\'')
                    insquote = !insquote;
                else if (*pp == L'"') {
                    indquote = !indquote;
                    lpCmdline->lpOptions[i].szValue[sp++] = L'"';
                }
                else if ((*pp == L'#' || *pp == L';') && !insquote && !indquote)
                    lpCmdline->lpOptions[i].szValue[sp++] = L'\0';
                else
                    lpCmdline->lpOptions[i].szValue[sp++] = *pp;
                pp++;
            }
            lpCmdline->lpOptions[i].dwType |= APXCMDOPT_FOUND | APXCMDOPT_ADD;
        }
        ++i;
    }

}
