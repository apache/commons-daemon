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

#define LINE_SEP    "\n"

static LPCSTR _log_level[] = {
    "[debug] ",
    "[info] ",
    "[warn] ",
    "[error] ",
    NULL
};

typedef struct apx_logfile_st {
    HANDLE      hFile;
    DWORD       dwLogLevel;
    BOOL        bRotate;
    SYSTEMTIME  sysTime;
    WCHAR       szPath[MAX_PATH + 1];
    WCHAR       szPrefix[MAX_PATH];
} apx_logfile_st;

/* Per-application master log file */
static apx_logfile_st *_st_sys_loghandle = NULL;

static apx_logfile_st  _st_sys_errhandle = { NULL, APXLOG_LEVEL_WARN, FALSE};


LPWSTR apxLogFile(
    APXHANDLE hPool,
    LPCWSTR szPath,
    LPCWSTR szPrefix,
    LPCWSTR szName)
{
    LPWSTR sRet;
    WCHAR sPath[MAX_PATH+1];
    WCHAR sName[MAX_PATH+1];
    SYSTEMTIME sysTime;

    GetLocalTime(&sysTime);
    if (!szPath) {
        if (GetSystemDirectoryW(sPath, MAX_PATH) == 0)
            return INVALID_HANDLE_VALUE;
        lstrlcatW(sPath, MAX_PATH, L"\\LogFiles\\Apache");
    }
    else {
        lstrlcpyW(sPath, MAX_PATH, szPath);
    }
    if (!szPrefix)
        szPrefix = L"";
    if (!szName)
        szName   = L"";
    wsprintfW(sName,
              L"\\%s%s%04d%02d%02d.log",
              szPrefix,
              szName,
              sysTime.wYear,
              sysTime.wMonth,
              sysTime.wDay);

    sRet = apxPoolAlloc(hPool, (MAX_PATH + 1) * sizeof(WCHAR));
    /* Set default level to info */
    CreateDirectoryW(sPath, NULL);

    lstrlcpyW(sRet, MAX_PATH, sPath);
    lstrlcatW(sRet, MAX_PATH, sName);

    return sRet;
}

/* Open the log file
 * TODO: format like standard apache error.log
 * Add the EventLogger
 */
HANDLE apxLogOpen(
    APXHANDLE hPool,
    LPCWSTR szPath,
    LPCWSTR szPrefix)
{

    WCHAR sPath[MAX_PATH+1];
    WCHAR sName[MAX_PATH+1];
    SYSTEMTIME sysTime;
    apx_logfile_st *h;

    GetLocalTime(&sysTime);
    if (!szPath) {
        if (GetSystemDirectoryW(sPath, MAX_PATH) == 0)
            return INVALID_HANDLE_VALUE;
        lstrlcatW(sPath, MAX_PATH, L"\\LogFiles\\Apache");
    }
    else {
        lstrlcpyW(sPath, MAX_PATH, szPath);
    }
    if (!szPrefix)
        szPrefix = L"jakarta_service_";
    wsprintfW(sName, L"\\%s%04d%02d%02d.log",
              szPrefix,
              sysTime.wYear,
              sysTime.wMonth,
              sysTime.wDay);
    if (!(h = (apx_logfile_st *)apxPoolCalloc(hPool, sizeof(apx_logfile_st))))
        return NULL;
    /* Set default level to info */
    h->dwLogLevel = APXLOG_LEVEL_INFO;
    CreateDirectoryW(sPath, NULL);

    h->sysTime = sysTime;
    lstrlcpyW(h->szPath, MAX_PATH, sPath);
    lstrlcatW(sPath, MAX_PATH, sName);
    lstrlcpyW(h->szPrefix, MAX_PATH, szPrefix);

    h->hFile =  CreateFileW(sPath,
                      GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL,
                      OPEN_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_SEQUENTIAL_SCAN,
                      NULL);
    /* Set this file as system log file */
    if (!_st_sys_loghandle)
        _st_sys_loghandle = h;

    return (HANDLE)h;
}

