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

/* Logfile handling
 * Use Systemdir/Logfiles/Apache as a default path
 */

#ifndef _LOG_H_INCLUDED_
#define _LOG_H_INCLUDED_

__APXBEGIN_DECLS

#define APX_LOGLOCK(file)                           \
    APXMACRO_BEGIN                                  \
        DWORD _lmax = 0;                            \
        while(!LockFile(file, 0, 0, 512, 0)) {      \
            Sleep(10);                              \
            if (_lmax++ > 1000) break;              \
        }                                           \
        SetFilePointer(file, 0, NULL, FILE_END);    \
    APXMACRO_END

#define APX_LOGUNLOCK(file)                         \
    APXMACRO_BEGIN                                  \
        UnlockFile(file, 0, 0, 512, 0);             \
    APXMACRO_END

#define APX_LOGENTER()                              \
    if (_pt_sys_loglock)                            \
        EnterCriticalSection(_pt_sys_loglock);      \
    else (void)0

#define APX_LOGLEAVE()                              \
    if (_pt_sys_loglock)                            \
        LeaveCriticalSection(_pt_sys_loglock);      \
    else (void)0

#define APXLOG_LEVEL_DEBUG  0
#define APXLOG_LEVEL_INFO   1
#define APXLOG_LEVEL_WARN   2
#define APXLOG_LEVEL_ERROR  3

#define APXLOG_MARK_INFO    NULL, APXLOG_LEVEL_INFO,  TRUE,  __FILE__, __LINE__, ""
#define APXLOG_MARK_WARN    NULL, APXLOG_LEVEL_WARN,  TRUE,  __FILE__, __LINE__, ""
#define APXLOG_MARK_ERROR   NULL, APXLOG_LEVEL_ERROR, TRUE,  __FILE__, __LINE__, ""
#define APXLOG_MARK_DEBUG   NULL, APXLOG_LEVEL_DEBUG, TRUE,  __FILE__, __LINE__, ""
#define APXLOG_MARK_RAW     NULL, APXLOG_LEVEL_INFO,  FALSE, NULL, 0,
#define APXLOG_MARK_SYSERR  NULL, APXLOG_LEVEL_ERROR, TRUE,  __FILE__, __LINE__, NULL

LPWSTR apxLogFile(
    APXHANDLE hPool,
    LPCWSTR szPath,
    LPCWSTR szPrefix,
    LPCWSTR szName,
    BOOL bTimeStamp,
    DWORD dwRotate
);

HANDLE apxLogOpen(
    APXHANDLE hPool,
    LPCWSTR szPath,
    LPCWSTR szPrefix,
    DWORD dwRotate
);

void apxLogClose(
    HANDLE hFile
);

void apxLogLevelSet(
    HANDLE  hFile,
    DWORD dwLevel
);

void apxLogLevelSetW(
    HANDLE  hFile,
    LPCWSTR szLevel
);

int
apxLogWrite(
    HANDLE  hFile,
    DWORD   dwLevel,
    BOOL    bTimeStamp,
    LPCSTR  szFile,
    DWORD   dwLine,
    LPCSTR  szFormat,
    ...
);

int
apxDisplayError(
    BOOL    bDisplay,
    LPCSTR  szFile,
    DWORD   dwLine,
    LPCSTR  szFormat,
    ...
);

__APXEND_DECLS

#endif /* _LOG_H_INCLUDED_ */
