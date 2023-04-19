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
#include <stdio.h>
#include <strsafe.h>

#define LINE_SEP    "\r\n"
#define LOGF_EXT    L".%04d-%02d-%02d.log"
#define LOGF_EXR    L".%04d-%02d-%02d.%02d%02d%02d.log"

static LPCSTR _log_level[] = {
    "[trace] ",
    "[debug] ",
    "[info]  ",
    "[warn]  ",
    "[error] ",
    NULL
};

typedef struct apx_logfile_st {
    HANDLE      hFile;
    DWORD       dwLogLevel;
    DWORD       dwRotate;
    SYSTEMTIME  sysTime;
    WCHAR       szPath[SIZ_PATHLEN];
    WCHAR       szPrefix[MAX_PATH];
    WCHAR       szFile[MAX_PATH];
} apx_logfile_st;

/* Per-application master log file */
static apx_logfile_st   *_st_sys_loghandle = NULL;
static CRITICAL_SECTION  _st_sys_loglock;
static CRITICAL_SECTION *_pt_sys_loglock = NULL;
static apx_logfile_st    _st_sys_errhandle = { NULL, APXLOG_LEVEL_WARN, FALSE};

static void logRotate(apx_logfile_st *lf, LPSYSTEMTIME t)
{
    WCHAR sName[SIZ_PATHLEN];
    ULARGE_INTEGER cft;
    ULARGE_INTEGER lft;
    HANDLE h;

    if (lf->dwRotate == 0)
        return;

    SystemTimeToFileTime(&lf->sysTime, (LPFILETIME)&lft);
    SystemTimeToFileTime(t, (LPFILETIME)&cft);
    if (cft.QuadPart < (lft.QuadPart + lf->dwRotate * 10000000ULL))
        return;
    /* Rotate time */
    lf->sysTime = *t;
    if (lf->dwRotate < 86400)
        StringCchPrintf(sName,
                  SIZ_PATHLEN,
                  L"\\%s"  LOGF_EXR,
                  lf->szPrefix,
                  lf->sysTime.wYear,
                  lf->sysTime.wMonth,
                  lf->sysTime.wDay,
                  lf->sysTime.wHour,
                  lf->sysTime.wMinute,
                  lf->sysTime.wSecond);
    else
        StringCchPrintf(sName,
                  SIZ_PATHLEN,
                  L"\\%s"  LOGF_EXT,
                  lf->szPrefix,
                  lf->sysTime.wYear,
                  lf->sysTime.wMonth,
                  lf->sysTime.wDay);
    lstrlcpyW(lf->szFile, MAX_PATH, lf->szPath);
    lstrlcatW(lf->szFile, MAX_PATH, sName);
    h =  CreateFileW(lf->szFile,
                     GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL,
                     OPEN_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_SEQUENTIAL_SCAN,
                     NULL);
    if (h == INVALID_HANDLE_VALUE) {
        /* TODO: Log something */
        return;
    }
    /* Make sure we relock the correct file */
    APX_LOGLOCK(h);
    APX_LOGUNLOCK(lf->hFile);
    /* Close original handle */
    CloseHandle(lf->hFile);
    lf->hFile = h;
}

LPWSTR apxLogFile(
    APXHANDLE hPool,
    LPCWSTR szPath,
    LPCWSTR szPrefix,
    LPCWSTR szName,
    BOOL bTimeStamp,
    DWORD dwRotate)
{
    LPWSTR sRet;
    WCHAR sPath[SIZ_PATHLEN];
    WCHAR sName[SIZ_PATHLEN];
    SYSTEMTIME sysTime;

    GetLocalTime(&sysTime);
    if (!szPath) {
        if (GetSystemDirectoryW(sPath, MAX_PATH) == 0)
            return INVALID_HANDLE_VALUE;
        lstrlcatW(sPath, MAX_PATH, LOG_PATH_DEFAULT);
    }
    else {
        lstrlcpyW(sPath, MAX_PATH, szPath);
    }
    if (!szPrefix)
        szPrefix = L"";
    if (!szName)
        szName   = L"";
    if (bTimeStamp) {
        if (dwRotate != 0 && dwRotate < 86400)
            StringCchPrintf(sName,
                  SIZ_PATHLEN,
                  L"\\%s%s" LOGF_EXR,
                  szPrefix,
                  szName,
                  sysTime.wYear,
                  sysTime.wMonth,
                  sysTime.wDay,
                  0,
                  0,
                  0);
        else
            StringCchPrintf(sName,
                      SIZ_PATHLEN,
                      L"\\%s%s" LOGF_EXT,
                      szPrefix,
                      szName,
                      sysTime.wYear,
                      sysTime.wMonth,
                      sysTime.wDay);
    }
    else {
        StringCchPrintf(sName,
                  SIZ_PATHLEN,
                  L"\\%s%s",
                  szPrefix,
                  szName);
    }
    sRet = apxPoolAlloc(hPool, (SIZ_PATHLEN) * sizeof(WCHAR));
    /* Set default level to info */
    SHCreateDirectoryExW(NULL, sPath, NULL);

    lstrlcpyW(sRet, SIZ_PATHMAX, sPath);
    lstrlcatW(sRet, SIZ_PATHMAX, sName);

    return sRet;
}