void apxLogLevelSet(HANDLE hFile, DWORD dwLevel)
{
    apx_logfile_st *lf = (apx_logfile_st *)hFile;
    if (IS_INVALID_HANDLE(lf))
        lf = _st_sys_loghandle;
    if (IS_INVALID_HANDLE(lf))
        return;
    if (dwLevel < 4)
        lf->dwLogLevel = dwLevel;
}

void apxLogRotateSet(HANDLE hFile, BOOL doRotate)
{
    apx_logfile_st *lf = (apx_logfile_st *)hFile;
    if (IS_INVALID_HANDLE(lf))
        lf = _st_sys_loghandle;
    if (IS_INVALID_HANDLE(lf))
        return;
    lf->bRotate = doRotate;
}

void apxLogLevelSetW(HANDLE  hFile,
                     LPCWSTR szLevel)
{
    apx_logfile_st *lf = (apx_logfile_st *)hFile;

    if (IS_INVALID_HANDLE(lf))
        lf = _st_sys_loghandle;
    if (IS_INVALID_HANDLE(lf))
        return;
    if (szLevel) {
        if (!lstrcmpiW(szLevel, L"error"))
            lf->dwLogLevel = APXLOG_LEVEL_ERROR;
        else if (!lstrcmpiW(szLevel, L"warn"))
            lf->dwLogLevel = APXLOG_LEVEL_WARN;
        else if (!lstrcmpiW(szLevel, L"info"))
            lf->dwLogLevel = APXLOG_LEVEL_INFO;
        else if (!lstrcmpiW(szLevel, L"debug"))
            lf->dwLogLevel = APXLOG_LEVEL_DEBUG;
    }
}

static BOOL apx_log_rotate(apx_logfile_st *l,
                           LPSYSTEMTIME lpCtime)
{
    WCHAR sPath[MAX_PATH+1];

    /* rotate on daily basis */
    if (l->sysTime.wDay == lpCtime->wDay)
        return TRUE;
    FlushFileBuffers(l->hFile);
    CloseHandle(l->hFile);
    l->sysTime = *lpCtime;

    wsprintfW(sPath, L"%s\\%s%04d%02d%02d.log",
              l->szPath,
              l->szPrefix,
              l->sysTime.wYear,
              l->sysTime.wMonth,
              l->sysTime.wDay);
    l->hFile =  CreateFileW(sPath,
                      GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL,
                      OPEN_ALWAYS,
                      FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN,
                      NULL);
    if (IS_INVALID_HANDLE(l->hFile))
        return FALSE;
    else
        return TRUE;
}

