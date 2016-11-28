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

/* ====================================================================
 * prunmgr -- Service Manager Application.
 * Contributed by Mladen Turk <mturk@apache.org>
 * 05 Aug 2003
 * ====================================================================
 */

/* Force the JNI vprintf functions */
#define _DEBUG_JNI  1
#include "apxwin.h"
#include "prunmgr.h"

LPAPXGUISTORE _gui_store  = NULL;
#define PRUNMGR_CLASS      TEXT("PRUNMGR")
#define TMNU_CONF          TEXT("Configure...")
#define TMNU_START         TEXT("Start service")
#define TMNU_STOP          TEXT("Stop service")
#define TMNU_EXIT          TEXT("Exit")
#define TMNU_ABOUT         TEXT("About")
#define TMNU_DUMP          TEXT("Thread Dump")

/* Display only Started/Paused status */
#define STAT_STARTED        TEXT("Started")
#define STAT_PAUSED         TEXT("Paused")
#define STAT_STOPPED        TEXT("Stopped")
#define STAT_DISABLED       TEXT("Disabled")
#define STAT_NONE           TEXT("")
#define STAT_SYSTEM         L"LocalSystem"

#define LOGL_ERROR          L"Error"
#define LOGL_DEBUG          L"Debug"
#define LOGL_INFO           L"Info"
#define LOGL_WARN           L"Warning"


#define START_AUTO           L"Automatic"
#define START_MANUAL         L"Manual"
#define START_DISABLED       L"Disabled"
#define START_BOOT           L"Boot"
#define START_SYSTEM         L"SystemInit"
#define EMPTY_PASSWORD       L"               "

#ifdef WIN64
#define KREG_WOW6432  KEY_WOW64_32KEY
#else
#define KREG_WOW6432  0
#endif

/* Main application pool */
APXHANDLE hPool     = NULL;
APXHANDLE hService  = NULL;
APXHANDLE hRegistry = NULL;
APXHANDLE hRegserv  = NULL;
HICON     hIcoRun   = NULL;
HICON     hIcoStop  = NULL;

LPAPXSERVENTRY _currentEntry = NULL;

BOOL      bEnableTry = FALSE;
DWORD     startPage  = 0;

static LPCWSTR  _s_log          = L"Log";
static LPCWSTR  _s_java         = L"Java";
static LPCWSTR  _s_start        = L"Start";
static LPCWSTR  _s_stop         = L"Stop";

/* Allowed prunmgr commands */
static LPCWSTR _commands[] = {
    L"ES",      /* 1 Manage Service (default)*/
    L"MS",      /* 2 Monitor Service */
    L"MR",      /* 3 Monitor Service and start if not runing */
    L"MQ",      /* 4 Quit all running Monitor applications */
    NULL
};

static LPCWSTR _altcmds[] = {
    L"manage",      /* 1 Manage Service (default)*/
    L"monitor",     /* 2 Monitor Service */
    L"start",       /* 3 Monitor Service and start if not runing */
    L"quit",        /* 4 Quit all running Monitor applications */
    NULL
};


/* Allowed procrun parameters */
static APXCMDLINEOPT _options[] = {
/* 0  */    { L"AllowMultiInstances", NULL, NULL,   APXCMDOPT_INT, NULL, 0},
            /* NULL terminate the array */
            { NULL }
};

/* Create RBUTTON try menu
 * Configure... (default, or lbutton dblclick)
 * Start <service name>
 * Stop  <service name>
 * Exit
 * Logo
 */
static void createRbuttonTryMenu(HWND hWnd)
{
    HMENU hMnu;
    POINT pt;
    BOOL canStop  = FALSE;
    BOOL canStart = FALSE;
    hMnu = CreatePopupMenu();

    if (_currentEntry) {
        if (_currentEntry->stServiceStatus.dwCurrentState == SERVICE_RUNNING) {
            if (_currentEntry->stServiceStatus.dwControlsAccepted & SERVICE_ACCEPT_STOP)
                canStop = TRUE;
        }
        else if (_currentEntry->stServiceStatus.dwCurrentState == SERVICE_STOPPED) {
            if (_currentEntry->lpConfig->dwStartType != SERVICE_DISABLED)
                canStart = TRUE;
        }
    }
    apxAppendMenuItem(hMnu, IDM_TM_CONFIG, TMNU_CONF,  TRUE, TRUE);
    apxAppendMenuItem(hMnu, IDM_TM_START,  TMNU_START, FALSE, canStart);
    apxAppendMenuItem(hMnu, IDM_TM_STOP,   TMNU_STOP,  FALSE, canStop);
    apxAppendMenuItem(hMnu, IDM_TM_DUMP,   TMNU_DUMP,  FALSE, canStop);
    apxAppendMenuItem(hMnu, IDM_TM_EXIT,   TMNU_EXIT,  FALSE, TRUE);
    apxAppendMenuItem(hMnu,    -1, NULL,   FALSE, FALSE);
    apxAppendMenuItem(hMnu, IDM_TM_ABOUT,  TMNU_ABOUT, FALSE, TRUE);

    /* Ensure we have a focus */
    if (!SetForegroundWindow(hWnd))
        SetForegroundWindow(NULL);
    GetCursorPos(&pt);
    /* Display the try menu */
    TrackPopupMenu(hMnu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                   pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMnu);
}

/* wParam progress dialog handle
 */
static BOOL __startServiceCallback(APXHANDLE hObject, UINT uMsg,
                                   WPARAM wParam, LPARAM lParam)
{
    HWND hDlg = (HWND)hObject;

    switch (uMsg) {
        case WM_USER+1:
            hDlg = (HWND)lParam;
            if (IS_INVALID_HANDLE(hService)) {
                EndDialog(hDlg, IDOK);
                PostMessage(_gui_store->hMainWnd, WM_COMMAND,
                            MAKEWPARAM(IDMS_REFRESH, 0), 0);
                return FALSE;
            }
            if (apxServiceControl(hService, SERVICE_CONTROL_CONTINUE, WM_USER+2,
                                  __startServiceCallback, hDlg)) {
                _currentEntry->stServiceStatus.dwCurrentState = SERVICE_RUNNING;
                _currentEntry->stStatusProcess.dwCurrentState = SERVICE_RUNNING;

            }
            EndDialog(hDlg, IDOK);
            PostMessage(_gui_store->hMainWnd, WM_COMMAND,
                        MAKEWPARAM(IDMS_REFRESH, 0), 0);
        break;
        case WM_USER+2:
            SendMessage(hDlg, WM_USER+1, 0, 0);
            Sleep(500);
            break;
    }
    return TRUE;
}

static BOOL __stopServiceCallback(APXHANDLE hObject, UINT uMsg,
                                   WPARAM wParam, LPARAM lParam)
{
    HWND hDlg = (HWND)hObject;

    switch (uMsg) {
        case WM_USER+1:
            hDlg = (HWND)lParam;
            if (IS_INVALID_HANDLE(hService))
                return FALSE;
            if (apxServiceControl(hService, SERVICE_CONTROL_STOP, WM_USER+2,
                                  __stopServiceCallback, hDlg)) {
            }
            EndDialog(hDlg, IDOK);
            PostMessage(_gui_store->hMainWnd, WM_COMMAND,
                        MAKEWPARAM(IDMS_REFRESH, 0), 0);
        break;
        case WM_USER+2:
            if (wParam == 4)
                AplCopyMemory(&_currentEntry->stServiceStatus,
                              (LPVOID)lParam, sizeof(SERVICE_STATUS));
            SendMessage(hDlg, WM_USER+1, 0, 0);
            Sleep(100);
            break;
    }
    return TRUE;
}

static BOOL __restartServiceCallback(APXHANDLE hObject, UINT uMsg,
                                     WPARAM wParam, LPARAM lParam)
{
    HWND hDlg = (HWND)hObject;
    switch (uMsg) {
        case WM_USER+1:
            hDlg = (HWND)lParam;
            if (IS_INVALID_HANDLE(hService))
                return FALSE;
            /* TODO: use 128 as controll code */
            if (apxServiceControl(hService, 128, WM_USER+2,
                                  __restartServiceCallback, hDlg)) {

            }
            EndDialog(hDlg, IDOK);
            PostMessage(_gui_store->hMainWnd, WM_COMMAND,
                        MAKEWPARAM(IDMS_REFRESH, 0), 0);
        break;
        case WM_USER+2:
            if (wParam == 4)
                AplCopyMemory(&_currentEntry->stServiceStatus,
                              (LPVOID)lParam, sizeof(SERVICE_STATUS));

            SendMessage(hDlg, WM_USER+1, 0, 0);
            Sleep(100);
            break;
    }
    return TRUE;
}

static BOOL __pauseServiceCallback(APXHANDLE hObject, UINT uMsg,
                                   WPARAM wParam, LPARAM lParam)
{
    HWND hDlg = (HWND)hObject;
    switch (uMsg) {
        case WM_USER+1:
            hDlg = (HWND)lParam;
            if (IS_INVALID_HANDLE(hService))
                return FALSE;
            if (apxServiceControl(hService, SERVICE_CONTROL_PAUSE, WM_USER+2,
                                  __pauseServiceCallback, hDlg)) {
            }
            EndDialog(hDlg, IDOK);
            PostMessage(_gui_store->hMainWnd, WM_COMMAND,
                        MAKEWPARAM(IDMS_REFRESH, 0), 0);
        break;
        case WM_USER+2:
            if (wParam == 4)
                AplCopyMemory(&_currentEntry->stServiceStatus,
                             (LPVOID)lParam, sizeof(SERVICE_STATUS));
            SendMessage(hDlg, WM_USER+1, 0, 0);
            Sleep(100);
            break;
    }
    return TRUE;
}

