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

#define SAFE_CLOSE_SCH(h) \
    if ((h) != NULL && (h) != INVALID_HANDLE_VALUE) {   \
        CloseServiceHandle((h));                        \
        (h) = NULL;                                     \
    }

typedef struct APXSERVICE {
    /* Are we a service manager or we are the service itself */
    BOOL            bManagerMode;
    /* Handle to the current service */
    SC_HANDLE       hService;
    /* Handle of the Service manager */
    SC_HANDLE       hManager;
    APXSERVENTRY    stServiceEntry;

} APXSERVICE, *LPAPXSERVICE;

/** Maps dwCurrentState to the constant name described on https://docs.microsoft.com/en-us/windows/win32/api/winsvc/ns-winsvc-service_status */
static const char* gSzCurrentState[] = {
    "",
    "SERVICE_STOPPED",
    "SERVICE_START_PENDING",
    "SERVICE_STOP_PENDING",
    "SERVICE_RUNNING",
    "SERVICE_CONTINUE_PENDING",
    "SERVICE_PAUSE_PENDING",
    "SERVICE_PAUSED"
};

const char* apxServiceGetStateName(DWORD dwCurrentState) {
    return gSzCurrentState[dwCurrentState < 0 ? 0 : dwCurrentState > _countof(gSzCurrentState) ? 0 : dwCurrentState];
}

static WCHAR __invalidPathChars[] = L"<>:\"/\\:|?*";
static BOOL __apxIsValidServiceName(LPCWSTR szServiceName)
{
    WCHAR ch;
    int   i = 0;
    while ((ch = szServiceName[i++])) {
        int j = 0;
        while (__invalidPathChars[j]) {
            if (ch < 30 || ch == __invalidPathChars[j++]) {
                apxDisplayError(FALSE, NULL, 0, "Service '%S' contains invalid character '%C'",
                                szServiceName, ch);
                return FALSE;
            }
        }
    }
    if (i > 256) {
        apxDisplayError(FALSE, NULL, 0, "Service name too long %S", szServiceName);
        return FALSE;
    }
    return TRUE;
}

static BOOL __apxServiceCallback(APXHANDLE hObject, UINT uMsg,
                                 WPARAM wParam, LPARAM lParam)
{
    LPAPXSERVICE lpService;

    lpService = APXHANDLE_DATA(hObject);
    switch (uMsg) {
        case WM_CLOSE:
            apxFree(lpService->stServiceEntry.lpConfig);
            lpService->stServiceEntry.lpConfig = NULL;
            SAFE_CLOSE_SCH(lpService->hService);
            SAFE_CLOSE_SCH(lpService->hManager);
        break;
        default:
        break;
    }
    return TRUE;
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
}

APXHANDLE
apxCreateService(APXHANDLE hPool, DWORD dwOptions, BOOL bManagerMode)
{
    APXHANDLE    hService;
    LPAPXSERVICE lpService;
    SC_HANDLE    hManager;

    if (!(hManager = OpenSCManager(NULL, NULL, dwOptions))) {
        if (GetLastError() != ERROR_ACCESS_DENIED)
            apxLogWrite(APXLOG_MARK_SYSERR);
        return NULL;
    }
    hService = apxHandleCreate(hPool, 0,
                               NULL, sizeof(APXSERVICE),
                               __apxServiceCallback);
    if (IS_INVALID_HANDLE(hService)) {
        apxLogWrite(APXLOG_MARK_ERROR "Failed to Create Handle for Service");
        return NULL;
    }
    hService->dwType = APXHANDLE_TYPE_SERVICE;
    lpService = APXHANDLE_DATA(hService);
    lpService->hManager     = hManager;
    lpService->bManagerMode = bManagerMode;

    return hService;
}

