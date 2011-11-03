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
        apxLogWrite(APXLOG_MARK_WARN "Failed to obtain service description");
        lpService->stServiceEntry.szServiceDescription[0] = L'\0';
    }
    if (!apxGetServiceUserW(szServiceName,
                            lpService->stServiceEntry.szObjectName,
                            SIZ_RESLEN)) {
        apxLogWrite(APXLOG_MARK_WARN "Failed to obtain service user name");
        lpService->stServiceEntry.szObjectName[0] = L'\0';
    }
    if (!QueryServiceConfigW(lpService->hService, NULL, 0, &dwNeeded)) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            apxLogWrite(APXLOG_MARK_SYSERR);
    }
    /* TODO: Check GetLastError  ERROR_INSUFFICIENT_BUFFER */
    lpService->stServiceEntry.lpConfig =  (LPQUERY_SERVICE_CONFIGW)apxPoolAlloc(hService->hPool,
                                                                                dwNeeded);
    return QueryServiceConfigW(lpService->hService,
                               lpService->stServiceEntry.lpConfig,
                               dwNeeded, &dwNeeded);
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
    if (szUsername || szPassword)
        dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    if (!ChangeServiceConfigW(lpService->hService,
                              dwServiceType,
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
                     DWORD dwServiceType,
                     DWORD dwStartType,
                     DWORD dwErrorControl)
{
    LPAPXSERVICE lpService;

    if (hService->dwType != APXHANDLE_TYPE_SERVICE)
        return FALSE;

    lpService = APXHANDLE_DATA(hService);
    /* Manager mode cannot handle services */
    if (lpService->bManagerMode)
        return FALSE;
    /* Check if the ServixeOpen has been called */
    if (IS_INVALID_HANDLE(lpService->hService))
        return FALSE;
    return ChangeServiceConfig(lpService->hService, dwServiceType,
                               dwStartType, dwErrorControl,
                               NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL);
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
    DWORD          dwPending = 0;
    DWORD          dwState = 0;
    DWORD          dwTick  = 0;
    DWORD          dwWait, dwCheck, dwStart;
    BOOL           bStatus;

    if (hService->dwType != APXHANDLE_TYPE_SERVICE)
        return FALSE;

    lpService = APXHANDLE_DATA(hService);
    /* Manager mode cannot handle services */
    if (lpService->bManagerMode)
        return FALSE;
    /* Check if the ServiceOpen has been called */
    if (IS_INVALID_HANDLE(lpService->hService))
        return FALSE;
    switch (dwControl) {
        case SERVICE_CONTROL_CONTINUE:
            dwPending = SERVICE_START_PENDING;
            dwState   = SERVICE_RUNNING;
            break;
        case SERVICE_CONTROL_STOP:
            dwPending = SERVICE_STOP_PENDING;
            dwState   = SERVICE_STOPPED;
            break;
        case SERVICE_CONTROL_PAUSE:
            dwPending = SERVICE_PAUSE_PENDING;
            dwState   = SERVICE_PAUSED;
            break;
        default:
            break;
    }
    /* user defined controls */
    if (dwControl > 127 && dwControl < 224) {
        /* 128 ... 159  start signals
         * 160 ... 191  stop signals
         * 192 ... 223  pause signals
         */
        switch (dwControl & 0xE0) {
            case 0x80:
            case 0x90:
                dwPending = SERVICE_START_PENDING;
                dwState   = SERVICE_RUNNING;
                break;
            case 0xA0:
            case 0xB0:
                dwPending = SERVICE_STOP_PENDING;
                dwState   = SERVICE_STOPPED;
                break;
            case 0xC0:
            case 0xD0:
                dwPending = SERVICE_PAUSE_PENDING;
                dwState   = SERVICE_PAUSED;
                break;
            default:
                break;
        }
    }
    if (!dwPending && !dwState)
        return FALSE;
    /* Now lets control */
    if (!QueryServiceStatus(lpService->hService, &stStatus)) {
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
            if (stStatus.dwCurrentState == dwPending) {
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
    /* signal that we are done with controling the service */
    if (fnControlCallback)
        (*fnControlCallback)(lpCbData, uMsg, (WPARAM)3, (LPARAM)0);
    /* Check if we are in the desired state */
    Sleep(1000);

    if (QueryServiceStatus(lpService->hService, &stStatus)) {
        if (fnControlCallback)
            (*fnControlCallback)(lpCbData, uMsg, (WPARAM)4,
                                 (LPARAM)&stStatus);
        if (stStatus.dwCurrentState == dwState)
            return TRUE;

    }

    return FALSE;
}

BOOL
apxServiceInstall(APXHANDLE hService, LPCWSTR szServiceName,
                  LPCWSTR szDisplayName, LPCWSTR szImagePath,
                  LPCWSTR lpDependencies, DWORD dwServiceType,
                  DWORD dwStartType)
{
    LPAPXSERVICE   lpService;

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

    if (lpDependencies)
        lpDependencies = apxMultiSzCombine(NULL, lpDependencies,
                                           L"Tcpip\0Afd\0", NULL);
    else
        lpDependencies = L"Tcpip\0Afd\0";
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
                                         NULL,
                                         NULL);

    if (IS_INVALID_HANDLE(lpService->hService)) {
        apxLogWrite(APXLOG_MARK_SYSERR);
        return FALSE;
    }
    else {
        lstrlcpyW(lpService->stServiceEntry.szServiceName,
                  SIZ_RESLEN, szServiceName);
        lpService->stServiceEntry.dwStart = dwStartType;
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

/* Browse the services */
DWORD
apxServiceBrowse(APXHANDLE hService,
                 LPCWSTR szIncludeNamePattern,
                 LPCWSTR szIncludeImagePattern,
                 LPCWSTR szExcludeNamePattern,
                 LPCWSTR szExcludeImagePattern,
                 UINT uMsg,
                 LPAPXFNCALLBACK fnDisplayCallback,
                 LPVOID lpCbData)
{
    DWORD        nFound = 0;
    APXREGENUM   stEnum;
    LPAPXSERVICE lpService;
    SC_LOCK      hLock;
    if (hService->dwType != APXHANDLE_TYPE_SERVICE || !fnDisplayCallback)
        return 0;

    lpService = APXHANDLE_DATA(hService);
    /* Only the manager mode can browse services */
    if (!lpService->bManagerMode ||
        IS_INVALID_HANDLE(lpService->hManager))
        return 0;
    hLock = LockServiceDatabase(lpService->hManager);
    if (IS_INVALID_HANDLE(hLock)) {
        apxLogWrite(APXLOG_MARK_SYSERR);

        return 0;
    }
    AplZeroMemory(&stEnum, sizeof(APXREGENUM));

    while (TRUE) {
        APXSERVENTRY stEntry;
        BOOL rv;
        AplZeroMemory(&stEntry, sizeof(APXSERVENTRY));
        rv = apxRegistryEnumServices(&stEnum, &stEntry);

        if (rv) {
            INT fm = -1;
            SC_HANDLE hSrv = NULL;
            DWORD dwNeeded = 0;
            hSrv = OpenServiceW(lpService->hManager,
                                stEntry.szServiceName,
                                GENERIC_READ);
            if (!IS_INVALID_HANDLE(hSrv)) {
                QueryServiceConfigW(hSrv, NULL, 0, &dwNeeded);
                stEntry.lpConfig = (LPQUERY_SERVICE_CONFIGW)apxPoolAlloc(hService->hPool,
                                                                         dwNeeded);
                /* Call the QueryServiceConfig againg with allocated config */
                if (QueryServiceConfigW(hSrv, stEntry.lpConfig, dwNeeded, &dwNeeded)) {
                    /* Make that customizable so that kernel mode drivers can be
                     * displayed and maintained. For now skip the
                     * filesystem and device drivers.
                     * XXX: Do we need that customizable after all?
                     */
                    if ((stEntry.lpConfig->dwServiceType &
                         ~SERVICE_INTERACTIVE_PROCESS) & SERVICE_WIN32)
                        fm = 0;

                    if (!fm && szIncludeNamePattern) {
                        fm = apxMultiStrMatchW(stEntry.szServiceName,
                                               szIncludeNamePattern, L';', TRUE);
                    }
                    if (!fm && szExcludeNamePattern) {
                        fm = !apxMultiStrMatchW(stEntry.szServiceName,
                                                szExcludeNamePattern, L';', TRUE);
                    }
                    if (!fm && szIncludeImagePattern) {
                        fm = apxMultiStrMatchW(stEntry.lpConfig->lpBinaryPathName,
                                               szIncludeImagePattern, L';', TRUE);
                    }
                    if (!fm && szExcludeImagePattern) {
                        fm = !apxMultiStrMatchW(stEntry.szServiceName,
                                                szExcludeImagePattern, L';', TRUE);
                    }
                    if (!fm) {
                        QueryServiceStatus(hSrv, &(stEntry.stServiceStatus));
                        /* WIN2K + extended service info */
                        if (_st_apx_oslevel >= 4) {
                            DWORD dwNeed;
                            QueryServiceStatusEx(hSrv, SC_STATUS_PROCESS_INFO,
                                                 (LPBYTE)(&(stEntry.stStatusProcess)),
                                                 sizeof(SERVICE_STATUS_PROCESS),
                                                 &dwNeed);
                        }
                        /* finaly call the provided callback */
                        rv = (*fnDisplayCallback)(lpCbData, uMsg,
                                                  (WPARAM)&stEntry,
                                                  (LPARAM)nFound++);
                    }
                }
                /* release the skipped service config */
                if (fm) {
                    apxFree(stEntry.lpConfig);
                }
            }
            SAFE_CLOSE_SCH(hSrv);
        }
        if (!rv)
            break;
    }

    UnlockServiceDatabase(hLock);
    return nFound;
}