static DWORD  _propertyChanged;
static BOOL   _propertyOpened = FALSE;
static HWND   _propertyHwnd = NULL;
/* Service property pages */
int CALLBACK __propertyCallback(HWND hwndPropSheet, UINT uMsg, LPARAM lParam)
{
    switch(uMsg) {
        case PSCB_PRECREATE:
           {
                LPDLGTEMPLATE  lpTemplate = (LPDLGTEMPLATE)lParam;
                if (!(lpTemplate->style & WS_SYSMENU))
                    lpTemplate->style |= WS_SYSMENU;
                _propertyHwnd = hwndPropSheet;

                _propertyChanged = 0;
                _propertyOpened = TRUE;
                return TRUE;
            }
        break;
        case PSCB_INITIALIZED:
        break;
    }
    return TRUE;
}

BOOL __generalPropertySave(HWND hDlg)
{
    WCHAR szN[SIZ_RESLEN];
    WCHAR szD[SIZ_DESLEN];
    DWORD dwStartType = SERVICE_NO_CHANGE;
    int i;

    if (!(TST_BIT_FLAG(_propertyChanged, 1)))
        return TRUE;
    CLR_BIT_FLAG(_propertyChanged, 1);

    if (IS_INVALID_HANDLE(hService))
        return FALSE;
    GetDlgItemTextW(hDlg, IDC_PPSGDISP, szN, SIZ_RESMAX);
    GetDlgItemTextW(hDlg, IDC_PPSGDESC, szD, SIZ_DESMAX);
    i = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PPSGCMBST));
    if (i == 0)
        dwStartType = SERVICE_AUTO_START;
    else if (i == 1)
        dwStartType = SERVICE_DEMAND_START;
    else if (i == 2)
        dwStartType = SERVICE_DISABLED;
    apxServiceSetNames(hService, NULL, szN, szD, NULL, NULL);
    apxServiceSetOptions(hService, SERVICE_NO_CHANGE, dwStartType, SERVICE_NO_CHANGE);

    if (!(TST_BIT_FLAG(_propertyChanged, 2)))
        PostMessage(_gui_store->hMainWnd, WM_COMMAND, MAKEWPARAM(IDMS_REFRESH, 0), 0);

    return TRUE;
}

BOOL __generalLogonSave(HWND hDlg)
{
    WCHAR szU[SIZ_RESLEN];
    WCHAR szP[SIZ_RESLEN];
    WCHAR szC[SIZ_RESLEN];
    DWORD dwStartType = SERVICE_NO_CHANGE;

    if (!(TST_BIT_FLAG(_propertyChanged, 2)))
        return TRUE;
    CLR_BIT_FLAG(_propertyChanged, 2);

    if (IS_INVALID_HANDLE(hService))
        return FALSE;
    GetDlgItemTextW(hDlg, IDC_PPSLUSER,  szU, SIZ_RESMAX);
    GetDlgItemTextW(hDlg, IDC_PPSLPASS,  szP, SIZ_RESMAX);
    GetDlgItemTextW(hDlg, IDC_PPSLCPASS, szC, SIZ_RESMAX);

    if (lstrlenW(szU) && lstrcmpiW(szU, STAT_SYSTEM)) {
        if (szP[0] != L' ' &&  szC[0] != L' ' && !lstrcmpW(szP, szC)) {
            apxServiceSetNames(hService, NULL, NULL, NULL, szU, szP);
            lstrlcpyW(_currentEntry->szObjectName, SIZ_RESLEN, szU);
        }
        else {
            MessageBoxW(hDlg, apxLoadResourceW(IDS_VALIDPASS, 0),
                        apxLoadResourceW(IDS_APPLICATION, 1),
                        MB_OK | MB_ICONSTOP);
            return FALSE;
        }
    }
    else {
        apxServiceSetNames(hService, NULL, NULL, NULL, STAT_SYSTEM, L"");
        lstrlcpyW(_currentEntry->szObjectName, SIZ_RESLEN, STAT_SYSTEM);
        if (IsDlgButtonChecked(hDlg, IDC_PPSLID) == BST_CHECKED)
            apxServiceSetOptions(hService,
                _currentEntry->stServiceStatus.dwServiceType | SERVICE_INTERACTIVE_PROCESS,
                SERVICE_NO_CHANGE, SERVICE_NO_CHANGE);
        else
            apxServiceSetOptions(hService,
                _currentEntry->stServiceStatus.dwServiceType & ~SERVICE_INTERACTIVE_PROCESS,
                SERVICE_NO_CHANGE, SERVICE_NO_CHANGE);
    }
    if (!(TST_BIT_FLAG(_propertyChanged, 1)))
        PostMessage(_gui_store->hMainWnd, WM_COMMAND, MAKEWPARAM(IDMS_REFRESH, 0), 0);
    return TRUE;
}

BOOL __generalLoggingSave(HWND hDlg)
{
    WCHAR szB[SIZ_DESLEN];

    if (!(TST_BIT_FLAG(_propertyChanged, 3)))
        return TRUE;
    CLR_BIT_FLAG(_propertyChanged, 3);

    if (IS_INVALID_HANDLE(hService))
        return FALSE;

    GetDlgItemTextW(hDlg, IDC_PPLGLEVEL,  szB, SIZ_DESMAX);
    apxRegistrySetStrW(hRegserv, APXREG_PARAMSOFTWARE, _s_log, L"Level", szB);
    GetDlgItemTextW(hDlg, IDC_PPLGPATH,  szB, SIZ_DESMAX);
    apxRegistrySetStrW(hRegserv, APXREG_PARAMSOFTWARE, _s_log, L"Path", szB);
    GetDlgItemTextW(hDlg, IDC_PPLGPREFIX,  szB, SIZ_DESMAX);
    apxRegistrySetStrW(hRegserv, APXREG_PARAMSOFTWARE, _s_log, L"Prefix", szB);
    GetDlgItemTextW(hDlg, IDC_PPLGPIDFILE,  szB, SIZ_DESMAX);
    apxRegistrySetStrW(hRegserv, APXREG_PARAMSOFTWARE, _s_log, L"PidFile", szB);
    GetDlgItemTextW(hDlg, IDC_PPLGSTDOUT,  szB, SIZ_DESMAX);
    apxRegistrySetStrW(hRegserv, APXREG_PARAMSOFTWARE, _s_log, L"StdOutput", szB);
    GetDlgItemTextW(hDlg, IDC_PPLGSTDERR,  szB, SIZ_DESMAX);
    apxRegistrySetStrW(hRegserv, APXREG_PARAMSOFTWARE, _s_log, L"StdError", szB);

    if (!(TST_BIT_FLAG(_propertyChanged, 1)))
        PostMessage(_gui_store->hMainWnd, WM_COMMAND, MAKEWPARAM(IDMS_REFRESH, 0), 0);
    return TRUE;
}

BOOL __generalJvmSave(HWND hDlg)
{
    WCHAR szB[SIZ_HUGLEN];
    LPWSTR p, s;
    DWORD  l;
    if (!(TST_BIT_FLAG(_propertyChanged, 4)))
        return TRUE;
    CLR_BIT_FLAG(_propertyChanged, 4);

    if (IS_INVALID_HANDLE(hService))
        return FALSE;
    if (!IsDlgButtonChecked(hDlg, IDC_PPJAUTO)) {
        GetDlgItemTextW(hDlg, IDC_PPJJVM,  szB, SIZ_HUGMAX);
    }
    else
        lstrcpyW(szB, L"auto");
    apxRegistrySetStrW(hRegserv, APXREG_PARAMSOFTWARE, _s_java, L"Jvm", szB);
    GetDlgItemTextW(hDlg, IDC_PPJCLASSPATH,  szB, SIZ_HUGMAX);
    apxRegistrySetStrW(hRegserv, APXREG_PARAMSOFTWARE, _s_java, L"Classpath", szB);

    l = GetWindowTextLength(GetDlgItem(hDlg, IDC_PPJOPTIONS));
    p = apxPoolAlloc(hPool, (l + 2) * sizeof(WCHAR));
    GetDlgItemTextW(hDlg, IDC_PPJOPTIONS,  p, l + 1);
    s = apxCRLFToMszW(hPool, p, &l);
    apxFree(p);
    apxRegistrySetMzStrW(hRegserv, APXREG_PARAMSOFTWARE,
                         _s_java, L"Options", s, l);
    if (!GetDlgItemTextW(hDlg, IDC_PPJMS,  szB, SIZ_HUGMAX))
        szB[0] = L'\0';

    apxRegistrySetNumW(hRegserv, APXREG_PARAMSOFTWARE, _s_java, L"JvmMs",
                       apxAtoulW(szB));
    if (!GetDlgItemTextW(hDlg, IDC_PPJMX,  szB, SIZ_DESMAX))
        szB[0] = L'\0';
    apxRegistrySetNumW(hRegserv, APXREG_PARAMSOFTWARE, _s_java, L"JvmMx",
                       apxAtoulW(szB));
    if (!GetDlgItemTextW(hDlg, IDC_PPJSS,  szB, SIZ_DESMAX))
        szB[0] = L'\0';
    apxRegistrySetNumW(hRegserv, APXREG_PARAMSOFTWARE, _s_java, L"JvmSs",
                       apxAtoulW(szB));
    apxFree(s);
    if (!(TST_BIT_FLAG(_propertyChanged, 1)))
        PostMessage(_gui_store->hMainWnd, WM_COMMAND, MAKEWPARAM(IDMS_REFRESH, 0), 0);
    return TRUE;
}