BOOL
apxServiceOpen(APXHANDLE hService, LPCWSTR szServiceName, DWORD dwOptions)
{
    LPAPXSERVICE lpService;
    DWORD dwNeeded;
    LPSERVICE_DELAYED_AUTO_START_INFO lpDelayedInfo;

    if (hService->dwType != APXHANDLE_TYPE_SERVICE)
        return FALSE;

    lpService = APXHANDLE_DATA(hService);
    /* Manager mode cannot handle services */
    if (lpService->bManagerMode)
        return FALSE;

    /* Close any previous instance
     * Same handle can manage multiple services
     */
    SAFE_CLOSE_SCH(lpService->hService);
    *lpService->stServiceEntry.szServiceDescription = L'\0';
    *lpService->stServiceEntry.szObjectName = L'\0';
    apxFree(lpService->stServiceEntry.lpConfig);
    lpService->stServiceEntry.lpConfig = NULL;
    /* Open the service */
    lpService->hService = OpenServiceW(lpService->hManager,
                                       szServiceName,
                                       dwOptions);

    if (IS_INVALID_HANDLE(lpService->hService)) {
        apxLogWrite(APXLOG_MARK_SYSERR);
        return FALSE;
    }
    lstrlcpyW(lpService->stServiceEntry.szServiceName, SIZ_RESLEN, szServiceName);
    if (!apxGetServiceDescriptionW(szServiceName,
                                   lpService->stServiceEntry.szServiceDescription,
                                   SIZ_DESLEN)) {
        apxLogWrite(APXLOG_MARK_WARN "Failed to obtain service description for '%S'", szServiceName);
        lpService->stServiceEntry.szServiceDescription[0] = L'\0';
    }
    if (!apxGetServiceUserW(szServiceName,
                            lpService->stServiceEntry.szObjectName,
                            SIZ_RESLEN)) {
        apxLogWrite(APXLOG_MARK_WARN "Failed to obtain service user name for '%S'", szServiceName);
        lpService->stServiceEntry.szObjectName[0] = L'\0';
    }
    if (!QueryServiceConfigW(lpService->hService, NULL, 0, &dwNeeded)) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            // This is expected. The call is expected to fail with the required
            // buffer size set in dwNeeded.
            // Clear the last error to prevent it being logged if a genuine
            // error occurs
            SetLastError(ERROR_SUCCESS);
        } else {
            apxLogWrite(APXLOG_MARK_SYSERR);
        }
    }
    lpService->stServiceEntry.lpConfig = (LPQUERY_SERVICE_CONFIGW)apxPoolAlloc(hService->hPool,
                                                                               dwNeeded);
    if (!QueryServiceConfigW(lpService->hService,
                             lpService->stServiceEntry.lpConfig,
                             dwNeeded, &dwNeeded)) {
    	return FALSE;
    }

    if (!QueryServiceConfig2W(lpService->hService, SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
                              NULL, 0, &dwNeeded)) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            // This is expected. The call is expected to fail with the required
            // buffer size set in dwNeeded.
            // Clear the last error to prevent it being logged if a genuine
            // error occurs
            SetLastError(ERROR_SUCCESS);
        } else {
            apxLogWrite(APXLOG_MARK_SYSERR);
        }
    }
    lpDelayedInfo = (LPSERVICE_DELAYED_AUTO_START_INFO) apxPoolAlloc(hService->hPool, dwNeeded);

    if (!QueryServiceConfig2W(lpService->hService, SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
                              (LPBYTE) lpDelayedInfo, dwNeeded, &dwNeeded)) {
    	return FALSE;
    }

    lpService->stServiceEntry.bDelayedStart = lpDelayedInfo->fDelayedAutostart;
    apxFree(lpDelayedInfo);
    return TRUE;
}

LPAPXSERVENTRY
apxServiceEntry(APXHANDLE hService, BOOL bRequeryStatus)
{
    LPAPXSERVICE lpService;

    if (hService->dwType != APXHANDLE_TYPE_SERVICE)
        return NULL;

    lpService = APXHANDLE_DATA(hService);
    /* Manager mode cannot handle services */
    if (lpService->bManagerMode)
        return NULL;

    if (bRequeryStatus && !QueryServiceStatus(lpService->hService,
                            &(lpService->stServiceEntry.stServiceStatus))) {
        apxLogWrite(APXLOG_MARK_SYSERR);
        return NULL;
    }

    return &lpService->stServiceEntry;
}