int
apxLogWrite(
    HANDLE  hFile,
    DWORD   dwLevel,
    BOOL    bTimeStamp,
    LPCSTR  szFile,
    DWORD   dwLine,
    LPCSTR  szFormat,
    ...)
{
    va_list args;
    CHAR    buffer[1024+32];
    LPSTR   szBp;
    int     len = 0;
    LPCSTR  f = szFile;
    CHAR    sb[MAX_PATH+1];
    DWORD   wr;
    DWORD   err;
    BOOL    dolock = TRUE;
    apx_logfile_st *lf = (apx_logfile_st *)hFile;

    err = GetLastError(); /* save the last Error code */
    if (IS_INVALID_HANDLE(lf))
        lf = _st_sys_loghandle;
    if (IS_INVALID_HANDLE(lf)) {
        lf = &_st_sys_errhandle;
        lf->hFile = GetStdHandle(STD_ERROR_HANDLE);
        dolock = FALSE;
    }
    if (dwLevel < lf->dwLogLevel)
        return 0;
    if (f) {
        f = (szFile + lstrlenA(szFile) - 1);
        while(f != szFile && '\\' != *f && '/' != *f)
            f--;
        if(f != szFile)
            f++;
    }
    lstrlcpyA(buffer, 1056, _log_level[dwLevel]);
    if (!dolock)
        lstrlcatA(buffer, 1056, "\n");
    szBp = &buffer[lstrlenA(buffer)];
    if (!szFormat) {
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       err,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       szBp,
                       1000,
                       NULL);
    }
    else {
        va_start(args, szFormat);
        wvsprintfA(szBp, szFormat, args);
        va_end(args);
    }
    len = lstrlenA(buffer);
    if (len > 0) {
        /* Remove trailing line separator */
        if (buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            --len;
        }
        if (!IS_INVALID_HANDLE(lf->hFile)) {
            SYSTEMTIME t;
            GetLocalTime(&t);
            if (lf->bRotate) {
                if (!apx_log_rotate(lf, &t))
                    return 0;
            }
            if (dolock) {
                APX_LOGLOCK(lf->hFile);
            }
            if (bTimeStamp) {
                wsprintfA(sb, "[%d-%02d-%02d %02d:%02d:%02d] ",
                          t.wYear, t.wMonth, t.wDay,
                          t.wHour, t.wMinute, t.wSecond);
                WriteFile(lf->hFile, sb, lstrlenA(sb), &wr, NULL);
            }
            if (f) {
                wsprintfA(sb, "[%-4d %s] ", dwLine, f);
                WriteFile(lf->hFile, sb, lstrlenA(sb), &wr, NULL);
            }

            WriteFile(lf->hFile, buffer, len, &wr, NULL);
            /* Terminate the line */
            WriteFile(lf->hFile, LINE_SEP, sizeof(LINE_SEP) - 1, &wr, NULL);
#ifdef _DEBUG_FULL
            FlushFileBuffers(lf->hFile);
#endif
            if (dolock) {
                APX_LOGUNLOCK(lf->hFile);
            }
        }
#ifdef _DEBUG_FULL
        {
            char tid[1024 + 16];
            wsprintfA(tid, "[%04d] %s", GetCurrentThreadId(), buffer);
            OutputDebugStringA(tid);
        }
#endif
    }
    /* Restore the last Error code */
    SetLastError(err);
    return len;
}

void apxLogClose(
    HANDLE hFile)
{
    apx_logfile_st *lf = (apx_logfile_st *)hFile;

    if (IS_INVALID_HANDLE(lf))
        lf = _st_sys_loghandle;
    if (IS_INVALID_HANDLE(lf))
        return;

    FlushFileBuffers(lf->hFile);
    CloseHandle(lf->hFile);
    if (lf == _st_sys_loghandle)
        _st_sys_loghandle = NULL;
    apxFree(lf);
}

int
apxDisplayError(
    BOOL    bDisplay,
    LPCSTR  szFile,
    DWORD   dwLine,
    LPCSTR  szFormat,
    ...)
{
    va_list args;
    CHAR    buffer[1024+32];
    CHAR    sysbuf[2048];
    int     len = 0, nRet;
    LPCSTR  f = szFile;
    DWORD   err = GetLastError(); /* save the last Error code */
    if (f) {
        f = (szFile + lstrlenA(szFile) - 1);
        while(f != szFile && '\\' != *f && '/' != *f)
            f--;
        if(f != szFile)
            f++;
    }
    sysbuf[0] = '\0';
    if (err != ERROR_SUCCESS) {
        len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
                             FORMAT_MESSAGE_IGNORE_INSERTS,
                             NULL,
                             err,
                             MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                             sysbuf,
                             1000,
                             NULL);
        sysbuf[len] = 0;
    }
    if (szFormat) {
        va_start(args, szFormat);
        wvsprintfA(buffer, szFormat, args);
        va_end(args);
        if (f) {
            CHAR sb[MAX_PATH+1];
            wsprintfA(sb, "\n%s (%d)", f, dwLine);
            lstrcatA(sysbuf, sb);
        }
        lstrlcatA(sysbuf, 2048, "\n");
        lstrlcatA(sysbuf, 2048, buffer);
    }
    len = lstrlenA(sysbuf);
#ifdef _DEBUG_FULL
    OutputDebugStringA(sysbuf);
#endif
    if (len > 0 && bDisplay) {
        nRet = MessageBoxA(NULL, sysbuf,
                           "Application System Error",
                           MB_ICONERROR | MB_OK | MB_SYSTEMMODAL);
    }
    /* Restore the last Error code */
    SetLastError(err);
    return len;
}