BOOL __generalStartSave(HWND hDlg)
{
    WCHAR szB[SIZ_HUGLEN];
    LPWSTR p, s;
    DWORD  l;

    if (!(TST_BIT_FLAG(_propertyChanged, 5)))
        return TRUE;
    CLR_BIT_FLAG(_propertyChanged, 5);

    if (IS_INVALID_HANDLE(hService))
        return FALSE;

    GetDlgItemTextW(hDlg, IDC_PPRCLASS,  szB, SIZ_HUGMAX);
    apxRegistrySetStrW(hRegserv, APXREG_PARAMSOFTWARE, _s_start, L"Class", szB);
    GetDlgItemTextW(hDlg, IDC_PPRIMAGE,  szB, SIZ_HUGMAX);
    apxRegistrySetStrW(hRegserv, APXREG_PARAMSOFTWARE, _s_start, L"Image", szB);
    GetDlgItemTextW(hDlg, IDC_PPRWPATH,  szB, SIZ_HUGMAX);
    apxRegistrySetStrW(hRegserv, APXREG_PARAMSOFTWARE, _s_start, L"WorkingPath", szB);
    GetDlgItemTextW(hDlg, IDC_PPRMETHOD,  szB, SIZ_HUGMAX);
    apxRegistrySetStrW(hRegserv, APXREG_PARAMSOFTWARE, _s_start, L"Method", szB);
    GetDlgItemTextW(hDlg, IDC_PPRMODE,  szB, SIZ_HUGMAX);
    apxRegistrySetStrW(hRegserv, APXREG_PARAMSOFTWARE, _s_start, L"Mode", szB);

    l = GetWindowTextLength(GetDlgItem(hDlg, IDC_PPRARGS));
    p = apxPoolAlloc(hPool, (l + 2) * sizeof(WCHAR));
    GetDlgItemTextW(hDlg, IDC_PPRARGS,  p, l + 1);
    s = apxCRLFToMszW(hPool, p, &l);
    apxFree(p);
    apxRegistrySetMzStrW(hRegserv, APXREG_PARAMSOFTWARE,
                         _s_start, L"Params", s, l);
    apxFree(s);

    if (!(TST_BIT_FLAG(_propertyChanged, 1)))
        PostMessage(_gui_store->hMainWnd, WM_COMMAND, MAKEWPARAM(IDMS_REFRESH, 0), 0);
    return TRUE;
}

BOOL __generalStopSave(HWND hDlg)
{
    WCHAR szB[SIZ_HUGLEN];
    LPWSTR p, s;
    DWORD  l;

    if (!(TST_BIT_FLAG(_propertyChanged, 6)))
        return TRUE;
    CLR_BIT_FLAG(_propertyChanged, 6);

    if (IS_INVALID_HANDLE(hService))
        return FALSE;

    GetDlgItemTextW(hDlg, IDC_PPSCLASS,  szB, SIZ_HUGMAX);
    apxRegistrySetStrW(hRegserv, APXREG_PARAMSOFTWARE, _s_stop, L"Class", szB);
    GetDlgItemTextW(hDlg, IDC_PPSIMAGE,  szB, SIZ_HUGMAX);
    apxRegistrySetStrW(hRegserv, APXREG_PARAMSOFTWARE, _s_stop, L"Image", szB);
    GetDlgItemTextW(hDlg, IDC_PPSWPATH,  szB, SIZ_HUGMAX);
    apxRegistrySetStrW(hRegserv, APXREG_PARAMSOFTWARE, _s_stop, L"WorkingPath", szB);
    GetDlgItemTextW(hDlg, IDC_PPSMETHOD,  szB, SIZ_HUGMAX);
    apxRegistrySetStrW(hRegserv, APXREG_PARAMSOFTWARE, _s_stop, L"Method", szB);
    GetDlgItemTextW(hDlg, IDC_PPSTIMEOUT,  szB, SIZ_HUGMAX);
    apxRegistrySetNumW(hRegserv, APXREG_PARAMSOFTWARE, _s_stop, L"Timeout", apxAtoulW(szB));
    GetDlgItemTextW(hDlg, IDC_PPSMODE,  szB, SIZ_HUGMAX);
    apxRegistrySetStrW(hRegserv, APXREG_PARAMSOFTWARE, _s_stop, L"Mode", szB);

    l = GetWindowTextLength(GetDlgItem(hDlg, IDC_PPSARGS));
    p = apxPoolAlloc(hPool, (l + 2) * sizeof(WCHAR));
    GetDlgItemTextW(hDlg, IDC_PPSARGS,  p, l + 1);
    s = apxCRLFToMszW(hPool, p, &l);
    apxFree(p);
    apxRegistrySetMzStrW(hRegserv, APXREG_PARAMSOFTWARE,
                         _s_stop, L"Params", s, l);
    apxFree(s);

    if (!(TST_BIT_FLAG(_propertyChanged, 1)))
        PostMessage(_gui_store->hMainWnd, WM_COMMAND, MAKEWPARAM(IDMS_REFRESH, 0), 0);
    return TRUE;
}

void __generalPropertyRefresh(HWND hDlg)
{
    Button_Enable(GetDlgItem(hDlg, IDC_PPSGSTART), FALSE);
    Button_Enable(GetDlgItem(hDlg, IDC_PPSGSTOP), FALSE);
    Button_Enable(GetDlgItem(hDlg, IDC_PPSGPAUSE), FALSE);
    Button_Enable(GetDlgItem(hDlg, IDC_PPSGRESTART), FALSE);
    switch (_currentEntry->stServiceStatus.dwCurrentState) {
        case SERVICE_RUNNING:
            if (_currentEntry->stServiceStatus.dwControlsAccepted & SERVICE_ACCEPT_STOP ||
                _currentEntry->lpConfig->dwStartType != SERVICE_DISABLED) {
                Button_Enable(GetDlgItem(hDlg, IDC_PPSGSTOP), TRUE);
                SetDlgItemText(hDlg, IDC_PPSGSTATUS, STAT_STARTED);
            }
            else
                SetDlgItemText(hDlg, IDC_PPSGSTATUS, STAT_DISABLED);
            if (_currentEntry->stServiceStatus.dwControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE) {
                Button_Enable(GetDlgItem(hDlg, IDC_PPSGPAUSE), TRUE);
                Button_Enable(GetDlgItem(hDlg, IDC_PPSGRESTART), TRUE);
            }
        break;
        case SERVICE_PAUSED:
            Button_Enable(GetDlgItem(hDlg, IDC_PPSGSTART), TRUE);
            Button_Enable(GetDlgItem(hDlg, IDC_PPSGRESTART), TRUE);
            SetDlgItemText(hDlg, IDC_PPSGSTATUS, STAT_PAUSED);
        break;
        case SERVICE_STOPPED:
            if (_currentEntry->lpConfig->dwStartType != SERVICE_DISABLED) {
                Button_Enable(GetDlgItem(hDlg, IDC_PPSGSTART), TRUE);
                SetDlgItemText(hDlg, IDC_PPSGSTATUS, STAT_STOPPED);
            }
            else
                SetDlgItemText(hDlg, IDC_PPSGSTATUS, STAT_DISABLED);
        break;
        default:
        break;
    }
}

static BOOL bpropCentered = FALSE;
static HWND _generalPropertyHwnd = NULL;

