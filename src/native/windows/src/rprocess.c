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

#define CHILD_RUNNING               0x0001
#define CHILD_INITIALIZED           0x0002
#define CHILD_MAINTREAD_FINISHED    0x0004
#define PROC_INITIALIZED            0x0008
#define CHILD_TERMINATE_CODE        19640323 /* Could be any value like my birthday ;-)*/

DYNOLAD_TYPE_DECLARE(CreateRemoteThread,
                     __stdcall, HANDLE)(HANDLE, LPSECURITY_ATTRIBUTES,
                                        DWORD, LPTHREAD_START_ROUTINE,
                                        LPVOID, DWORD, LPDWORD);

DYNOLAD_TYPE_DECLARE(ExitProcess, __stdcall, void)(UINT);

#define CHECK_IF_ACTIVE(proc) \
    APXMACRO_BEGIN                                                      \
        DWORD __st;                                                     \
        if (!GetExitCodeProcess((proc)->stProcInfo.hProcess, &__st) ||  \
                                (__st != STILL_ACTIVE))                 \
            goto cleanup;                                               \
    APXMACRO_END

#define SAVE_STD_HANDLES(p) \
    APXMACRO_BEGIN                                                      \
    if ((p)->bSaveHandles) {                                            \
    (p)->hParentStdSave[0] = GetStdHandle(STD_INPUT_HANDLE);            \
    (p)->hParentStdSave[1] = GetStdHandle(STD_OUTPUT_HANDLE);           \
    (p)->hParentStdSave[2] = GetStdHandle(STD_ERROR_HANDLE);            \
    } APXMACRO_END

#define RESTORE_STD_HANDLES(p) \
    APXMACRO_BEGIN                                                      \
    if ((p)->bSaveHandles) {                                            \
    SetStdHandle(STD_INPUT_HANDLE,  (p)->hParentStdSave[0]);            \
    SetStdHandle(STD_OUTPUT_HANDLE, (p)->hParentStdSave[1]);            \
    SetStdHandle(STD_ERROR_HANDLE,  (p)->hParentStdSave[2]);            \
    } APXMACRO_END

#define REDIRECT_STD_HANDLES(p) \
    APXMACRO_BEGIN                                                      \
    if ((p)->bSaveHandles) {                                            \
    SetStdHandle(STD_INPUT_HANDLE,  (p)->hChildStdInp);                 \
    SetStdHandle(STD_OUTPUT_HANDLE, (p)->hChildStdOut);                 \
    SetStdHandle(STD_ERROR_HANDLE,  (p)->hChildStdErr);                 \
    } APXMACRO_END

typedef struct APXPROCESS {
    DWORD                   dwChildStatus;
    DWORD                   dwOptions;
    PROCESS_INFORMATION     stProcInfo;
    /* Size of chars for ANSI/Unicode programs */
    DWORD                   chSize;
    /* application working path */
    LPWSTR                  szWorkingPath;
    /* executable name */
    LPWSTR                  szApplicationExec;
    /* command line (first arg is program name for argv[0]) */
    LPWSTR                  szCommandLine;
    LPWSTR                  lpEnvironment;
    /* set of child inherited pipes */
    HANDLE                  hChildStdInp;
    HANDLE                  hChildStdOut;
    HANDLE                  hChildStdErr;
    /* parent ends of child pipes */
    HANDLE                  hChildInpWr;
    HANDLE                  hChildOutRd;
    HANDLE                  hChildErrRd;
    /* Saved console pipes */
    HANDLE                  hParentStdSave[3];
    HANDLE                  hWorkerThreads[3];
    HANDLE                  hUserToken;
    HANDLE                  hCurrentProcess;
    BOOL                    bSaveHandles;
    /** callback function */
    LPAPXFNCALLBACK         fnUserCallback;
    LPSECURITY_ATTRIBUTES   lpSA;
    LPVOID                  lpSD;
    BYTE                    bSD[SECURITY_DESCRIPTOR_MIN_LENGTH];
    BYTE                    bSA[sizeof(SECURITY_ATTRIBUTES)];

} APXPROCESS, *LPAPXPROCESS;

/** Process worker thread
 * Monitors the process thread
 */