/* Set the service names etc...
 * If the ImagePath contains a space, it must be quoted
 */
BOOL
apxServiceSetNames(APXHANDLE hService,
                   LPCWSTR szImagePath,
                   LPCWSTR szDisplayName,
                   LPCWSTR szDescription,
                   LPCWSTR szUsername,
                   LPCWSTR szPassword)
{
    LPAPXSERVICE lpService;
    DWORD dwServiceType = SERVICE_NO_CHANGE;

    if (hService->dwType != APXHANDLE_TYPE_SERVICE)
        return FALSE;

    lpService = APXHANDLE_DATA(hService);
    /* Manager mode cannot handle services */
    if (lpService->bManagerMode)
        return FALSE;
    /* Check if the ServiceOpen has been called */
    if (IS_INVALID_HANDLE(lpService->hService))
        return FALSE;
    if (!ChangeServiceConfigW(lpService->hService,
                              SERVICE_NO_CHANGE,
                              SERVICE_NO_CHANGE,
                              SERVICE_NO_CHANGE,
                              szImagePath,
                              NULL,
                              NULL,
                              NULL,
                              szUsername,
                              szPassword,
                              szDisplayName)) {
        apxLogWrite(APXLOG_MARK_SYSERR);
        return FALSE;
    }
    if (szDescription) {
        SERVICE_DESCRIPTIONW desc;
        desc.lpDescription = (LPWSTR)szDescription;
        return ChangeServiceConfig2(lpService->hService,
                                    SERVICE_CONFIG_DESCRIPTION,
                                    &desc);
    }
    return TRUE;
}