LRESULT CALLBACK __generalProperty(HWND hDlg,
                                   UINT uMessage,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
    LPPSHNOTIFY lpShn;
    WCHAR       szBuf[SIZ_DESLEN];

    switch (uMessage) {
        case WM_INITDIALOG:
            {
                _generalPropertyHwnd = hDlg;
                if (!bEnableTry)
                    apxCenterWindow(GetParent(hDlg), NULL);
                else if (!bpropCentered)
                    apxCenterWindow(GetParent(hDlg), NULL);
                bpropCentered = TRUE;
                startPage = 0;
                if (!bEnableTry)
                    apxCenterWindow(GetParent(hDlg), NULL);
                SendMessage(GetDlgItem(hDlg, IDC_PPSGDISP), EM_LIMITTEXT, SIZ_RESMAX, 0);
                SendMessage(GetDlgItem(hDlg, IDC_PPSGDESC), EM_LIMITTEXT, SIZ_DESMAX, 0);

                ComboBox_AddStringW(GetDlgItem(hDlg, IDC_PPSGCMBST), START_AUTO);
                ComboBox_AddStringW(GetDlgItem(hDlg, IDC_PPSGCMBST), START_MANUAL);
                ComboBox_AddStringW(GetDlgItem(hDlg, IDC_PPSGCMBST), START_DISABLED);
                if (_currentEntry->lpConfig->dwStartType == SERVICE_AUTO_START)
                    ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_PPSGCMBST), 0);
                else if (_currentEntry->lpConfig->dwStartType == SERVICE_DEMAND_START)
                    ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_PPSGCMBST), 1);
                else if (_currentEntry->lpConfig->dwStartType == SERVICE_DISABLED)
                    ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_PPSGCMBST), 2);

                SetDlgItemTextW(hDlg, IDC_PPSGNAME, _currentEntry->szServiceName);
                SetDlgItemTextW(hDlg, IDC_PPSGDISP, _currentEntry->lpConfig->lpDisplayName);
                SetDlgItemTextW(hDlg, IDC_PPSGDESC, _currentEntry->szServiceDescription);
                SetDlgItemTextW(hDlg, IDC_PPSGDEXE, _currentEntry->lpConfig->lpBinaryPathName);
                __generalPropertyRefresh(hDlg);
            }
        break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_PPSGCMBST:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 1);
                    }
                break;
                case IDC_PPSGDISP:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        GetDlgItemTextW(hDlg, IDC_PPSGDISP, szBuf, SIZ_RESMAX);
                        if (!lstrcmpW(szBuf, _currentEntry->lpConfig->lpDisplayName)) {
                            PropSheet_UnChanged(GetParent(hDlg), hDlg);
                            CLR_BIT_FLAG(_propertyChanged, 1);
                        }
                        else {
                            PropSheet_Changed(GetParent(hDlg), hDlg);
                            SET_BIT_FLAG(_propertyChanged, 1);
                        }
                    }
                break;
                case IDC_PPSGDESC:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        GetDlgItemTextW(hDlg, IDC_PPSGDESC, szBuf, SIZ_DESMAX);
                        if (!lstrcmpW(szBuf, _currentEntry->szServiceDescription)) {
                            PropSheet_UnChanged(GetParent(hDlg), hDlg);
                            CLR_BIT_FLAG(_propertyChanged, 1);
                        }
                        else {
                            PropSheet_Changed(GetParent(hDlg), hDlg);
                            SET_BIT_FLAG(_propertyChanged, 1);
                        }
                    }
                break;
                case IDC_PPSGSTART:
                    apxProgressBox(hDlg, apxLoadResource(IDS_HSSTART, 0),
                                   _currentEntry->lpConfig->lpDisplayName,
                                   __startServiceCallback, NULL);
                    __generalPropertyRefresh(hDlg);
                break;
                case IDC_PPSGSTOP:
                    apxProgressBox(hDlg, apxLoadResource(IDS_HSSTOP, 0),
                                   _currentEntry->lpConfig->lpDisplayName,
                                   __stopServiceCallback, NULL);
                    __generalPropertyRefresh(hDlg);
                break;
                case IDC_PPSGPAUSE:
                    apxProgressBox(hDlg, apxLoadResource(IDS_HSPAUSE, 0),
                                   _currentEntry->lpConfig->lpDisplayName,
                                   __pauseServiceCallback, NULL);
                    __generalPropertyRefresh(hDlg);
                break;
                case IDC_PPSGRESTART:
                    apxProgressBox(hDlg, apxLoadResource(IDS_HSRESTART, 0),
                                   _currentEntry->lpConfig->lpDisplayName,
                                   __restartServiceCallback, NULL);
                    __generalPropertyRefresh(hDlg);
                break;
            }
        break;
        case WM_NOTIFY:
            lpShn = (LPPSHNOTIFY )lParam;
            switch (lpShn->hdr.code) {
                case PSN_APPLY:   /* sent when OK or Apply button pressed */
                    if (__generalPropertySave(hDlg)) {
                        PropSheet_UnChanged(GetParent(hDlg), hDlg);
                    }
                    else {
                        SET_BIT_FLAG(_propertyChanged, 1);
                        SetWindowLong(hDlg, DWLP_MSGRESULT,
                                      PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;
                    }

                break;
                default:
                break;
            }
        break;
        default:
        break;
    }

    return FALSE;
}

LRESULT CALLBACK __logonProperty(HWND hDlg,
                                 UINT uMessage,
                                 WPARAM wParam,
                                 LPARAM lParam)
{
    LPPSHNOTIFY lpShn;
    WCHAR       szBuf[SIZ_DESLEN];
    switch (uMessage) {
        case WM_INITDIALOG:
            {
                BOOL           bAccount = FALSE;
                startPage = 1;
                if (!bpropCentered)
                    apxCenterWindow(GetParent(hDlg), NULL);
                bpropCentered = TRUE;

                SendMessage(GetDlgItem(hDlg, IDC_PPSLUSER), EM_LIMITTEXT, 63, 0);
                SendMessage(GetDlgItem(hDlg, IDC_PPSLPASS), EM_LIMITTEXT, 63, 0);
                SendMessage(GetDlgItem(hDlg, IDC_PPSLCPASS), EM_LIMITTEXT, 63, 0);
                /* Check if we use LocalSystem or user defined account */
                if (lstrcmpiW(_currentEntry->szObjectName, STAT_SYSTEM)) {
                    bAccount = TRUE;
                    CheckRadioButton(hDlg, IDC_PPSLLS, IDC_PPSLUA, IDC_PPSLUA);
                    SetDlgItemTextW(hDlg, IDC_PPSLUSER, _currentEntry->szObjectName);
                    SetDlgItemTextW(hDlg, IDC_PPSLPASS, EMPTY_PASSWORD);
                    SetDlgItemTextW(hDlg, IDC_PPSLCPASS, EMPTY_PASSWORD);
                }
                else {
                    CheckRadioButton(hDlg, IDC_PPSLLS, IDC_PPSLUA, IDC_PPSLLS);
                    if (_currentEntry->lpConfig->dwServiceType &
                        SERVICE_INTERACTIVE_PROCESS)
                        CheckDlgButton(hDlg, IDC_PPSLID, BST_CHECKED);
                }
                EnableWindow(GetDlgItem(hDlg, IDC_PPSLID), !bAccount);
                EnableWindow(GetDlgItem(hDlg, IDC_PPSLUSER), bAccount);
                EnableWindow(GetDlgItem(hDlg, IDC_PPSLBROWSE), bAccount);
                EnableWindow(GetDlgItem(hDlg, IDL_PPSLPASS), bAccount);
                EnableWindow(GetDlgItem(hDlg, IDC_PPSLPASS), bAccount);
                EnableWindow(GetDlgItem(hDlg, IDL_PPSLCPASS), bAccount);
                EnableWindow(GetDlgItem(hDlg, IDC_PPSLCPASS), bAccount);
            }
        break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_PPSLLS:
                    SetDlgItemTextW(hDlg, IDC_PPSLUSER, L"");
                    SetDlgItemTextW(hDlg, IDC_PPSLPASS, L"");
                    SetDlgItemTextW(hDlg, IDC_PPSLCPASS, L"");
                    EnableWindow(GetDlgItem(hDlg, IDC_PPSLID), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_PPSLUSER), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDC_PPSLBROWSE), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDL_PPSLPASS), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDC_PPSLPASS), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDL_PPSLCPASS), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDC_PPSLCPASS), FALSE);
                    CheckRadioButton(hDlg, IDC_PPSLLS, IDC_PPSLUA, (INT)wParam);
                    if (lstrcmpiW(_currentEntry->szObjectName, STAT_SYSTEM)) {
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 2);
                    }
                    else {
                        PropSheet_UnChanged(GetParent(hDlg), hDlg);
                        CLR_BIT_FLAG(_propertyChanged, 2);
                    }
                    break;
                case IDC_PPSLUA:
                    SetDlgItemTextW(hDlg, IDC_PPSLUSER, _currentEntry->szObjectName);
                    SetDlgItemTextW(hDlg, IDC_PPSLPASS, EMPTY_PASSWORD);
                    SetDlgItemTextW(hDlg, IDC_PPSLCPASS, EMPTY_PASSWORD);
                    EnableWindow(GetDlgItem(hDlg, IDC_PPSLID), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDC_PPSLUSER), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_PPSLBROWSE), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDL_PPSLPASS), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_PPSLPASS), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDL_PPSLCPASS), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_PPSLCPASS), TRUE);
                    CheckRadioButton(hDlg, IDC_PPSLLS, IDC_PPSLUA, (INT)wParam);
                    if (lstrcmpW(_currentEntry->szObjectName, STAT_SYSTEM)) {
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 2);
                    }
                    else {
                        PropSheet_UnChanged(GetParent(hDlg), hDlg);
                        CLR_BIT_FLAG(_propertyChanged, 2);
                    }
                    break;
                case IDC_PPSLID:
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    SET_BIT_FLAG(_propertyChanged, 2);
                break;
                case IDC_PPSLUSER:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        GetDlgItemTextW(hDlg, IDC_PPSLUSER, szBuf, SIZ_RESMAX);
                        if (!lstrcmpiW(szBuf, _currentEntry->szObjectName)) {
                            PropSheet_UnChanged(GetParent(hDlg), hDlg);
                            CLR_BIT_FLAG(_propertyChanged, 2);
                        }
                        else {
                            PropSheet_Changed(GetParent(hDlg), hDlg);
                            SET_BIT_FLAG(_propertyChanged, 2);
                        }
                    }
                break;
                case IDC_PPSLPASS:
                case IDC_PPSLCPASS:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        WCHAR szP[SIZ_RESLEN];
                        WCHAR szC[SIZ_RESLEN];
                        GetDlgItemTextW(hDlg, IDC_PPSLPASS, szP, SIZ_RESMAX);
                        GetDlgItemTextW(hDlg, IDC_PPSLCPASS, szC, SIZ_RESMAX);
                        /* check for valid password match */
                        if (szP[0] == L' ' &&  szC[0] == L' ') {
                            PropSheet_UnChanged(GetParent(hDlg), hDlg);
                            CLR_BIT_FLAG(_propertyChanged, 2);
                        }
                        else if (!lstrcmpW(szP, szC)) {
                            PropSheet_Changed(GetParent(hDlg), hDlg);
                            SET_BIT_FLAG(_propertyChanged, 2);
                        }
                    }
                break;
                case IDC_PPSLBROWSE:
                    {
                        WCHAR szUser[SIZ_RESLEN];
                        if (apxDlgSelectUser(hDlg, szUser))
                            SetDlgItemTextW(hDlg, IDC_PPSLUSER, szUser);
                    }
                break;
            }
        break;
        case WM_NOTIFY:
            lpShn = (LPPSHNOTIFY )lParam;
            switch (lpShn->hdr.code) {
                case PSN_APPLY:   /* sent when OK or Apply button pressed */
                    if (__generalLogonSave(hDlg))
                        PropSheet_UnChanged(GetParent(hDlg), hDlg);
                    else {
                        SET_BIT_FLAG(_propertyChanged, 2);
                        SetWindowLong(hDlg, DWLP_MSGRESULT,
                                      PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;
                    }

                break;
            }
        break;

        default:
        break;
    }
    return FALSE;
}