static DWORD WINAPI __apxProcWorkerThread(LPVOID lpParameter)
{
    APXHANDLE hProcess = (APXHANDLE)lpParameter;
    LPAPXPROCESS lpProc;
    DWORD dwExitCode = 0;

    lpProc = APXHANDLE_DATA(hProcess);
    /* Wait util a process has finished its initialization.
     */
    WaitForInputIdle(lpProc->stProcInfo.hProcess, INFINITE);
    lpProc->dwChildStatus |= CHILD_INITIALIZED;
    /* Wait until the child process exits */
    if (WaitForSingleObject(lpProc->stProcInfo.hProcess,
                            INFINITE) == WAIT_OBJECT_0) {
        lpProc->dwChildStatus |= CHILD_MAINTREAD_FINISHED;

        /* store worker's exit code as VM exit code for later use */
        GetExitCodeProcess(lpProc->stProcInfo.hProcess, &dwExitCode);
        apxLogWrite(APXLOG_MARK_DEBUG "Child process exit code %d", dwExitCode);
        apxSetVmExitCode(dwExitCode);

#if 0
        if (hProcess->fnCallback)
            (*hProcess->fnCallback)(hProcess, WM_QUIT, (WPARAM)dwExitCode, 0);
        lpProc->dwChildStatus &= ~CHILD_RUNNING;
        GetExitCodeProcess(lpProc->stProcInfo.hProcess, &dwExitCode);

        /* Close that handle */
#endif
    }
    ExitThread(0);
    return 0;
}

static DWORD WINAPI __apxProcStdoutThread(LPVOID lpParameter)
{
    APXHANDLE hProcess = (APXHANDLE)lpParameter;
    LPAPXPROCESS lpProc;
    APXCALLHOOK *lpCall;
    INT ch;
    DWORD dwReaded;
    lpProc = APXHANDLE_DATA(hProcess);
    while (lpProc->dwChildStatus & CHILD_RUNNING) {
        ch = 0;
        if (!ReadFile(lpProc->hChildOutRd, &ch, lpProc->chSize,
                      &dwReaded, NULL) || !dwReaded) {

            break;
        }
        if (lpProc->fnUserCallback)
            (*lpProc->fnUserCallback)(hProcess, WM_CHAR, (WPARAM)ch, (LPARAM)0);
        TAILQ_FOREACH(lpCall, &hProcess->lCallbacks, queue) {
            (*lpCall->fnCallback)(hProcess, WM_CHAR, (WPARAM)ch, (LPARAM)0);
        }
        dwReaded = 0;
        SwitchToThread();
    }
    ExitThread(0);
    return 0;
}

static DWORD WINAPI __apxProcStderrThread(LPVOID lpParameter)
{
    APXHANDLE hProcess = (APXHANDLE)lpParameter;
    LPAPXPROCESS lpProc;
    APXCALLHOOK *lpCall;
    INT ch;
    DWORD dwReaded;
    lpProc = APXHANDLE_DATA(hProcess);
    while (lpProc->dwChildStatus & CHILD_RUNNING) {
        if (!ReadFile(lpProc->hChildErrRd, &ch, lpProc->chSize,
                      &dwReaded, NULL) || !dwReaded) {

            break;
        }
        if (lpProc->fnUserCallback)
            (*lpProc->fnUserCallback)(hProcess, WM_CHAR, (WPARAM)ch, (LPARAM)1);
        TAILQ_FOREACH(lpCall, &hProcess->lCallbacks, queue) {
            (*lpCall->fnCallback)(hProcess, WM_CHAR, (WPARAM)ch, (LPARAM)1);
        }

        dwReaded = 0;
        SwitchToThread();
    }

    ExitThread(0);
    return 0;
}

static DWORD __apxProcessPutc(LPAPXPROCESS lpProc, INT ch, DWORD dwSize)
{
    if (lpProc->dwChildStatus & CHILD_RUNNING) {
        DWORD wr = 0;
        if (WriteFile(lpProc->hChildInpWr, &ch, dwSize, &wr, NULL) &&
                      wr == dwSize) {
            return 1;
        }
    }

    return 0;
}

static DWORD __apxProcessPuts(LPAPXPROCESS lpProc, LPCTSTR szString)
{
    DWORD l, n = 0;
    l = lstrlen(szString) * lpProc->chSize;
    if (lpProc->dwChildStatus & CHILD_RUNNING && l) {
        DWORD wr = 0;
        while (TRUE) {
            if (WriteFile(lpProc->hChildInpWr, szString, l,
                          &wr, NULL)) {
                n += wr;
                if (wr < l) {
                    l -= wr;
                    szString += wr;
                }
                else {
                    /* Flush the buffer */
                    FlushFileBuffers(lpProc->hChildInpWr);
                    break;
                }
            }
            else
                break;
        }
    }

    return n;
}