/* Open the log file
 * TODO: format like standard apache error.log
 * Add the EventLogger
 */
HANDLE apxLogOpen(
    APXHANDLE hPool,
    LPCWSTR szPath,
    LPCWSTR szPrefix,
    DWORD dwRotate)
{

    WCHAR sPath[SIZ_PATHLEN];
    WCHAR sName[SIZ_PATHLEN];
    SYSTEMTIME sysTime;
    apx_logfile_st *h;

    if (_pt_sys_loglock == NULL) {
        InitializeCriticalSection(&_st_sys_loglock);
        _pt_sys_loglock = &_st_sys_loglock;
    }
    GetLocalTime(&sysTime);
    if (!szPath) {
        if (GetSystemDirectoryW(sPath, MAX_PATH) == 0)
            return INVALID_HANDLE_VALUE;
        lstrlcatW(sPath, MAX_PATH, L"\\LogFiles");
        if (!SHCreateDirectoryExW(NULL, sPath, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
            if (!SHCreateDirectoryExW(NULL, sPath, NULL))
                return INVALID_HANDLE_VALUE;
        }
        lstrlcatW(sPath, MAX_PATH, L"\\Apache");
        if (!SHCreateDirectoryExW(NULL, sPath, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
            if (!SHCreateDirectoryExW(NULL, sPath, NULL))
                return INVALID_HANDLE_VALUE;
        }
    }
    else {
        lstrlcpyW(sPath, MAX_PATH, szPath);
    }
    if (!szPrefix)
        szPrefix = L"commons-daemon";
    if (dwRotate != 0 && dwRotate < 86400)
        StringCchPrintf(sName,
                  SIZ_PATHLEN,
                  L"\\%s"  LOGF_EXR,
                  szPrefix,
                  sysTime.wYear,
                  sysTime.wMonth,
                  sysTime.wDay,
                  0,
                  0,
                  0);
    else
        StringCchPrintf(sName,
                  SIZ_PATHLEN,
                  L"\\%s"  LOGF_EXT,
                  szPrefix,
                  sysTime.wYear,
                  sysTime.wMonth,
                  sysTime.wDay);
    if (!(h = (apx_logfile_st *)apxPoolCalloc(hPool, sizeof(apx_logfile_st))))
        return INVALID_HANDLE_VALUE;
    /* Set default level to info */
    h->dwLogLevel = APXLOG_LEVEL_INFO;
    SHCreateDirectoryExW(NULL, sPath, NULL);

    h->sysTime = sysTime;
    lstrlcpyW(h->szPath, MAX_PATH, sPath);
    lstrlcpyW(h->szFile, MAX_PATH, sPath);
    lstrlcatW(h->szFile, MAX_PATH, sName);
    lstrlcpyW(h->szPrefix, MAX_PATH, szPrefix);

    h->hFile =  CreateFileW(h->szFile,
                      GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL,
                      OPEN_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_SEQUENTIAL_SCAN,
                      NULL);
    if (h->hFile == INVALID_HANDLE_VALUE) {
        /* Make sure we write somewhere */
        h = &_st_sys_errhandle;
        apxDisplayError(FALSE, NULL, 0,
                        "Unable to create logger at '%S'\n", h->szFile);
        return (HANDLE)h;
    }
    else {
        h->dwRotate = dwRotate;
    }
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

void apxLogLevelSetW(HANDLE  hFile,
                     LPCWSTR szLevel)
{
    apx_logfile_st *lf = (apx_logfile_st *)hFile;

    if (IS_INVALID_HANDLE(lf))
        lf = _st_sys_loghandle;
    if (IS_INVALID_HANDLE(lf)) {
        lf = &_st_sys_errhandle;
        lf->hFile = GetStdHandle(STD_ERROR_HANDLE);
    }
    if (szLevel) {
        if (!lstrcmpiW(szLevel, L"error"))
            lf->dwLogLevel = APXLOG_LEVEL_ERROR;
        else if (!lstrcmpiW(szLevel, L"warn"))
            lf->dwLogLevel = APXLOG_LEVEL_WARN;
        else if (!lstrcmpiW(szLevel, L"info"))
            lf->dwLogLevel = APXLOG_LEVEL_INFO;
        else if (!lstrcmpiW(szLevel, L"debug"))
            lf->dwLogLevel = APXLOG_LEVEL_DEBUG;
        else if (!lstrcmpiW(szLevel, L"trace"))
            lf->dwLogLevel = APXLOG_LEVEL_TRACE;
    }
}

DWORD
apxGetMessage(
    DWORD dwMessageId,
    LPSTR lpBuffer,
    DWORD nSize)
{
    if (nSize == 0 || lpBuffer == 0) {
        return 0;
    }
    return FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwMessageId,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        lpBuffer,
        nSize,
        NULL);
}

int
apxLogWrite(
    HANDLE  hFile,
    DWORD   dwLevel,
    BOOL    bTimeStamp,
    LPCSTR  szFile,
    DWORD   dwLine,
    LPCSTR  szFunction,
    LPCSTR  szFormat,
    ...)
{
    va_list args;
    CHAR    buffer[LOG_MSG_MAX_LEN] = "";
    LPSTR   szBp;
    int     len = 0;
    LPCSTR  file = szFile;
    CHAR    sb[SIZ_PATHLEN];
    DWORD   wr;
    DWORD   dwMessageId;
    BOOL    dolock = TRUE;
    apx_logfile_st *lf = (apx_logfile_st *)hFile;

    dwMessageId = GetLastError(); /* save the last Error code */
    if (IS_INVALID_HANDLE(lf))
        lf = _st_sys_loghandle;
    if (IS_INVALID_HANDLE(lf)) {
        lf = &_st_sys_errhandle;
        lf->hFile = GetStdHandle(STD_ERROR_HANDLE);
    }
    if (lf == &_st_sys_errhandle) {
        /* Do not rotate if redirected to console */
        dolock = FALSE;
    }
    if (dwLevel < lf->dwLogLevel)
        return 0;
    APX_LOGENTER();
    if (file && (lf->dwLogLevel <= APXLOG_LEVEL_DEBUG || dwLevel == APXLOG_LEVEL_ERROR)) {
        file = (szFile + lstrlenA(szFile) - 1);
        while(file != szFile && '\\' != *file && '/' != *file)
            file--;
        if(file != szFile)
            file++;
    }
    else
        file = NULL;
    szBp = buffer;
    if (!szFormat) {
        if (dwMessageId == 0) {
            StringCchCopyA(szBp, LOG_MSG_MAX_LEN, "Unknown error code");
            if (dwLevel == APXLOG_LEVEL_ERROR) {
                szBp += 18;
                StringCchPrintfA(szBp, LOG_MSG_MAX_LEN -18, " occurred in (%s:%d) ", file, dwLine);
            }
        }
        else
            apxGetMessage(dwMessageId, szBp, 1000);
    }
    else {
        va_start(args, szFormat);
        StringCchVPrintfA(szBp, LOG_MSG_MAX_LEN, szFormat, args);
        va_end(args);
    }
    len = lstrlenA(buffer);
    if (len > 0) {
        /* Remove trailing line separator */
        if (buffer[len - 1] == '\n')
            buffer[--len] = '\0';
        if (len > 0 && buffer[len - 1] == '\r')
            buffer[--len] = '\0';
        if (!IS_INVALID_HANDLE(lf->hFile)) {
            SYSTEMTIME t;
            /* Append operation */
            SetFilePointer(lf->hFile, 0, NULL, FILE_END);
            GetLocalTime(&t);
            if (dolock) {
                APX_LOGLOCK(lf->hFile);
                logRotate(lf, &t);
            }
            if (bTimeStamp) {
                StringCchPrintfA(sb, SIZ_PATHLEN,
                          "[%d-%02d-%02d %02d:%02d:%02d] ",
                          t.wYear, t.wMonth, t.wDay,
                          t.wHour, t.wMinute, t.wSecond);
                WriteFile(lf->hFile, sb, lstrlenA(sb), &wr, NULL);
            }
            WriteFile(lf->hFile, _log_level[dwLevel],
                      lstrlenA(_log_level[dwLevel]), &wr, NULL);

            if (szFunction && lf->dwLogLevel <= APXLOG_LEVEL_TRACE) {
                /* add function name in TRACE mode. */
                StringCchPrintfA(sb, SIZ_PATHLEN, "(%10s:%-4d:%-27s) ", file, dwLine, szFunction);
                WriteFile(lf->hFile, sb, lstrlenA(sb), &wr, NULL);
            } else if (file && lf->dwLogLevel <= APXLOG_LEVEL_DEBUG) {
                /* add file and line in DEBUG mode. */
                StringCchPrintfA(sb, SIZ_PATHLEN, "(%10s:%-4d) ", file, dwLine);
                WriteFile(lf->hFile, sb, lstrlenA(sb), &wr, NULL);
            }


            /* add thread ID to log output */
            StringCchPrintfA(sb, SIZ_PATHLEN, "[%5d] ", GetCurrentThreadId());
            WriteFile(lf->hFile, sb, lstrlenA(sb), &wr, NULL);

            if (len)
                WriteFile(lf->hFile, buffer, len, &wr, NULL);

            /* Terminate the line */
            WriteFile(lf->hFile, LINE_SEP, sizeof(LINE_SEP) - 1, &wr, NULL);
            if (dwLevel)
                FlushFileBuffers(lf->hFile);
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
    APX_LOGLEAVE();
    /* Restore the last Error code */
    SetLastError(dwMessageId);
    if (szFormat && dwMessageId != 0 && dwLevel == APXLOG_LEVEL_ERROR) {
        /* Print the System error description
         */
        apxLogWrite(hFile, dwLevel, bTimeStamp, szFile, dwLine, szFunction, NULL);
    }
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
    CHAR    buffer[SIZ_HUGLEN];
    CHAR    sysbuf[SIZ_HUGLEN];
    int     len = 0, nRet;
    LPCSTR  f = szFile;
    DWORD   dwMessageId = GetLastError(); /* save the last Error code */
    if (f) {
        f = (szFile + lstrlenA(szFile) - 1);
        while(f != szFile && '\\' != *f && '/' != *f)
            f--;
        if(f != szFile)
            f++;
    }
    else
        f = "";
    sysbuf[0] = '\0';
    if (dwMessageId != ERROR_SUCCESS) {
        len = apxGetMessage(dwMessageId, sysbuf, SIZ_DESLEN);
        sysbuf[len] = '\0';
        if (len > 0) {
            if (sysbuf[len - 1] == '\n')
                sysbuf[--len] = '\0';
            if (len > 0 && sysbuf[len - 1] == '\r')
                sysbuf[--len] = '\0';
        }
    }
    if (szFormat) {
        va_start(args, szFormat);
        StringCchVPrintfA(buffer, SIZ_HUGLEN, szFormat, args);
        va_end(args);
        if (f && *f) {
            CHAR sb[SIZ_PATHLEN];
            StringCchPrintfA(sb, SIZ_PATHLEN, "%s (%d)", f, dwLine);
            StringCchCatA(sysbuf, SIZ_HUGLEN, sb);
        }
        lstrlcatA(sysbuf, SIZ_HUGLEN, LINE_SEP);
        lstrlcatA(sysbuf, SIZ_HUGLEN, buffer);
    }
    len = lstrlenA(sysbuf);
#ifdef _DEBUG_FULL
    OutputDebugStringA(sysbuf);
#endif
    if (len > 0) {
        if (bDisplay) {
            nRet = MessageBoxA(NULL, sysbuf,
                               "Application System Error",
                               MB_ICONERROR | MB_OK | MB_SYSTEMMODAL);
        }
        else {
            fputs(sysbuf, stderr);
            if (!szFormat)
                fputs(LINE_SEP, stderr);
            fflush(stderr);
        }
    }
    /* Restore the last Error code */
    SetLastError(dwMessageId);
    return len;
}