LRESULT CALLBACK __loggingProperty(HWND hDlg,
                                   UINT uMessage,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
    LPPSHNOTIFY lpShn;
    LPWSTR      lpBuf;

    switch (uMessage) {
        case WM_INITDIALOG:
            {
                LPWSTR b;
                startPage = 2;
                if (!bpropCentered)
                    apxCenterWindow(GetParent(hDlg), NULL);
                bpropCentered = TRUE;
                ComboBox_AddStringW(GetDlgItem(hDlg, IDC_PPLGLEVEL), LOGL_ERROR);
                ComboBox_AddStringW(GetDlgItem(hDlg, IDC_PPLGLEVEL), LOGL_INFO);
                ComboBox_AddStringW(GetDlgItem(hDlg, IDC_PPLGLEVEL), LOGL_WARN);
                ComboBox_AddStringW(GetDlgItem(hDlg, IDC_PPLGLEVEL), LOGL_DEBUG);
                if ((b = apxRegistryGetStringW(hRegserv, APXREG_PARAMSOFTWARE,
                                               _s_log, L"Level")) != NULL) {
                    if (!lstrcmpiW(b, LOGL_ERROR))
                        ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_PPLGLEVEL), 0);
                    else if (!lstrcmpiW(b, LOGL_INFO))
                        ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_PPLGLEVEL), 1);
                    else if (!lstrcmpiW(b, LOGL_WARN))
                        ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_PPLGLEVEL), 2);
                    else
                        ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_PPLGLEVEL), 3);
                    apxFree(b);
                }
                else
                    ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_PPLGLEVEL), 1);
                if ((b = apxRegistryGetStringW(hRegserv, APXREG_PARAMSOFTWARE,
                                               _s_log, L"Path")) != NULL) {
                    SetDlgItemTextW(hDlg, IDC_PPLGPATH, b);
                    apxFree(b);
                }
                if ((b = apxRegistryGetStringW(hRegserv, APXREG_PARAMSOFTWARE,
                                               _s_log, L"Prefix")) != NULL) {
                    SetDlgItemTextW(hDlg, IDC_PPLGPREFIX, b);
                    apxFree(b);
                }
                else
                    SetDlgItemTextW(hDlg, IDC_PPLGPREFIX, L"commons-daemon");
                if ((b = apxRegistryGetStringW(hRegserv, APXREG_PARAMSOFTWARE,
                                               _s_log, L"PidFile")) != NULL) {
                    SetDlgItemTextW(hDlg, IDC_PPLGPIDFILE, b);
                    apxFree(b);
                }
                if ((b = apxRegistryGetStringW(hRegserv, APXREG_PARAMSOFTWARE,
                                               _s_log, L"StdOutput")) != NULL) {
                    SetDlgItemTextW(hDlg, IDC_PPLGSTDOUT, b);
                    apxFree(b);
                }
                if ((b = apxRegistryGetStringW(hRegserv, APXREG_PARAMSOFTWARE,
                                               _s_log, L"StdError")) != NULL) {
                    SetDlgItemTextW(hDlg, IDC_PPLGSTDERR, b);
                    apxFree(b);
                }
            }
        break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_PPLGLEVEL:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 3);
                    }
                break;
                case IDC_PPLGPATH:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 3);
                    }
                break;
                case IDC_PPLGPREFIX:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 3);
                    }
                break;
                case IDC_PPLGPIDFILE:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 3);
                    }
                break;
                case IDC_PPLGSTDERR:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 3);
                    }
                break;
                case IDC_PPLGSTDOUT:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 3);
                    }
                break;
                case IDC_PPLGBPATH:
                    lpBuf = apxBrowseForFolderW(hDlg, apxLoadResourceW(IDS_LGPATHTITLE, 0),
                                                NULL);
                    if (lpBuf) {
                        SetDlgItemTextW(hDlg, IDC_PPLGPATH, lpBuf);
                        apxFree(lpBuf);
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 3);
                    }
                break;
                case IDC_PPLGBSTDOUT:
                    lpBuf = apxGetFileNameW(hDlg, apxLoadResourceW(IDS_LGSTDOUT, 0),
                                            apxLoadResourceW(IDS_ALLFILES, 1), NULL,
                                            NULL, FALSE, NULL);
                    if (lpBuf) {
                        SetDlgItemTextW(hDlg, IDC_PPLGSTDOUT, lpBuf);
                        apxFree(lpBuf);
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 3);
                    }
                break;
                case IDC_PPLGBSTDERR:
                    lpBuf = apxGetFileNameW(hDlg, apxLoadResourceW(IDS_LGSTDERR, 0),
                                            apxLoadResourceW(IDS_ALLFILES, 1), NULL,
                                            NULL, FALSE, NULL);
                    if (lpBuf) {
                        SetDlgItemTextW(hDlg, IDC_PPLGSTDERR, lpBuf);
                        apxFree(lpBuf);
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 3);
                    }
                break;
            }
        break;
        case WM_NOTIFY:
            lpShn = (LPPSHNOTIFY )lParam;
            switch (lpShn->hdr.code) {
                case PSN_APPLY:   /* sent when OK or Apply button pressed */
                    if (__generalLoggingSave(hDlg))
                        PropSheet_UnChanged(GetParent(hDlg), hDlg);
                    else {
                        SET_BIT_FLAG(_propertyChanged, 3);
                        SetWindowLong(hDlg, DWLP_MSGRESULT,
                                      PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;
                    }

                break;
            }
        break;

        default:
        break;
    }
    return FALSE;
}