static DWORD __apxProcessWrite(LPAPXPROCESS lpProc, LPCVOID lpData, DWORD dwLen)
{
    LPBYTE buf = (LPBYTE)lpData;
    DWORD  n = 0;
    if (!lpData || !dwLen)
        return 0;

    if (lpProc->dwChildStatus & CHILD_RUNNING) {
        DWORD wr = 0;
        while (lpProc->dwChildStatus & CHILD_RUNNING) {
            if (WriteFile(lpProc->hChildInpWr, buf, dwLen,
                          &wr, NULL)) {
                n += wr;
                if (wr < dwLen) {
                    dwLen -= wr;
                    buf += wr;
                }
                else
                    break;
            }
            else
                break;
        }
    }

    return n;
}

/** Helper functions */
static BOOL __apxProcCreateChildPipes(LPAPXPROCESS lpProc)
{
    BOOL   rv = FALSE;

    if (!CreatePipe(&(lpProc->hChildStdInp),
                    &(lpProc->hChildInpWr),
                    lpProc->lpSA, 0)) {
        apxLogWrite(APXLOG_MARK_SYSERR);
        goto cleanup;
    }
    if (!SetHandleInformation(lpProc->hChildInpWr,
                              HANDLE_FLAG_INHERIT, 0)) {
        apxLogWrite(APXLOG_MARK_SYSERR);
        goto cleanup;
    }

    if (!CreatePipe(&(lpProc->hChildOutRd),
                    &(lpProc->hChildStdOut),
                    lpProc->lpSA, 0)) {
        apxLogWrite(APXLOG_MARK_SYSERR);
        goto cleanup;
    }

    if (!SetHandleInformation(lpProc->hChildOutRd,
                              HANDLE_FLAG_INHERIT, 0)) {
        apxLogWrite(APXLOG_MARK_SYSERR);
        goto cleanup;
    }

    if (!CreatePipe(&(lpProc->hChildErrRd),
                    &(lpProc->hChildStdErr),
                    lpProc->lpSA, 0)) {
        apxLogWrite(APXLOG_MARK_SYSERR);
        goto cleanup;
    }

    if (!SetHandleInformation(lpProc->hChildErrRd,
                              HANDLE_FLAG_INHERIT, 0)) {
        apxLogWrite(APXLOG_MARK_SYSERR);
        goto cleanup;
    }
    rv = TRUE;
cleanup:

    return rv;
}

/* Close the process.
 * Create the remote thread and call the ExitProcess
 * Terminate the process, if all of the above fails.
 */
static BOOL __apxProcessClose(APXHANDLE hProcess)
{
    LPAPXPROCESS lpProc;
    DYNLOAD_FPTR_DECLARE(CreateRemoteThread);
    DYNLOAD_FPTR_DECLARE(ExitProcess);

    UINT    uExitCode = CHILD_TERMINATE_CODE; /* Could be any value like my birthday ;-)*/
    HANDLE  hDup, hRemote;

    lpProc = APXHANDLE_DATA(hProcess);
    CHECK_IF_ACTIVE(lpProc);

    /* Try to close the child's stdin first */
    SAFE_CLOSE_HANDLE(lpProc->hChildInpWr);
    /* Wait 1 sec for child process to
     * recognize that the stdin has been closed.
     */
    if (WaitForSingleObject(lpProc->stProcInfo.hProcess, 1000) == WAIT_OBJECT_0)
        goto cleanup;

    CHECK_IF_ACTIVE(lpProc);

    /* Try to create the remote thread in the child address space */
    DYNLOAD_FPTR_ADDRESS(CreateRemoteThread, KERNEL32);
    if (DuplicateHandle(lpProc->hCurrentProcess,
                        lpProc->stProcInfo.hProcess,
                        lpProc->hCurrentProcess,
                        &hDup,
                        PROCESS_ALL_ACCESS,
                        FALSE, 0)) {
        DYNLOAD_FPTR_ADDRESS(ExitProcess, KERNEL32);
        /* Now call the ExitProcess from inside the client
         * This will safely unload all the dll's.
         */
        hRemote = DYNLOAD_CALL(CreateRemoteThread)(hDup,
                                NULL, 0,
                                (LPTHREAD_START_ROUTINE)DYNLOAD_FPTR(ExitProcess),
                                (PVOID)&uExitCode, 0, NULL);
        if (!IS_INVALID_HANDLE(hRemote)) {
            if (WaitForSingleObject(lpProc->stProcInfo.hProcess,
                    2000) == WAIT_OBJECT_0) {

            }
            else {
                TerminateProcess(lpProc->stProcInfo.hProcess, CHILD_TERMINATE_CODE);
            }
            CloseHandle(hRemote);
        }
        CloseHandle(hDup);
        goto cleanup;
    }

    TerminateProcess(lpProc->stProcInfo.hProcess, CHILD_TERMINATE_CODE);

cleanup:
     /* Close the process handle */
    SAFE_CLOSE_HANDLE(lpProc->stProcInfo.hProcess);
    lpProc->dwChildStatus &= ~CHILD_RUNNING;
    return TRUE;
}