BOOL
apxServiceSetOptions(APXHANDLE hService,
    LPCWSTR lpDependencies,
    DWORD dwServiceType,
    DWORD dwStartType,
    BOOL bDelayedStart,
    DWORD dwErrorControl)
{
    LPAPXSERVICE lpService;
    SERVICE_DELAYED_AUTO_START_INFO sDelayedInfo;

    if (hService->dwType != APXHANDLE_TYPE_SERVICE) {
        apxLogWrite(APXLOG_MARK_WARN "Can't set options for service.");
        return FALSE;
    }

    lpService = APXHANDLE_DATA(hService);
    /* Manager mode cannot handle services */
    if (lpService->bManagerMode) {
        apxLogWrite(APXLOG_MARK_WARN "Can't set options for service: Manager mode cannot handle services");
        return FALSE;
    }
    /* Check if the ServiceOpen has been called */
    if (IS_INVALID_HANDLE(lpService->hService)) {
        apxLogWrite(APXLOG_MARK_WARN "Can't set options for service: Service is not open.");
        return FALSE;
    }

    if (!ChangeServiceConfig(lpService->hService, dwServiceType,
                                   dwStartType, dwErrorControl,
                                   NULL, NULL, NULL,
                                   lpDependencies,
                                   NULL, NULL, NULL)) {
        apxLogWrite(APXLOG_MARK_WARN "Can't set options for service: Failed to change the configuration parameters.");
    	return FALSE;
    }

    if (dwStartType == SERVICE_AUTO_START) {
    	sDelayedInfo.fDelayedAutostart = bDelayedStart;
        if (!ChangeServiceConfig2A(lpService->hService, SERVICE_CONFIG_DELAYED_AUTO_START_INFO, &sDelayedInfo)) {
            apxLogWrite(APXLOG_MARK_WARN "Can't set options for service: Failed to change the optional configuration parameters.");
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL
__apxStopDependentServices(LPAPXSERVICE lpService)
{
    DWORD i;
    DWORD dwBytesNeeded;
    DWORD dwCount;

    LPENUM_SERVICE_STATUSW  lpDependencies = NULL;
    ENUM_SERVICE_STATUS     ess;
    SC_HANDLE               hDepService;
    SERVICE_STATUS_PROCESS  ssp;

    DWORD dwStartTime = GetTickCount();
    /* Use the 30-second time-out */
    DWORD dwTimeout   = 30000;

    /* Pass a zero-length buffer to get the required buffer size.
     */
    if (EnumDependentServicesW(lpService->hService,
                               SERVICE_ACTIVE,
                               lpDependencies, 0,
                               &dwBytesNeeded,
                               &dwCount)) {
         /* If the Enum call succeeds, then there are no dependent
          * services, so do nothing.
          */
         return TRUE;
    }
    else  {
        if (GetLastError() != ERROR_MORE_DATA)
            return FALSE; // Unexpected error

        /* Allocate a buffer for the dependencies.
         */
        lpDependencies = (LPENUM_SERVICE_STATUS) HeapAlloc(GetProcessHeap(),
                                                           HEAP_ZERO_MEMORY,
                                                           dwBytesNeeded);
        if (!lpDependencies)
            return FALSE;

        __try {
            /* Enumerate the dependencies. */
            if (!EnumDependentServicesW(lpService->hService,
                                        SERVICE_ACTIVE,
                                        lpDependencies,
                                        dwBytesNeeded,
                                        &dwBytesNeeded,
                                        &dwCount))
            return FALSE;

            for (i = 0; i < dwCount; i++)  {
                ess = *(lpDependencies + i);
                /* Open the service. */
                hDepService = OpenServiceW(lpService->hManager,
                                           ess.lpServiceName,
                                           SERVICE_STOP | SERVICE_QUERY_STATUS);

                if (!hDepService)
                   continue;
                if (lstrcmpiW(ess.lpServiceName, L"Tcpip") == 0 ||
                    lstrcmpiW(ess.lpServiceName, L"Afd") == 0)
                    continue;
                __try {
                    /* Send a stop code. */
                    if (!ControlService(hDepService,
                                        SERVICE_CONTROL_STOP,
                                        (LPSERVICE_STATUS) &ssp))
                    return FALSE;

                    /* Wait for the service to stop. */
                    while (ssp.dwCurrentState != SERVICE_STOPPED) {
                        Sleep(ssp.dwWaitHint);
                        if (!QueryServiceStatusEx(hDepService,
                                                  SC_STATUS_PROCESS_INFO,
                                                 (LPBYTE)&ssp,
                                                  sizeof(SERVICE_STATUS_PROCESS),
                                                 &dwBytesNeeded))
                        return FALSE;

                        if (ssp.dwCurrentState == SERVICE_STOPPED)
                            break;

                        if (GetTickCount() - dwStartTime > dwTimeout)
                            return FALSE;
                    }
                }
                __finally {
                    /* Always release the service handle. */
                    CloseServiceHandle(hDepService);
                }
            }
        }
        __finally {
            /* Always free the enumeration buffer. */
            HeapFree(GetProcessHeap(), 0, lpDependencies);
        }
    }
    return TRUE;
}

BOOL
apxServiceControl(APXHANDLE hService, DWORD dwControl, UINT uMsg,
                  LPAPXFNCALLBACK fnControlCallback,
                  LPVOID lpCbData)
{
    LPAPXSERVICE   lpService;
    SERVICE_STATUS stStatus;
    DWORD          dwPendingState = 0;
    DWORD          dwState = 0;
    DWORD          dwTick  = 0;
    DWORD          dwWait, dwCheck, dwStart, sleepMillis;
    BOOL           bStatus;

    if (hService->dwType != APXHANDLE_TYPE_SERVICE)
        return FALSE;

    lpService = APXHANDLE_DATA(hService);
    /* Manager mode cannot handle services */
    if (lpService->bManagerMode) {
        apxLogWrite(APXLOG_MARK_ERROR "apxServiceControl(): Manager mode cannot handle services, returning FALSE");
        return FALSE;
    }
    /* Check if the ServiceOpen has been called */
    if (IS_INVALID_HANDLE(lpService->hService)) {
        apxLogWrite(APXLOG_MARK_ERROR "apxServiceControl(): Service is not open, returning FALSE");
        return FALSE;
    }
    switch (dwControl) {
        case SERVICE_CONTROL_CONTINUE:
            dwPendingState = SERVICE_START_PENDING;
            dwState        = SERVICE_RUNNING;
            break;
        case SERVICE_CONTROL_STOP:
            dwPendingState = SERVICE_STOP_PENDING;
            dwState        = SERVICE_STOPPED;
            break;
        case SERVICE_CONTROL_PAUSE:
            dwPendingState = SERVICE_PAUSE_PENDING;
            dwState        = SERVICE_PAUSED;
            break;
        default:
            break;
    }
    /* user-defined controls */
    if (dwControl > 127 && dwControl < 224) {
        /* 128 ... 159  start signals
         * 160 ... 191  stop signals
         * 192 ... 223  pause signals
         */
        switch (dwControl & 0xE0) {
            case 0x80:
            case 0x90:
                dwPendingState = SERVICE_START_PENDING;
                dwState        = SERVICE_RUNNING;
                break;
            case 0xA0:
            case 0xB0:
                dwPendingState = SERVICE_STOP_PENDING;
                dwState        = SERVICE_STOPPED;
                break;
            case 0xC0:
            case 0xD0:
                dwPendingState = SERVICE_PAUSE_PENDING;
                dwState        = SERVICE_PAUSED;
                break;
            default:
                break;
        }
    }
    if (!dwPendingState && !dwState) {
        apxLogWrite(APXLOG_MARK_ERROR
            "apxServiceControl():  !dwPendingState(%d = %s) && !dwState(%d = %s); returning FALSE",
            dwPendingState,
            apxServiceGetStateName(dwPendingState),
            dwState,
            apxServiceGetStateName(dwState));
        return FALSE;
    }
    /* Now lets control */
    if (!QueryServiceStatus(lpService->hService, &stStatus)) {
        apxLogWrite(APXLOG_MARK_ERROR "apxServiceControl(): QueryServiceStatus failure, returning FALSE");
        apxLogWrite(APXLOG_MARK_SYSERR);
        return FALSE;
    }
    /* signal that we are about to control the service */
    if (fnControlCallback)
        (*fnControlCallback)(lpCbData, uMsg, (WPARAM)1, (LPARAM)dwState);
    if (dwControl == SERVICE_CONTROL_CONTINUE &&
        stStatus.dwCurrentState != SERVICE_PAUSED)
        bStatus = StartService(lpService->hService, 0, NULL);
    else {
        bStatus = TRUE;
        if (dwControl == SERVICE_CONTROL_STOP) {
            /* First stop dependent services
             */
            bStatus = __apxStopDependentServices(lpService);
        }
        if (bStatus)
            bStatus = ControlService(lpService->hService, dwControl, &stStatus);
    }
    dwStart = GetTickCount();
    dwCheck = stStatus.dwCheckPoint;
    if (bStatus) {
        Sleep(100); /* Initial Sleep period */
        while (QueryServiceStatus(lpService->hService, &stStatus)) {
            if (stStatus.dwCurrentState == dwPendingState) {
                /* Do not wait longer than the wait hint. A good interval is
                 * one tenth the wait hint, but no less than 1 second and no
                 * more than 10 seconds.
                 */
                dwWait = stStatus.dwWaitHint / 10;

                if( dwWait < 1000 )
                    dwWait = 1000;
                else if ( dwWait > 10000 )
                    dwWait = 10000;
                /* Signal to the callback that we are pending
                 * break if callback returns false.
                 */
                if (fnControlCallback) {
                    if (!(*fnControlCallback)(lpCbData, uMsg, (WPARAM)2,
                                              (LPARAM)dwTick++))
                        break;
                }
                Sleep(dwWait);
                if (stStatus.dwCheckPoint > dwCheck) {
                    /* The service is making progress. */
                    dwStart = GetTickCount();
                    dwCheck = stStatus.dwCheckPoint;
                }
                else {
                    if(GetTickCount() - dwStart > stStatus.dwWaitHint) {
                        /* No progress made within the wait hint */
                        break;
                    }
                }
            }
            else
                break;
        }
    }
    /* signal that we are done with controlling the service */
    if (fnControlCallback)
        (*fnControlCallback)(lpCbData, uMsg, (WPARAM)3, (LPARAM)0);
    /* Check if we are in the desired state */
    sleepMillis = 1000;
    apxLogWrite(APXLOG_MARK_DEBUG "apxServiceControl(): Sleeping %d milliseconds", sleepMillis);
    Sleep(sleepMillis);

    if (QueryServiceStatus(lpService->hService, &stStatus)) {
        apxLogWrite(APXLOG_MARK_DEBUG "apxServiceControl(): QueryServiceStatus OK");
        if (fnControlCallback) {
            apxLogWrite(APXLOG_MARK_DEBUG "apxServiceControl(): Calling fnControlCallback()");
            (*fnControlCallback)(lpCbData, uMsg, (WPARAM)4, (LPARAM)&stStatus);
            apxLogWrite(APXLOG_MARK_DEBUG "apxServiceControl(): Returned from fnControlCallback()");
        }
        if (stStatus.dwCurrentState == dwState) {
            return TRUE;
        } else {
            apxLogWrite(APXLOG_MARK_ERROR
                "apxServiceControl(): dwState(%d = %s) != dwCurrentState(%d = %s); "
                "dwWin32ExitCode = %d, dwWaitHint = %d millseconds, dwServiceSpecificExitCode = %d",
                dwState,
                apxServiceGetStateName(dwState),
                stStatus.dwCurrentState,
                apxServiceGetStateName(stStatus.dwCurrentState),
                stStatus.dwWin32ExitCode,
                stStatus.dwWaitHint,
                stStatus.dwServiceSpecificExitCode);
        }

    } else {
        apxLogWrite(APXLOG_MARK_ERROR "apxServiceControl(): QueryServiceStatus failure");
    }

    apxLogWrite(APXLOG_MARK_ERROR "apxServiceControl(): returning FALSE");
    return FALSE;
}

/* Wait one second and check that the service has stopped, returns TRUE if stopped FALSE otherwise */
BOOL
apxServiceCheckStop(APXHANDLE hService)
{
    LPAPXSERVICE   lpService;
    SERVICE_STATUS stStatus;
    DWORD          dwState = SERVICE_STOPPED;
    DWORD          sleepMillis;

    if (hService->dwType != APXHANDLE_TYPE_SERVICE)
        return FALSE;

    lpService = APXHANDLE_DATA(hService);
    /* Manager mode cannot handle services */
    if (lpService->bManagerMode) {
        apxLogWrite(APXLOG_MARK_ERROR "apxServiceCheckStop(): Manager mode cannot handle services, returning FALSE");
        return FALSE;
    }
    /* Check if the ServiceOpen has been called */
    if (IS_INVALID_HANDLE(lpService->hService)) {
        apxLogWrite(APXLOG_MARK_ERROR "apxServiceCheckStop(): Service is not open, returning FALSE");
        return FALSE;
    }

    /* Check if we are in the stopped state */
    sleepMillis = 1000;
    apxLogWrite(APXLOG_MARK_DEBUG "apxServiceCheckStop(): Sleeping %d milliseconds", sleepMillis);
    Sleep(sleepMillis);

    if (QueryServiceStatus(lpService->hService, &stStatus)) {
        apxLogWrite(APXLOG_MARK_DEBUG "apxServiceCheckStop(): QueryServiceStatus OK");
        if (stStatus.dwCurrentState == dwState) {
            return TRUE;
        } else {
            apxLogWrite(APXLOG_MARK_DEBUG
                "apxServiceCheckStop(): dwState(%d) != dwCurrentState(%d); "
                "dwWin32ExitCode = %d, dwWaitHint = %d milliseconds, dwServiceSpecificExitCode = %d",
                dwState,
                stStatus.dwCurrentState,
                stStatus.dwWin32ExitCode,
                stStatus.dwWaitHint,
                stStatus.dwServiceSpecificExitCode);
        }

    } else {
        apxLogWrite(APXLOG_MARK_ERROR "apxServiceCheckStop(): QueryServiceStatus failure");
    }

    apxLogWrite(APXLOG_MARK_DEBUG "apxServiceCheckStop(): returning FALSE");
    return FALSE;
}

BOOL
apxServiceInstall(APXHANDLE hService, LPCWSTR szServiceName,
                  LPCWSTR szDisplayName, LPCWSTR szImagePath,
                  LPCWSTR lpDependencies, DWORD dwServiceType,
                  DWORD dwStartType)
{
    LPAPXSERVICE   lpService;
    LPCWSTR szServiceUser = L"NT Authority\\LocalService";

    if (hService->dwType != APXHANDLE_TYPE_SERVICE)
        return FALSE;

    lpService = APXHANDLE_DATA(hService);
    /* Manager mode cannot handle services */
    if (lpService->bManagerMode)
        return FALSE;
    if (IS_INVALID_HANDLE(lpService->hManager))
        return FALSE;
    if (!__apxIsValidServiceName(szServiceName))
        return FALSE;
    /* Close any previous instance
     * Same handle can install multiple services
     */
    SAFE_CLOSE_SCH(lpService->hService);

    apxFree(lpService->stServiceEntry.lpConfig);
    lpService->stServiceEntry.lpConfig = NULL;
    AplZeroMemory(&lpService->stServiceEntry, sizeof(APXSERVENTRY));

    if (lpDependencies) {
        /* Only add Tcpip and Afd if not already present. */
        BOOL needTcpip = TRUE;
        BOOL needAfd = TRUE;
        LPCWSTR p = lpDependencies;
        while (*p && (needTcpip || needAfd)) {
            if (lstrcmpiW(p, L"Tcpip") == 0) {
                needTcpip = FALSE;
            }
            if (lstrcmpiW(p, L"Afd") == 0) {
                needAfd = FALSE;
            }
            while (*p) {
                p++;
            }
            p++;
        }
        if (needTcpip) {
            lpDependencies = apxMultiSzCombine(NULL, lpDependencies,
                                               L"Tcpip\0", NULL);
        }
        if (needAfd) {
            lpDependencies = apxMultiSzCombine(NULL, lpDependencies,
                                               L"Afd\0", NULL);
        }
    } else {
        lpDependencies = L"Tcpip\0Afd\0";
    }

    if ((dwServiceType & SERVICE_INTERACTIVE_PROCESS) == SERVICE_INTERACTIVE_PROCESS) {
        // Caller is responsible for checking the user is set appropriately
        // for an interactive service (will use LocalSystem)
        szServiceUser = NULL;
    }

    lpService->hService = CreateServiceW(lpService->hManager,
                                         szServiceName,
                                         szDisplayName,
                                         SERVICE_ALL_ACCESS,
                                         dwServiceType,
                                         dwStartType,
                                         SERVICE_ERROR_NORMAL,
                                         szImagePath,
                                         NULL,
                                         NULL,
                                         lpDependencies,
                                         szServiceUser,
                                         NULL);

    if (IS_INVALID_HANDLE(lpService->hService)) {
        apxLogWrite(APXLOG_MARK_SYSERR);
        SetLastError(0);
        return FALSE;
    }
    else {
        lstrlcpyW(lpService->stServiceEntry.szServiceName,
                  SIZ_RESLEN, szServiceName);
        return TRUE;
    }
}

BOOL
apxServiceDelete(APXHANDLE hService)
{
    LPAPXSERVICE   lpService;
    SERVICE_STATUS stStatus;
    if (hService->dwType != APXHANDLE_TYPE_SERVICE)
        return FALSE;

    lpService = APXHANDLE_DATA(hService);
    /* Manager mode cannot handle services */
    if (lpService->bManagerMode)
        return FALSE;
    /* Check if the ServiceOpen has been called */
    if (IS_INVALID_HANDLE(lpService->hService))
        return FALSE;
    if (QueryServiceStatus(lpService->hService, &stStatus)) {
        BOOL rv;
        if (stStatus.dwCurrentState != SERVICE_STOPPED)
            apxServiceControl(hService, SERVICE_CONTROL_STOP, 0, NULL, NULL);
        rv = DeleteService(lpService->hService);
        SAFE_CLOSE_SCH(lpService->hService);
        SAFE_CLOSE_SCH(lpService->hManager);

        return rv;
    }
    return FALSE;
}