LRESULT CALLBACK __jvmProperty(HWND hDlg,
                               UINT uMessage,
                               WPARAM wParam,
                               LPARAM lParam)
{
    LPPSHNOTIFY lpShn;
    LPWSTR      lpBuf, b;
    DWORD       v;
    CHAR        bn[32];

    switch (uMessage) {
        case WM_INITDIALOG:
            {
                startPage = 3;
                if (!bpropCentered)
                    apxCenterWindow(GetParent(hDlg), NULL);
                bpropCentered = TRUE;
                if ((lpBuf = apxRegistryGetStringW(hRegserv, APXREG_PARAMSOFTWARE,
                                               _s_java, L"Jvm")) != NULL) {
                    if (!lstrcmpiW(lpBuf, L"auto")) {
                        CheckDlgButton(hDlg, IDC_PPJAUTO, BST_CHECKED);
                        apxFree(lpBuf);
                        lpBuf = apxGetJavaSoftRuntimeLib(hPool);
                        EnableWindow(GetDlgItem(hDlg, IDC_PPJJVM), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_PPJBJVM), FALSE);
                    }
                    if (lpBuf) {
                        SetDlgItemTextW(hDlg, IDC_PPJJVM, lpBuf);
                        apxFree(lpBuf);
                    }
                }
                if ((lpBuf = apxRegistryGetStringW(hRegserv, APXREG_PARAMSOFTWARE,
                                               _s_java, L"Classpath")) != NULL) {
                    SetDlgItemTextW(hDlg, IDC_PPJCLASSPATH, lpBuf);
                    apxFree(lpBuf);
                }
                if ((lpBuf = apxRegistryGetMzStrW(hRegserv, APXREG_PARAMSOFTWARE,
                                               _s_java, L"Options", NULL, NULL)) != NULL) {
                    LPWSTR p = apxMszToCRLFW(hPool, lpBuf);
                    SetDlgItemTextW(hDlg, IDC_PPJOPTIONS, p);
                    apxFree(lpBuf);
                    apxFree(p);
                }
                v = apxRegistryGetNumberW(hRegserv, APXREG_PARAMSOFTWARE,
                                          _s_java, L"JvmMs");
                if (v && v != 0xFFFFFFFF) {
                    wsprintfA(bn, "%d", v);
                    SetDlgItemTextA(hDlg, IDC_PPJMS, bn);
                }
                v = apxRegistryGetNumberW(hRegserv, APXREG_PARAMSOFTWARE,
                                          _s_java, L"JvmMx");
                if (v && v != 0xFFFFFFFF) {
                    wsprintfA(bn, "%d", v);
                    SetDlgItemTextA(hDlg, IDC_PPJMX, bn);
                }
                v = apxRegistryGetNumberW(hRegserv, APXREG_PARAMSOFTWARE,
                                          _s_java, L"JvmSs");
                if (v && v != 0xFFFFFFFF) {
                    wsprintfA(bn, "%d", v);
                    SetDlgItemTextA(hDlg, IDC_PPJSS, bn);
                }

            }
        break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_PPJBJVM:
                    b = apxGetJavaSoftHome(hPool, TRUE);
                    lpBuf = apxGetFileNameW(hDlg, apxLoadResourceW(IDS_PPJBJVM, 0),
                                            apxLoadResourceW(IDS_DLLFILES, 1), NULL,
                                            b,
                                            TRUE, NULL);
                    apxFree(b);
                    if (lpBuf) {
                        SetDlgItemTextW(hDlg, IDC_PPJJVM, lpBuf);
                        apxFree(lpBuf);
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 4);
                    }
                break;
                case IDC_PPJAUTO:
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    SET_BIT_FLAG(_propertyChanged, 4);
                    if (IsDlgButtonChecked(hDlg, IDC_PPJAUTO)) {
                        EnableWindow(GetDlgItem(hDlg, IDC_PPJJVM), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_PPJBJVM), FALSE);
                        lpBuf = apxGetJavaSoftRuntimeLib(hPool);
                        if (lpBuf) {
                            SetDlgItemTextW(hDlg, IDC_PPJJVM, lpBuf);
                            apxFree(lpBuf);
                        }
                    }
                    else {
                        EnableWindow(GetDlgItem(hDlg, IDC_PPJJVM), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_PPJBJVM), TRUE);
                    }
                break;
                case IDC_PPJJVM:
                case IDC_PPJCLASSPATH:
                case IDC_PPJOPTIONS:
                case IDC_PPJMX:
                case IDC_PPJMS:
                case IDC_PPJSS:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 4);
                    }
                break;
            }
        break;
        case WM_NOTIFY:
            lpShn = (LPPSHNOTIFY )lParam;
            switch (lpShn->hdr.code) {
                case PSN_APPLY:   /* sent when OK or Apply button pressed */
                    if (__generalJvmSave(hDlg))
                        PropSheet_UnChanged(GetParent(hDlg), hDlg);
                    else {
                        SET_BIT_FLAG(_propertyChanged, 4);
                        SetWindowLong(hDlg, DWLP_MSGRESULT,
                                      PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;
                    }

                break;
            }
        break;

        default:
        break;
    }
    return FALSE;
}

LRESULT CALLBACK __startProperty(HWND hDlg,
                                 UINT uMessage,
                                 WPARAM wParam,
                                 LPARAM lParam)
{
    LPPSHNOTIFY lpShn;
    LPWSTR      lpBuf, b;

    switch (uMessage) {
        case WM_INITDIALOG:
            {
                startPage = 4;
                if (!bpropCentered)
                    apxCenterWindow(GetParent(hDlg), NULL);
                bpropCentered = TRUE;

                ComboBox_AddStringW(GetDlgItem(hDlg, IDC_PPRMODE), L"exe");
                ComboBox_AddStringW(GetDlgItem(hDlg, IDC_PPRMODE), L"jvm");
                ComboBox_AddStringW(GetDlgItem(hDlg, IDC_PPRMODE), _s_java);

                if ((lpBuf = apxRegistryGetStringW(hRegserv, APXREG_PARAMSOFTWARE,
                                                   _s_start, L"Class")) != NULL) {
                    SetDlgItemTextW(hDlg, IDC_PPRCLASS, lpBuf);
                    apxFree(lpBuf);
                }
                if ((lpBuf = apxRegistryGetStringW(hRegserv, APXREG_PARAMSOFTWARE,
                                                   _s_start, L"Image")) != NULL) {
                    SetDlgItemTextW(hDlg, IDC_PPRIMAGE, lpBuf);
                    apxFree(lpBuf);
                }
                if ((lpBuf = apxRegistryGetStringW(hRegserv, APXREG_PARAMSOFTWARE,
                                                   _s_start, L"WorkingPath")) != NULL) {
                    SetDlgItemTextW(hDlg, IDC_PPRWPATH, lpBuf);
                    apxFree(lpBuf);
                }
                if ((lpBuf = apxRegistryGetStringW(hRegserv, APXREG_PARAMSOFTWARE,
                                                   _s_start, L"Method")) != NULL) {
                    SetDlgItemTextW(hDlg, IDC_PPRMETHOD, lpBuf);
                    apxFree(lpBuf);
                }
                if ((lpBuf = apxRegistryGetMzStrW(hRegserv, APXREG_PARAMSOFTWARE,
                                               _s_start, L"Params", NULL, NULL)) != NULL) {
                    b = apxMszToCRLFW(hPool, lpBuf);
                    SetDlgItemTextW(hDlg, IDC_PPRARGS, b);
                    apxFree(lpBuf);
                    apxFree(b);
                }
                if ((lpBuf = apxRegistryGetStringW(hRegserv, APXREG_PARAMSOFTWARE,
                                                   _s_start, L"Mode")) != NULL) {
                    if (!lstrcmpiW(lpBuf, L"jvm")) {
                        ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_PPRMODE), 1);

                    }
                    else if (!lstrcmpiW(lpBuf, _s_java)) {
                        ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_PPRMODE), 2);
                    }
                    else {
                        ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_PPRMODE), 0);
                        EnableWindow(GetDlgItem(hDlg, IDC_PPRIMAGE), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_PPRBIMAGE), TRUE);
                    }
                    apxFree(lpBuf);
                }
            }
        break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_PPRBWPATH:
                    lpBuf = apxBrowseForFolderW(hDlg, apxLoadResourceW(IDS_PPWPATH, 0),
                                                NULL);
                    if (lpBuf) {
                        SetDlgItemTextW(hDlg, IDC_PPRWPATH, lpBuf);
                        apxFree(lpBuf);
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 5);
                    }
                break;
                case IDC_PPRBIMAGE:
                    lpBuf = apxGetFileNameW(hDlg, apxLoadResourceW(IDS_PPIMAGE, 0),
                                            apxLoadResourceW(IDS_EXEFILES, 1), NULL,
                                            NULL, TRUE, NULL);
                    if (lpBuf) {
                        SetDlgItemTextW(hDlg, IDC_PPRIMAGE, lpBuf);
                        apxFree(lpBuf);
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 5);
                    }
                break;
                case IDC_PPRCLASS:
                case IDC_PPRMETHOD:
                case IDC_PPRARGS:
                case IDC_PPRIMAGE:
                case IDC_PPRWPATH:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 5);
                    }
                break;
                case IDC_PPRMODE:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 5);
                        if (ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PPRMODE))) {
                            EnableWindow(GetDlgItem(hDlg, IDC_PPRIMAGE), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDC_PPRBIMAGE), FALSE);
                        }
                        else {
                            EnableWindow(GetDlgItem(hDlg, IDC_PPRIMAGE), TRUE);
                            EnableWindow(GetDlgItem(hDlg, IDC_PPRBIMAGE), TRUE);
                        }
                    }
                break;

            }
        break;
        case WM_NOTIFY:
            lpShn = (LPPSHNOTIFY )lParam;
            switch (lpShn->hdr.code) {
                case PSN_APPLY:   /* sent when OK or Apply button pressed */
                    if (__generalStartSave(hDlg))
                        PropSheet_UnChanged(GetParent(hDlg), hDlg);
                    else {
                        SET_BIT_FLAG(_propertyChanged, 5);
                        SetWindowLong(hDlg, DWLP_MSGRESULT,
                                      PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;
                    }

                break;
            }
        break;

        default:
        break;
    }
    return FALSE;
}