static BOOL __apxProcessCallback(APXHANDLE hObject, UINT uMsg,
                                 WPARAM wParam, LPARAM lParam)
{
    LPAPXPROCESS lpProc;

    lpProc = APXHANDLE_DATA(hObject);
    /* Call the user supplied callback first */
    if (lpProc->fnUserCallback)
        (*lpProc->fnUserCallback)(hObject, uMsg, wParam, lParam);
    switch (uMsg) {
        case WM_CLOSE:
            if (lpProc->dwChildStatus & CHILD_RUNNING) {
                __apxProcessClose(hObject);
                /* Wait for all worker threads to exit */
                WaitForMultipleObjects(3, lpProc->hWorkerThreads, TRUE, INFINITE);
            }
            SAFE_CLOSE_HANDLE(lpProc->stProcInfo.hProcess);

            /* Close parent side of the pipes */
            SAFE_CLOSE_HANDLE(lpProc->hChildInpWr);
            SAFE_CLOSE_HANDLE(lpProc->hChildOutRd);
            SAFE_CLOSE_HANDLE(lpProc->hChildErrRd);

            SAFE_CLOSE_HANDLE(lpProc->hWorkerThreads[0]);
            SAFE_CLOSE_HANDLE(lpProc->hWorkerThreads[1]);
            SAFE_CLOSE_HANDLE(lpProc->hWorkerThreads[2]);
            SAFE_CLOSE_HANDLE(lpProc->hUserToken);
            apxFree(lpProc->szApplicationExec);
            apxFree(lpProc->szCommandLine);
            apxFree(lpProc->szWorkingPath);
            RESTORE_STD_HANDLES(lpProc);
            SAFE_CLOSE_HANDLE(lpProc->hCurrentProcess);
            if (lpProc->lpEnvironment)
                FreeEnvironmentStringsW(lpProc->lpEnvironment);

        case WM_QUIT:
            /* The process has finished
             * This is a WorkerThread message
             */
            lpProc->dwChildStatus &= ~CHILD_RUNNING;
        break;
        case WM_CHAR:
            __apxProcessPutc(lpProc, (INT)lParam, lpProc->chSize);
        break;
        case WM_SETTEXT:
            if (wParam)
                __apxProcessWrite(lpProc, (LPCVOID)lParam, (DWORD)wParam);
            else
                __apxProcessPuts(lpProc, (LPCTSTR)lParam);
        break;
        default:
        break;
    }

    return TRUE;
}

/** Create the process but don't start it
 *
 APXHANDLE
apxCreateProcessW(APXHANDLE hPool, DWORD dwOptions,
                  LPAPXFNCALLBACK fnCallback,
                  LPCWSTR szUsername, LPCWSTR szPassword,
                  BOOL bLogonAsService)
{
    APXHANDLE hProcess;
    LPAPXPROCESS lpProc;
    HANDLE hUserToken = NULL;
    if (szUsername && (APX_GET_OSLEVEL() >= APX_WINVER_NT_4)) {
        HANDLE hUser;
#ifndef _UNICODE
        WCHAR wsUsername[256];
        WCHAR wsPassword[256];
        AsciiToWide(szUsername, wsUsername);
        AsciiToWide(szPassword, wsPassword);
#else
        LPCWSTR wsUsername = szUsername;
        LPCWSTR wsPassword = szPassword;
#endif

 */


 APXHANDLE