LRESULT CALLBACK __stopProperty(HWND hDlg,
                                UINT uMessage,
                                WPARAM wParam,
                                LPARAM lParam)
{
    LPPSHNOTIFY lpShn;
    LPWSTR      lpBuf, b;
    DWORD       v;

    switch (uMessage) {
        case WM_INITDIALOG:
            {
                startPage = 5;
                if (!bpropCentered)
                    apxCenterWindow(GetParent(hDlg), NULL);
                bpropCentered = TRUE;

                ComboBox_AddStringW(GetDlgItem(hDlg, IDC_PPSMODE), L"exe");
                ComboBox_AddStringW(GetDlgItem(hDlg, IDC_PPSMODE), L"jvm");
                ComboBox_AddStringW(GetDlgItem(hDlg, IDC_PPSMODE), _s_java);

                if ((lpBuf = apxRegistryGetStringW(hRegserv, APXREG_PARAMSOFTWARE,
                                                   _s_stop, L"Class")) != NULL) {
                    SetDlgItemTextW(hDlg, IDC_PPSCLASS, lpBuf);
                    apxFree(lpBuf);
                }
                if ((lpBuf = apxRegistryGetStringW(hRegserv, APXREG_PARAMSOFTWARE,
                                                   _s_stop, L"Image")) != NULL) {
                    SetDlgItemTextW(hDlg, IDC_PPSIMAGE, lpBuf);
                    apxFree(lpBuf);
                }
                if ((lpBuf = apxRegistryGetStringW(hRegserv, APXREG_PARAMSOFTWARE,
                                                   _s_stop, L"WorkingPath")) != NULL) {
                    SetDlgItemTextW(hDlg, IDC_PPSWPATH, lpBuf);
                    apxFree(lpBuf);
                }
                if ((lpBuf = apxRegistryGetStringW(hRegserv, APXREG_PARAMSOFTWARE,
                                                   _s_stop, L"Method")) != NULL) {
                    SetDlgItemTextW(hDlg, IDC_PPSMETHOD, lpBuf);
                    apxFree(lpBuf);
                }
                if ((lpBuf = apxRegistryGetMzStrW(hRegserv, APXREG_PARAMSOFTWARE,
                                               _s_stop, L"Params", NULL, NULL)) != NULL) {
                    b = apxMszToCRLFW(hPool, lpBuf);
                    SetDlgItemTextW(hDlg, IDC_PPSARGS, b);
                    apxFree(lpBuf);
                    apxFree(b);
                }
                v = apxRegistryGetNumberW(hRegserv, APXREG_PARAMSOFTWARE,
                                          _s_stop, L"Timeout");
                {
                    CHAR bn[32];
                    wsprintfA(bn, "%d", v);
                    SetDlgItemTextA(hDlg, IDC_PPSTIMEOUT, bn);
                }
                if ((lpBuf = apxRegistryGetStringW(hRegserv, APXREG_PARAMSOFTWARE,
                                                   _s_stop, L"Mode")) != NULL) {
                    if (!lstrcmpiW(lpBuf, L"jvm")) {
                        ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_PPSMODE), 1);

                    }
                    else if (!lstrcmpiW(lpBuf, _s_java)) {
                        ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_PPSMODE), 2);
                    }
                    else {
                        ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_PPSMODE), 0);
                        EnableWindow(GetDlgItem(hDlg, IDC_PPSIMAGE), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_PPSBIMAGE), TRUE);
                    }
                    apxFree(lpBuf);
                }
            }
        break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_PPSBWPATH:
                    lpBuf = apxBrowseForFolderW(hDlg, apxLoadResourceW(IDS_PPWPATH, 0),
                                                NULL);
                    if (lpBuf) {
                        SetDlgItemTextW(hDlg, IDC_PPSWPATH, lpBuf);
                        apxFree(lpBuf);
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 6);
                    }
                break;
                case IDC_PPSBIMAGE:
                    lpBuf = apxGetFileNameW(hDlg, apxLoadResourceW(IDS_PPIMAGE, 0),
                                            apxLoadResourceW(IDS_EXEFILES, 1), NULL,
                                            NULL, TRUE, NULL);
                    if (lpBuf) {
                        SetDlgItemTextW(hDlg, IDC_PPSIMAGE, lpBuf);
                        apxFree(lpBuf);
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 6);
                    }
                break;
                case IDC_PPSCLASS:
                case IDC_PPSMETHOD:
                case IDC_PPSTIMEOUT:
                case IDC_PPSARGS:
                case IDC_PPSIMAGE:
                case IDC_PPSWPATH:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 6);
                    }
                break;
                case IDC_PPSMODE:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        SET_BIT_FLAG(_propertyChanged, 6);
                        if (ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PPSMODE))) {
                            EnableWindow(GetDlgItem(hDlg, IDC_PPSIMAGE), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDC_PPSBIMAGE), FALSE);
                        }
                        else {
                            EnableWindow(GetDlgItem(hDlg, IDC_PPSIMAGE), TRUE);
                            EnableWindow(GetDlgItem(hDlg, IDC_PPSBIMAGE), TRUE);
                        }
                    }
                break;

            }
        break;
        case WM_NOTIFY:
            lpShn = (LPPSHNOTIFY )lParam;
            switch (lpShn->hdr.code) {
                case PSN_APPLY:   /* sent when OK or Apply button pressed */
                    if (__generalStopSave(hDlg))
                        PropSheet_UnChanged(GetParent(hDlg), hDlg);
                    else {
                        SET_BIT_FLAG(_propertyChanged, 6);
                        SetWindowLong(hDlg, DWLP_MSGRESULT,
                                      PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;
                    }

                break;
            }
        break;

        default:
        break;
    }
    return FALSE;
}

void __initPpage(PROPSHEETPAGEW *lpPage, INT iDlg, INT iTitle, DLGPROC pfnDlgProc)
{
    lpPage->dwSize      = sizeof(PROPSHEETPAGE);
    lpPage->dwFlags     = PSP_USETITLE;
    lpPage->hInstance   = _gui_store->hInstance;
    lpPage->pszTemplate = MAKEINTRESOURCEW(iDlg);
    lpPage->pszIcon     = NULL;
    lpPage->pfnDlgProc  = pfnDlgProc;
    lpPage->pszTitle    = MAKEINTRESOURCEW(iTitle);
    lpPage->lParam      = 0;
}

void ShowServiceProperties(HWND hWnd)
{
    PROPSHEETPAGEW   psP[6];
    PROPSHEETHEADERW psH;
    WCHAR           szT[SIZ_DESLEN] = {0};

    if (_propertyOpened) {
        SetForegroundWindow(_gui_store->hMainWnd);
        return;
    }
    __initPpage(&psP[0], IDD_PROPPAGE_SGENERAL, IDS_PPGENERAL,
                __generalProperty);
    __initPpage(&psP[1], IDD_PROPPAGE_LOGON, IDS_PPLOGON,
                __logonProperty);
    __initPpage(&psP[2], IDD_PROPPAGE_LOGGING, IDS_PPLOGGING,
                __loggingProperty);
    __initPpage(&psP[3], IDD_PROPPAGE_JVM, IDS_PPJAVAVM,
                __jvmProperty);
    __initPpage(&psP[4], IDD_PROPPAGE_START, IDS_PPSTART,
                __startProperty);
    __initPpage(&psP[5], IDD_PROPPAGE_STOP, IDS_PPSTOP,
                __stopProperty);

    if (_currentEntry && _currentEntry->lpConfig)
        lstrlcpyW(szT, SIZ_DESMAX, _currentEntry->lpConfig->lpDisplayName);
    else
        return;
    lstrlcatW(szT, SIZ_DESMAX, L" Properties");

    psH.dwSize           = sizeof(PROPSHEETHEADER);
    psH.dwFlags          = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_USECALLBACK | PSH_NOCONTEXTHELP;
    psH.hwndParent       = bEnableTry ? hWnd : NULL;
    psH.hInstance        = _gui_store->hInstance;
    psH.pszIcon          = MAKEINTRESOURCEW(IDI_MAINICON);
    psH.pszCaption       = szT;
    psH.nPages           = 6;
    psH.ppsp             = (LPCPROPSHEETPAGEW) &psP;
    psH.pfnCallback      = (PFNPROPSHEETCALLBACK)__propertyCallback;
    psH.nStartPage       = startPage;

    PropertySheetW(&psH);
    _propertyOpened = FALSE;
    _generalPropertyHwnd = NULL;
    if (!bEnableTry)
        PostQuitMessage(0);
     bpropCentered = FALSE;

}

static void signalService(LPCWSTR szServiceName)
{
    HANDLE event;
    WCHAR en[SIZ_DESLEN];
    int i;

    lstrlcpyW(en, SIZ_DESLEN, L"Global\\");
    lstrlcatW(en, SIZ_DESLEN, szServiceName);
    lstrlcatW(en, SIZ_DESLEN, L"SIGNAL");
    for (i = 7; i < lstrlenW(en); i++) {
        if (en[i] == L' ')
            en[i] = L'_';
        else
            en[i] = towupper(en[i]);
    }


    event = OpenEventW(EVENT_MODIFY_STATE, FALSE, en);
    if (event) {
        SetEvent(event);
        CloseHandle(event);
    }
    else
        apxDisplayError(TRUE, NULL, 0, "Unable to open the Event Mutex");

}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg,
                             WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_CREATE:
            if (bEnableTry) {
                if (_currentEntry && _currentEntry->lpConfig) {
                    BOOL isRunning = _currentEntry->stServiceStatus.dwCurrentState == SERVICE_RUNNING;
                    apxManageTryIconW(hWnd, NIM_ADD, NULL,
                                      _currentEntry->lpConfig->lpDisplayName,
                                      isRunning ? hIcoRun : hIcoStop);
                }
                else {
                    apxManageTryIconA(hWnd, NIM_ADD, NULL,
                                      apxLoadResourceA(IDS_APPLICATION, 0),
                                      NULL);
                }
            }
            else
                ShowServiceProperties(hWnd);

        break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDM_TM_CONFIG:
                    ShowServiceProperties(hWnd);
                break;
                case IDM_TM_ABOUT:
                    apxAboutBox(hWnd);
                break;
                case IDM_TM_EXIT:
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                break;
                case IDM_TM_START:
                    if (!_propertyOpened)
                        apxProgressBox(hWnd, apxLoadResource(IDS_HSSTART, 0),
                                       _currentEntry->lpConfig->lpDisplayName,
                                       __startServiceCallback, NULL);
                break;
                case IDM_TM_STOP:
                    if (!_propertyOpened)
                        apxProgressBox(hWnd, apxLoadResource(IDS_HSSTOP, 0),
                                       _currentEntry->lpConfig->lpDisplayName,
                                       __stopServiceCallback, NULL);
                break;
                case IDM_TM_PAUSE:
                    if (!_propertyOpened)
                        apxProgressBox(hWnd, apxLoadResource(IDS_HSPAUSE, 0),
                                       _currentEntry->lpConfig->lpDisplayName,
                                       __pauseServiceCallback, NULL);
                break;
                case IDM_TM_RESTART:
                    if (!_propertyOpened)
                        apxProgressBox(hWnd, apxLoadResource(IDS_HSRESTART, 0),
                                       _currentEntry->lpConfig->lpDisplayName,
                                       __restartServiceCallback, NULL);
                break;
                case IDM_TM_DUMP:
                    signalService(_currentEntry->szServiceName);
                break;
                case IDMS_REFRESH:
                    if (bEnableTry &&
                        (_currentEntry = apxServiceEntry(hService, TRUE)) != NULL) {
                        BOOL isRunning = _currentEntry->stServiceStatus.dwCurrentState == SERVICE_RUNNING;
                        apxManageTryIconW(hWnd, NIM_MODIFY, NULL,
                                          _currentEntry->lpConfig->lpDisplayName,
                                          isRunning ? hIcoRun : hIcoStop);
                    }
                break;

            }
        break;
        case WM_TRAYMESSAGE:
            switch (lParam) {
                case WM_LBUTTONDBLCLK:
                    ShowServiceProperties(hWnd);
                break;
                case WM_RBUTTONUP:
                    _currentEntry = apxServiceEntry(hService, TRUE);
                    createRbuttonTryMenu(hWnd);
                break;
            }
        break;
        case WM_QUIT:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        break;
        case WM_DESTROY:
            if (bEnableTry)
                apxManageTryIconA(hWnd, NIM_DELETE, NULL, NULL, NULL);
            PostQuitMessage(0);
        break;
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        break;
    }

    return FALSE;
}

static BOOL loadConfiguration()
{
    return TRUE;
}

static BOOL saveConfiguration()
{
    return TRUE;
}

static BOOL isManagerRunning = FALSE;
static DWORD WINAPI refreshThread(LPVOID lpParam)
{
    while (isManagerRunning) {
        /* Refresh property window */
        DWORD os = 0;
        if (_currentEntry)
            os = _currentEntry->stServiceStatus.dwCurrentState;
        _currentEntry = apxServiceEntry(hService, TRUE);
        if (_currentEntry && _currentEntry->stServiceStatus.dwCurrentState != os) {
            if (_gui_store->hMainWnd)
                PostMessage(_gui_store->hMainWnd, WM_COMMAND,
                            MAKEWPARAM(IDMS_REFRESH, 0), 0);
            if (_generalPropertyHwnd)
                __generalPropertyRefresh(_generalPropertyHwnd);
        }
        Sleep(1000);
    }
    return 0;
}

/* Main program entry
 * Since we are inependant from CRT
 * the arguments are not used
 */
#ifdef _NO_CRTLIBRARY
int xMain(void)
#else
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
#endif
{
    MSG    msg;
    LPAPXCMDLINE lpCmdline;
    HANDLE mutex = NULL;
    BOOL quiet = FALSE;

    apxHandleManagerInitialize();
    hPool     = apxPoolCreate(NULL, 0);

    /* Parse the command line */
    if ((lpCmdline = apxCmdlineParse(hPool, _options, _commands, _altcmds)) == NULL) {
        /* TODO: dispalay error message */
        apxDisplayError(TRUE, NULL, 0, "Error parsing command line");
        goto cleanup;
    }

    if (!lpCmdline->dwCmdIndex) {
        /* Skip sytem error message */
        SetLastError(ERROR_SUCCESS);
        apxDisplayError(TRUE, NULL, 0,
                        apxLoadResourceA(IDS_ERRORCMD, 0),
                        lpCmdLine);
        goto cleanup;
    }
    else if (lpCmdline->dwCmdIndex == 4)
        quiet = TRUE;
    else if (lpCmdline->dwCmdIndex >= 2)
        bEnableTry = TRUE;
    hService = apxCreateService(hPool, SC_MANAGER_CONNECT, FALSE);
    if (IS_INVALID_HANDLE(hService)) {
        if (!quiet)
            apxDisplayError(TRUE, NULL, 0, "Unable to open the Service Manager");
        goto cleanup;
    }
    /* Open the main service handle */
    if (!apxServiceOpen(hService, lpCmdline->szApplication,
                        SERVICE_ALL_ACCESS)) {
        LPWSTR w = lpCmdline->szApplication + lstrlenW(lpCmdline->szApplication) - 1;
        if (*w == L'w')
            *w = L'\0';
        if (!apxServiceOpen(hService, lpCmdline->szApplication,
                            SERVICE_ALL_ACCESS)) {
            if (!apxServiceOpen(hService, lpCmdline->szApplication,
                                GENERIC_READ | GENERIC_EXECUTE)) {
                if (!quiet)
                    apxDisplayError(TRUE, NULL, 0, "Unable to open the service '%S'",
                                    lpCmdline->szApplication);
                goto cleanup;
            }
        }
    }
    /* Obtain service parameters and status */
    if (!(_currentEntry = apxServiceEntry(hService, TRUE))) {
        if (!quiet)
            apxDisplayError(TRUE, NULL, 0, "Unable to query the service '%S' status",
                            lpCmdline->szApplication);
        goto cleanup;
    }
#ifdef _UNICODE
    _gui_store = apxGuiInitialize(MainWndProc, lpCmdline->szApplication);
#else
    {
        CHAR szApp[MAX_PATH];
        _gui_store = apxGuiInitialize(MainWndProc,
                                  WideToAscii(lpCmdline->szApplication, szApp));
    }
#endif
    if (!_gui_store) {
        if (!quiet)
            apxDisplayError(TRUE, NULL, 0, "Unable to initialize GUI manager");
        goto cleanup;
    }
    hIcoRun  = LoadImage(_gui_store->hInstance, MAKEINTRESOURCE(IDI_ICONRUN),
                         IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    hIcoStop = LoadImage(_gui_store->hInstance, MAKEINTRESOURCE(IDI_ICONSTOP),
                         IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

    /* Handle //MQ// option */
    if (lpCmdline->dwCmdIndex == 4) {
        HANDLE hOther = FindWindow(_gui_store->szWndClass, NULL);
        if (hOther)
            SendMessage(hOther, WM_CLOSE, 0, 0);
        goto cleanup;
    }

    if (!_options[0].dwValue) {
        mutex = CreateMutex(NULL, FALSE, _gui_store->szWndMutex);
        if ((mutex == NULL) || (GetLastError() == ERROR_ALREADY_EXISTS)) {
            HANDLE hOther = FindWindow(_gui_store->szWndClass, NULL);
            if (hOther) {
                SetForegroundWindow(hOther);
                SendMessage(hOther, WM_COMMAND, MAKEWPARAM(IDM_TM_CONFIG, 0), 0);
            }
            else {
                /* Skip sytem error message */
                SetLastError(ERROR_SUCCESS);
                if (!quiet)
                    apxDisplayError(TRUE, NULL, 0, apxLoadResourceA(IDS_ALREAY_RUNING, 0),
                                    lpCmdline->szApplication);
            }
            goto cleanup;
        }
    }
    hRegistry = apxCreateRegistry(hPool, KEY_ALL_ACCESS, NULL,
                                  apxLoadResource(IDS_APPLICATION, 0),
                                  APXREG_USER);
    loadConfiguration();
    hRegserv = apxCreateRegistryW(hPool, KEY_READ | KEY_WRITE | KREG_WOW6432,
                                  PRG_REGROOT,
                                  lpCmdline->szApplication,
                                  APXREG_SOFTWARE | APXREG_SERVICE);

    if (IS_INVALID_HANDLE(hRegserv)) {
        if (!quiet)
            apxDisplayError(TRUE, NULL, 0, apxLoadResourceA(IDS_ERRSREG, 0));
        return FALSE;
    }
    isManagerRunning = TRUE;
    CreateThread(NULL, 0, refreshThread, NULL, 0, NULL);
    /* Create main invisible window */
    _gui_store->hMainWnd = CreateWindow(_gui_store->szWndClass,
                                        apxLoadResource(IDS_APPLICATION, 0),
                                        0, 0, 0, 0, 0,
                                        NULL, NULL,
                                        _gui_store->hInstance,
                                        NULL);

    if (!_gui_store->hMainWnd) {
        goto cleanup;
    }
    if (lpCmdline->dwCmdIndex == 3)
        PostMessage(_gui_store->hMainWnd, WM_COMMAND, IDM_TM_START, 0);
    while (GetMessage(&msg, NULL, 0, 0))  {
        if(!TranslateAccelerator(_gui_store->hMainWnd,
                                 _gui_store->hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    isManagerRunning = FALSE;
    saveConfiguration();

cleanup:
    if (hIcoStop)
        DestroyIcon(hIcoStop);
    if (hIcoRun)
        DestroyIcon(hIcoRun);
    if (mutex)
        CloseHandle(mutex);
    if (lpCmdline)
        apxCmdlineFree(lpCmdline);
    apxCloseHandle(hService);
    apxHandleManagerDestroy();
    ExitProcess(0);
    return 0;
}