apxCreateProcessA(APXHANDLE hPool, DWORD dwOptions,
                  LPAPXFNCALLBACK fnCallback,
                  LPCSTR szUsername, LPCSTR szPassword,
                  BOOL bLogonAsService)
{

    if (szUsername && szPassword) {
        WCHAR wsUsername[256];
        WCHAR wsPassword[256];
        AsciiToWide(szUsername, wsUsername);
        AsciiToWide(szPassword, wsPassword);
        return apxCreateProcessW(hPool, dwOptions, fnCallback,
                                 wsUsername, wsPassword, bLogonAsService);
    }
    else
        return apxCreateProcessW(hPool, dwOptions, fnCallback,
                                 NULL, NULL, bLogonAsService);
}

APXHANDLE
apxCreateProcessW(APXHANDLE hPool, DWORD dwOptions,
                  LPAPXFNCALLBACK fnCallback,
                  LPCWSTR szUsername, LPCWSTR szPassword,
                  BOOL bLogonAsService)
{
    APXHANDLE hProcess;
    LPAPXPROCESS lpProc;
    HANDLE hUserToken = NULL;
    /* CreateProcessAsUser is supported only on NT */
    if (szUsername && (APX_GET_OSLEVEL() >= APX_WINVER_NT_4)) {
        HANDLE hUser;
        if (!LogonUserW(szUsername,
                        NULL,
                        szPassword,
                        bLogonAsService ? LOGON32_LOGON_SERVICE : LOGON32_LOGON_NETWORK,
                        LOGON32_PROVIDER_DEFAULT,
                        &hUser)) {
            /* Logon Failed */
            apxLogWrite(APXLOG_MARK_SYSERR);
            return NULL;
        }
        if (!DuplicateTokenEx(hUser,
                              TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,
                              NULL,
                              SecurityImpersonation,
                              TokenPrimary,
                              &hUserToken)) {
            CloseHandle(hUser);
            /* Failed to duplicate the user token */
            apxLogWrite(APXLOG_MARK_SYSERR);
            return NULL;
        }
        if (!ImpersonateLoggedOnUser(hUserToken)) {
            CloseHandle(hUser);
            CloseHandle(hUserToken);
            /* failed to impersonate the logged user */
            apxLogWrite(APXLOG_MARK_SYSERR);
            return NULL;
        }
        CloseHandle(hUser);
    }

    hProcess = apxHandleCreate(hPool, APXHANDLE_HAS_EVENT,
                               NULL, sizeof(APXPROCESS),
                               __apxProcessCallback);
    if (IS_INVALID_HANDLE(hProcess))
        return NULL;
    hProcess->dwType = APXHANDLE_TYPE_PROCESS;
    lpProc = APXHANDLE_DATA(hProcess);
    lpProc->dwOptions = dwOptions;
    lpProc->fnUserCallback = fnCallback;
    lpProc->hUserToken  = hUserToken;
    /* set the CHAR length */
    if (dwOptions & CREATE_UNICODE_ENVIRONMENT)
        lpProc->chSize = sizeof(WCHAR);
    else
        lpProc->chSize = sizeof(CHAR);
#if 1
    DuplicateHandle(GetCurrentProcess(),
                    GetCurrentProcess(),
                    GetCurrentProcess(),
                    &lpProc->hCurrentProcess,
                    PROCESS_ALL_ACCESS,
                    FALSE,
                    0);
#else
    lpProc->hCurrentProcess = GetCurrentProcess();
#endif
    lpProc->lpSD = &lpProc->bSD;

    InitializeSecurityDescriptor(lpProc->lpSD, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(lpProc->lpSD, -1, 0, 0);

    lpProc->lpSA = (LPSECURITY_ATTRIBUTES)&lpProc->bSA[0];
    lpProc->lpSA->nLength = sizeof (SECURITY_ATTRIBUTES);
    lpProc->lpSA->lpSecurityDescriptor = lpProc->lpSD;
    lpProc->lpSA->bInheritHandle = TRUE;

    return hProcess;
}

static WCHAR _desktop_name[] =
    {'W', 'i', 'n', 's', 't', 'a', '0', '\\', 'D', 'e', 'f', 'a', 'u', 'l', 't', 0};

BOOL
apxProcessExecute(APXHANDLE hProcess)
{
    LPAPXPROCESS lpProc;
    STARTUPINFOW si;
    DWORD id;
    BOOL  bS = FALSE;

    if (hProcess->dwType != APXHANDLE_TYPE_PROCESS)
        return FALSE;

    lpProc = APXHANDLE_DATA(hProcess);
    /* don't allow multiple execute calls on the same object */
    if (lpProc->dwChildStatus & PROC_INITIALIZED)
        return FALSE;
    lpProc->bSaveHandles = TRUE;
    SAVE_STD_HANDLES(lpProc);
    if (!__apxProcCreateChildPipes(lpProc))
        goto cleanup;
    REDIRECT_STD_HANDLES(lpProc);
    AplZeroMemory(&si, sizeof(STARTUPINFO));

    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    /* Set the redirected handles */
    si.hStdOutput = lpProc->hChildStdOut;
    si.hStdError  = lpProc->hChildStdErr;
    si.hStdInput  = lpProc->hChildStdInp;

    if (lpProc->lpEnvironment)
        FreeEnvironmentStringsW(lpProc->lpEnvironment);
    lpProc->lpEnvironment = GetEnvironmentStringsW();

    if (!IS_INVALID_HANDLE(lpProc->hUserToken)) {
        si.lpDesktop = _desktop_name;
        bS = CreateProcessAsUserW(lpProc->hUserToken,
                                  lpProc->szApplicationExec,
                                  lpProc->szCommandLine,
                                  lpProc->lpSA,
                                  NULL,
                                  TRUE,
                                  CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT | lpProc->dwOptions,
                                  lpProc->lpEnvironment,
                                  lpProc->szWorkingPath,
                                  &si,
                                  &(lpProc->stProcInfo));
    }
    else {
        OutputDebugStringW(lpProc->szApplicationExec);
        OutputDebugStringW(lpProc->szCommandLine);

        bS = CreateProcessW(lpProc->szApplicationExec,
                            lpProc->szCommandLine,
                            lpProc->lpSA,
                            NULL,
                            TRUE,
                            CREATE_SUSPENDED  | CREATE_UNICODE_ENVIRONMENT | lpProc->dwOptions,
                            lpProc->lpEnvironment,
                            lpProc->szWorkingPath,
                            &si,
                            &(lpProc->stProcInfo));
    }
    /* Close unused sides of pipes */
    SAFE_CLOSE_HANDLE(lpProc->hChildStdInp);
    SAFE_CLOSE_HANDLE(lpProc->hChildStdOut);
    SAFE_CLOSE_HANDLE(lpProc->hChildStdErr);
    if (!bS)
        goto cleanup;
    /* Set the running flag */
    lpProc->dwChildStatus |= (CHILD_RUNNING | PROC_INITIALIZED);

    lpProc->hWorkerThreads[0] = CreateThread(NULL, 0, __apxProcStdoutThread,
                                             hProcess, 0, &id);
    lpProc->hWorkerThreads[1] = CreateThread(NULL, 0, __apxProcStderrThread,
                                             hProcess, 0, &id);
    ResumeThread(lpProc->stProcInfo.hThread);
    lpProc->hWorkerThreads[2] = CreateThread(NULL, 0, __apxProcWorkerThread,
                                            hProcess, 0, &id);

    SAFE_CLOSE_HANDLE(lpProc->stProcInfo.hThread);
    /* Close child handles first */
    return TRUE;
cleanup:
    /* Close parent side of the pipes */
    SAFE_CLOSE_HANDLE(lpProc->hChildInpWr);
    SAFE_CLOSE_HANDLE(lpProc->hChildOutRd);
    SAFE_CLOSE_HANDLE(lpProc->hChildErrRd);

    return FALSE;
}

BOOL
apxProcessSetExecutableA(APXHANDLE hProcess, LPCSTR szName)
{
    LPAPXPROCESS lpProc;

    if (hProcess->dwType != APXHANDLE_TYPE_PROCESS)
        return FALSE;

    lpProc = APXHANDLE_DATA(hProcess);
    apxFree(lpProc->szApplicationExec);
    lpProc->szApplicationExec = apxPoolWStrdupA(hProcess->hPool, szName);
    OutputDebugStringW(lpProc->szApplicationExec);
    return lpProc->szApplicationExec != NULL;
}

BOOL
apxProcessSetExecutableW(APXHANDLE hProcess, LPCWSTR szName)
{
    LPAPXPROCESS lpProc;

    if (hProcess->dwType != APXHANDLE_TYPE_PROCESS)
        return FALSE;

    lpProc = APXHANDLE_DATA(hProcess);
    apxFree(lpProc->szApplicationExec);
    lpProc->szApplicationExec = apxPoolStrdupW(hProcess->hPool, szName);
    OutputDebugStringW(lpProc->szApplicationExec);
    return lpProc->szApplicationExec != NULL;
}

BOOL
apxProcessSetCommandLineA(APXHANDLE hProcess, LPCSTR szCmdline)
{
    LPAPXPROCESS lpProc;

    if (hProcess->dwType != APXHANDLE_TYPE_PROCESS)
        return FALSE;

    lpProc = APXHANDLE_DATA(hProcess);
    apxFree(lpProc->szCommandLine);
    lpProc->szCommandLine = apxPoolWStrdupA(hProcess->hPool, szCmdline);

    return lpProc->szCommandLine != NULL;
}

BOOL
apxProcessSetCommandLineW(APXHANDLE hProcess, LPCWSTR szCmdline)
{
    LPAPXPROCESS lpProc;

    if (hProcess->dwType != APXHANDLE_TYPE_PROCESS)
        return FALSE;

    lpProc = APXHANDLE_DATA(hProcess);
    apxFree(lpProc->szCommandLine);
    lpProc->szCommandLine = apxPoolStrdupW(hProcess->hPool, szCmdline);

    return lpProc->szCommandLine != NULL;
}

BOOL
apxProcessSetWorkingPathA(APXHANDLE hProcess, LPCSTR szPath)
{
    LPAPXPROCESS lpProc;

    if (hProcess->dwType != APXHANDLE_TYPE_PROCESS)
        return FALSE;

    lpProc = APXHANDLE_DATA(hProcess);
    apxFree(lpProc->szWorkingPath);
    lpProc->szWorkingPath = apxPoolWStrdupA(hProcess->hPool, szPath);

    return lpProc->szWorkingPath != NULL;
}

BOOL
apxProcessSetWorkingPathW(APXHANDLE hProcess, LPCWSTR szPath)
{
    LPAPXPROCESS lpProc;

    if (hProcess->dwType != APXHANDLE_TYPE_PROCESS)
        return FALSE;

    lpProc = APXHANDLE_DATA(hProcess);
    apxFree(lpProc->szWorkingPath);
    if (!szPath) {
        /* Clear the WorkingPath */
        lpProc->szWorkingPath = NULL;
        return TRUE;
    }
    lpProc->szWorkingPath = apxPoolWStrdupW(hProcess->hPool, szPath);

    return lpProc->szWorkingPath != NULL;
}

DWORD
apxProcessPutcA(APXHANDLE hProcess, INT ch)
{
   LPAPXPROCESS lpProc;

    if (hProcess->dwType != APXHANDLE_TYPE_PROCESS)
        return 0;

    lpProc = APXHANDLE_DATA(hProcess);
    return __apxProcessPutc(lpProc, ch, sizeof(CHAR));
}

DWORD
apxProcessPutcW(APXHANDLE hProcess, INT ch)
{
   LPAPXPROCESS lpProc;

    if (hProcess->dwType != APXHANDLE_TYPE_PROCESS)
        return 0;

    lpProc = APXHANDLE_DATA(hProcess);
    return __apxProcessPutc(lpProc, ch, sizeof(WCHAR));
}

DWORD
apxProcessPutsA(APXHANDLE hProcess, LPCSTR szString)
{
    LPAPXPROCESS lpProc;
    DWORD len;
    if (hProcess->dwType != APXHANDLE_TYPE_PROCESS || !szString)
        return 0;
    lpProc = APXHANDLE_DATA(hProcess);
    len = lstrlenA(szString);
    len = __apxProcessWrite(lpProc, szString, len);
    if (len) {
        FlushFileBuffers(lpProc->hChildInpWr);
        return len;
    }
    else
        return 0;
}

DWORD
apxProcessPutsW(APXHANDLE hProcess, LPCWSTR szString)
{
    LPAPXPROCESS lpProc;
    DWORD len;
    if (hProcess->dwType != APXHANDLE_TYPE_PROCESS || !szString)
        return 0;
    lpProc = APXHANDLE_DATA(hProcess);
    len = lstrlenW(szString);
    len = __apxProcessWrite(lpProc, szString, len * 2);
    if (len) {
        FlushFileBuffers(lpProc->hChildInpWr);
        return (len / sizeof(WCHAR));
    }
    else
        return 0;
}

DWORD
apxProcessWrite(APXHANDLE hProcess, LPCVOID lpData, DWORD dwLen)
{
    LPAPXPROCESS lpProc;
    if (hProcess->dwType != APXHANDLE_TYPE_PROCESS)
        return 0;

    lpProc = APXHANDLE_DATA(hProcess);

    return __apxProcessWrite(lpProc, lpData, dwLen);
}

BOOL
apxProcessFlushStdin(APXHANDLE hProcess)
{
   LPAPXPROCESS lpProc;

    if (hProcess->dwType != APXHANDLE_TYPE_PROCESS)
        return FALSE;

    lpProc = APXHANDLE_DATA(hProcess);

    if (lpProc->dwChildStatus & CHILD_RUNNING) {
        return FlushFileBuffers(lpProc->hChildInpWr);
    }

    return FALSE;
}

VOID
apxProcessCloseInputStream(APXHANDLE hProcess)
{
   if (hProcess->dwType == APXHANDLE_TYPE_PROCESS) {
       LPAPXPROCESS lpProc = APXHANDLE_DATA(hProcess);
       if (lpProc->dwChildStatus & CHILD_RUNNING)
           SAFE_CLOSE_HANDLE(lpProc->hChildInpWr);
   }
}

DWORD
apxProcessWait(APXHANDLE hProcess, DWORD dwMilliseconds, BOOL bKill)
{
   LPAPXPROCESS lpProc;

    if (hProcess->dwType != APXHANDLE_TYPE_PROCESS)
        return WAIT_ABANDONED;

    lpProc = APXHANDLE_DATA(hProcess);

    if (lpProc->dwChildStatus & CHILD_RUNNING) {
        DWORD rv = WaitForMultipleObjects(3, lpProc->hWorkerThreads,
                                          TRUE, dwMilliseconds);
        if (rv == WAIT_TIMEOUT && bKill) {
            __apxProcessCallback(hProcess, WM_CLOSE, 0, 0);
        }
        return rv;
    }
    else
        return WAIT_OBJECT_0;
}

BOOL
apxProcessRunning(APXHANDLE hProcess)
{
   LPAPXPROCESS lpProc;

    if (hProcess->dwType != APXHANDLE_TYPE_PROCESS)
        return FALSE;

    lpProc = APXHANDLE_DATA(hProcess);

    return (lpProc->dwChildStatus & CHILD_RUNNING);
}

DWORD
apxProcessGetPid(APXHANDLE hProcess)
{
   LPAPXPROCESS lpProc;

    if (hProcess->dwType != APXHANDLE_TYPE_PROCESS)
        return 0;

    lpProc = APXHANDLE_DATA(hProcess);

    return lpProc->stProcInfo.dwProcessId;
}

static LPWSTR __apxStrQuote(LPWSTR lpDest, LPCWSTR szSrc)
{
    LPWSTR p;
    BOOL   space = FALSE, quote = FALSE;

    /* Find if string has embeded spaces, add quotes only if no quotes found
     */
    for (p = (LPWSTR)szSrc; *p; p++) {
        if (*p == L' ' || *p == '\t') {
            space = TRUE;
        } else if (*p == L'"') {
            quote = TRUE;
        }
    }
    p = lpDest;
    if (space && !quote) *p++ = L'"';
    while (*szSrc) {
        *p++ = *szSrc++;
    }
    if (space && !quote) *p++ = L'"';
    return p;
}

BOOL
apxProcessSetCommandArgsW(APXHANDLE hProcess, LPCWSTR szTitle,
                          DWORD dwArgc, LPCWSTR *lpArgs)
{
    LPAPXPROCESS lpProc;
    DWORD  i, l = 0;
    LPWSTR p;
    if (hProcess->dwType != APXHANDLE_TYPE_PROCESS)
        return FALSE;

    lpProc = APXHANDLE_DATA(hProcess);
    apxFree(lpProc->szCommandLine);

    l = lstrlenW(szTitle) + 3;
    for (i = 0; i < dwArgc; i++) {
        int q = 0;
        l += (lstrlenW(lpArgs[i]) + 3);
        l += q;
    }
    p = lpProc->szCommandLine = apxPoolAlloc(hProcess->hPool, l * sizeof(WCHAR));
    p = __apxStrQuote(p, szTitle);
    for (i = 0; i < dwArgc; i++) {
        *p++ = L' ';
        p = __apxStrQuote(p, lpArgs[i]);
    }
    *p = L'\0';
    OutputDebugStringW(lpProc->szCommandLine);
    return lpProc->szCommandLine != NULL;
}

