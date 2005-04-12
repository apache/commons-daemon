/* Copyright 2000-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
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
#include "apsvcmgr.h"

LPAPXGUISTORE _gui_store  = NULL;
HWND          hWndToolbar = NULL;
HWND          hWndList    = NULL;
HWND          hWndListHdr = NULL;
WNDPROC       ListViewWinMain;
WNDPROC       ListViewWinHead;
HWND          hWndStatus  = NULL;
HWND          hWndModal   = NULL;
LVHITTESTINFO lvLastHit;
int           lvHitItem = -1;
/* Toolbar buttons 
 * TODO: Localize...
 */
TBBUTTON tbButtons[] = {
    { 4, IDAM_SAVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    { 5, IDAM_NEW, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    { 7, IDMS_REFRESH, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {16, IDMV_FILTER, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0},
    { 8, IDAM_DELETE, 0, TBSTYLE_BUTTON, 0, 0},
    { 6, IDMS_PROPERTIES, 0, TBSTYLE_BUTTON, 0, 0},
    { 0, IDMS_START, 0, TBSTYLE_BUTTON, 0, 0},
    { 1, IDMS_STOP, 0, TBSTYLE_BUTTON, 0, 0},
    { 2, IDMS_PAUSE, 0, TBSTYLE_BUTTON, 0, 0},
    { 3, IDMS_RESTART, 0, TBSTYLE_BUTTON, 0, 0},
    { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0},
    {11, IDMH_HELP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0}
};

APXLVITEM lvItems[] = {
    { 0, FALSE, 80,  80, LVCFMT_LEFT, TEXT("Status") },
    { 0, TRUE, 120, 120, LVCFMT_LEFT, TEXT("Name") },
    { 0, TRUE, 240, 240, LVCFMT_LEFT, TEXT("Description") },
    { 0, TRUE,  80,  80, LVCFMT_LEFT, TEXT("Startup Type") },
    { 0, TRUE,  80,  80, LVCFMT_LEFT, TEXT("Log On As") },
    { 0, FALSE, 80,  80, LVCFMT_RIGHT, TEXT("Process Id") },
    { 0, FALSE,  0,  80, LVCFMT_RIGHT, TEXT("# Handles") }
};

#define NUMTBBUTTONS  (sizeof(tbButtons) / sizeof(tbButtons[0]))
#define NUMLVITEMS    (sizeof(lvItems) / sizeof(lvItems[0]))

#define APSVCMGR_CLASS      TEXT("APSVCMGR")

/* Display only Started/Paused status */
#define STAT_STARTED        TEXT("Started")
#define STAT_PAUSED         TEXT("Paused")
#define STAT_STOPPED        TEXT("Stopped")
#define STAT_DISABLED       TEXT("Disabled")
#define STAT_NONE           TEXT("")
#define STAT_SYSTEM         L"LocalSystem"

#define START_AUTO           L"Automatic"
#define START_MANUAL         L"Manual"
#define START_DISABLED       L"Disabled"
#define START_BOOT           L"Boot"
#define START_SYSTEM         L"SystemInit"

#define SFILT_KEY            L"Filters"
#define SFILT_INAME          L"IncludeName"
#define SFILT_XNAME          L"ExcludeName"
#define SFILT_ISIMG          L"IncludeImage"
#define SFILT_XSIMG          L"ExcludeImage"

#define DISPLAY_KEY          "Display"
#define DISPLAY_STATE        "GuiState"
#define EMPTY_PASSWORD       L"               "
#define EXPORT_TITLE         "Export Services list"
#define EXPORT_EXTFILT       "Comma separated (*.csv)\0*.csv\0Tab Delimited (*.tab)\0*.txt\0All Files(*.*)\0*.*\0"


#define WM_TIMER_REFRESH    10
#define WM_USER_LREFRESH    WM_USER + 1
#define UPD_FAST            10000
#define UPD_SLOW            60000

static int _toolbarHeight = 0;
static int _statbarHeight = 0;
static int _currentTimer  = IDMV_UPAUSED;
static int _currentLitem  = 0;
static int _listviewCols  = 0;
static int _sortOrder     = 1;
static int _sortColumn    = 1;
static LPAPXSERVENTRY     _currentEntry = NULL;

static LPWSTR   _filterIname    = NULL;
static LPWSTR   _filterXname    = NULL;
static LPWSTR   _filterIimage   = NULL;
static LPWSTR   _filterXimage   = NULL;
static LPSTR    _exportFilename = NULL;
static DWORD    _exportIndex    = 0;
/* Main application pool */
APXHANDLE hPool     = NULL;
APXHANDLE hService  = NULL;
APXHANDLE hRegistry = NULL;

/* Service browse callback declaration */
BOOL ServiceCallback(APXHANDLE hObject, UINT uMsg,
                     WPARAM wParam, LPARAM lParam);

int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    WCHAR  szA[1024] = {0};
    WCHAR  szB[1024] = {0};
    int    rc;
    ListView_GetItemTextW(hWndList, (INT)lParam1, _sortColumn, szA, 1023);
    ListView_GetItemTextW(hWndList, (INT)lParam2, _sortColumn, szB, 1023);
    rc = lstrcmpW(szA, szB);

    return rc * _sortOrder;
}

void DestroyListView()
{
    int i, x;
    
    x = ListView_GetItemCount(hWndList);
    if (x > 0) {
        LV_ITEM lvI;
        lvI.mask = LVIF_PARAM;
        for (i = 0; i < x; i++) {
            lvI.iItem  = i;
            lvI.lParam = 0;
            ListView_GetItem(hWndList, &lvI);
            if (lvI.lParam) {
                LPAPXSERVENTRY lpEnt = (LPAPXSERVENTRY)lvI.lParam;
                apxFree(lpEnt->lpConfig);
                apxFree(lpEnt);
            }
        }
    }
    ListView_DeleteAllItems(hWndList);
    _currentLitem = 0;
}

BOOL ExportListView(LPCSTR szFileName, DWORD dwIndex)
{
    int i, j, x;
    CHAR szI[SIZ_DESLEN];
    HANDLE hFile;
    DWORD  dwWriten;
    LPSTR  szD;
    hFile = CreateFileA(szFileName, 
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (IS_INVALID_HANDLE(hFile))
        return FALSE;
    x = ListView_GetItemCount(hWndList);
    if (x > 0) {
        LV_ITEM lvI;
        lvI.mask = LVIF_PARAM;

        if (dwIndex == 1)
            szD = ";";
        else
            szD = "\t";
        WriteFile(hFile, "ServiceName", sizeof("ServiceName") - 1, &dwWriten, NULL);
        for (i = 1; i <  NUMLVITEMS; i++) {
            if (lvItems[i].iWidth) {
#ifdef _UNICODE
                WideToAscii(lvItems[i].szLabel, szI);
#else
                lstrcpyA(szI, lvItems[i].szLabel);
#endif
                WriteFile(hFile, szD, 1, &dwWriten, NULL);
                WriteFile(hFile, szI, lstrlenA(szI), &dwWriten, NULL);
            }
        }
        WriteFile(hFile, "\r\n", 2, &dwWriten, NULL);           
        for (i = 0; i < x; i++) {
            lvI.iItem  = i;
            lvI.lParam = 0;
            ListView_GetItem(hWndList, &lvI);
            if (lvI.lParam) {
                LPAPXSERVENTRY lpEnt = (LPAPXSERVENTRY)lvI.lParam;
                WideToAscii(lpEnt->szServiceName, szI);
                WriteFile(hFile, szI, lstrlenA(szI), &dwWriten, NULL);
                WriteFile(hFile, szD, 1, &dwWriten, NULL);
                
            }
            for (j = 1; j < _listviewCols - 1; j++) {
                szI[0] = '\0';
                ListView_GetItemTextA(hWndList, i, j, szI, SIZ_DESMAX);
                if (szI[0])
                    WriteFile(hFile, szI, lstrlenA(szI), &dwWriten, NULL);
                WriteFile(hFile, szD, 1, &dwWriten, NULL);
            }
            szI[0] = '\0';
            ListView_GetItemTextA(hWndList, i, j, szI, SIZ_DESMAX);
            if (szI[0])
                WriteFile(hFile, szI, lstrlenA(szI), &dwWriten, NULL);
            WriteFile(hFile, "\r\n", 2, &dwWriten, NULL);           
        }
    }  
    CloseHandle(hFile);
    return TRUE;
}

void RefreshServices(int iRefresh)
{
    int prev = _currentLitem;
    _currentLitem = 0;
    apxServiceBrowse(hService, _filterIname, _filterIimage,
                     _filterXname, _filterXimage, WM_USER+1+iRefresh,
                     ServiceCallback, NULL);
    ListView_SortItemsEx(hWndList, CompareFunc, NULL);
    if (prev > _currentLitem) {
        DestroyListView();
        apxServiceBrowse(hService, _filterIname, _filterIimage,
                         _filterXname, _filterXimage, WM_USER+1,
                         ServiceCallback, NULL);
        ListView_SortItemsEx(hWndList, CompareFunc, NULL);
    }
    if ((lvHitItem = ListView_GetSelectionMark(hWndList)) >= 0) {
#if 0
        /* Ensure that the selected is visible */
        ListView_EnsureVisible(hWndList,lvHitItem, FALSE);
#endif
        PostMessage(hWndList, WM_USER_LREFRESH, 0, 0);
    }
}

void RestoreRefreshTimer()
{
    if (_currentTimer == IDMV_UFAST)
        SetTimer(_gui_store->hMainWnd, WM_TIMER_REFRESH, UPD_FAST, NULL); 
    else if (_currentTimer == IDMV_USLOW)
        SetTimer(_gui_store->hMainWnd, WM_TIMER_REFRESH, UPD_SLOW, NULL); 
    else
        KillTimer(_gui_store->hMainWnd, WM_TIMER_REFRESH);
}

/* wParam progress dialog handle
 */
static BOOL __startServiceCallback(APXHANDLE hObject, UINT uMsg,
                                   WPARAM wParam, LPARAM lParam)
{
    HWND hDlg = (HWND)hObject;
    APXHANDLE hSrv;
    switch (uMsg) {
        case WM_USER+1:
            hDlg = (HWND)lParam;
            hSrv = apxCreateService(hPool, SC_MANAGER_CONNECT, FALSE);
            if (!hSrv) {
                EndDialog(hDlg, IDOK);
                PostMessage(_gui_store->hMainWnd, WM_COMMAND,
                            MAKEWPARAM(IDMS_REFRESH, 0), 0);
                return FALSE;
            }
            if (!apxServiceOpen(hSrv, _currentEntry->szServiceName,
                                GENERIC_READ | GENERIC_EXECUTE)) {
                apxCloseHandle(hSrv);
                EndDialog(hDlg, IDOK);
                PostMessage(_gui_store->hMainWnd, WM_COMMAND,
                            MAKEWPARAM(IDMS_REFRESH, 0), 0);
                return FALSE;
            }
            if (apxServiceControl(hSrv, SERVICE_CONTROL_CONTINUE, WM_USER+2,
                                  __startServiceCallback, hDlg)) {
                _currentEntry->stServiceStatus.dwCurrentState = SERVICE_RUNNING;
                _currentEntry->stStatusProcess.dwCurrentState = SERVICE_RUNNING;
                                
            }
            apxCloseHandle(hSrv);
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
    APXHANDLE hSrv;
    switch (uMsg) {
        case WM_USER+1:
            hDlg = (HWND)lParam;
            hSrv = apxCreateService(hPool, SC_MANAGER_CONNECT, FALSE);
            if (!hSrv)
                return FALSE;
            if (!apxServiceOpen(hSrv, _currentEntry->szServiceName,
                                GENERIC_READ | GENERIC_EXECUTE)) {
                apxCloseHandle(hSrv);
                return FALSE;
            }
            if (apxServiceControl(hSrv, SERVICE_CONTROL_STOP, WM_USER+2,
                                  __stopServiceCallback, hDlg)) {
            }
            apxCloseHandle(hSrv);
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
    APXHANDLE hSrv;
    switch (uMsg) {
        case WM_USER+1:
            hDlg = (HWND)lParam;
            hSrv = apxCreateService(hPool, SC_MANAGER_CONNECT, FALSE);
            if (!hSrv)
                return FALSE;
            if (!apxServiceOpen(hSrv, _currentEntry->szServiceName,
                                GENERIC_READ | GENERIC_EXECUTE)) {
                apxCloseHandle(hSrv);
                return FALSE;
            }
            /* TODO: use 128 as controll code */
            if (apxServiceControl(hSrv, 128, WM_USER+2,
                                  __restartServiceCallback, hDlg)) {
                                
            }
            apxCloseHandle(hSrv);
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
    APXHANDLE hSrv;
    switch (uMsg) {
        case WM_USER+1:
            hDlg = (HWND)lParam;
            hSrv = apxCreateService(hPool, SC_MANAGER_CONNECT, FALSE);
            if (!hSrv)
                return FALSE;
            if (!apxServiceOpen(hSrv, _currentEntry->szServiceName,
                                GENERIC_READ | GENERIC_EXECUTE)) {
                apxCloseHandle(hSrv);
                return FALSE;
            }
            if (apxServiceControl(hSrv, SERVICE_CONTROL_PAUSE, WM_USER+2,
                                  __pauseServiceCallback, hDlg)) {
            }
            apxCloseHandle(hSrv);
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

LRESULT CALLBACK ListViewMainSubclass(HWND hWnd, UINT uMsg, WPARAM wParam,
                                      LPARAM lParam)
{
    static POINTS  mouseClick;

    HMENU         hMenu = NULL;
    HMENU         hLoad = NULL;
    HMENU         hMain = NULL;
    switch (uMsg) {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            mouseClick = MAKEPOINTS(lParam);
            lvLastHit.pt.x = mouseClick.x;
            lvLastHit.pt.y = mouseClick.y;
        case WM_USER_LREFRESH:
            lvHitItem = ListView_HitTest(hWndList, &lvLastHit);
            _currentEntry = NULL;
            hMain = GetMenu(_gui_store->hMainWnd);
            CallWindowProc(ListViewWinMain, hWnd, uMsg, wParam, lParam);
            EnableMenuItem(hMain, IDMS_START, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hMain, IDMS_STOP, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hMain, IDMS_PAUSE, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hMain, IDMS_RESTART, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hMain, IDAM_DELETE, MF_BYCOMMAND | MF_GRAYED);
            SendMessage(hWndToolbar, TB_ENABLEBUTTON, IDMS_START, FALSE);
            SendMessage(hWndToolbar, TB_ENABLEBUTTON, IDMS_STOP, FALSE);
            SendMessage(hWndToolbar, TB_ENABLEBUTTON, IDMS_PAUSE, FALSE);
            SendMessage(hWndToolbar, TB_ENABLEBUTTON, IDMS_RESTART, FALSE);
            SendMessage(hWndToolbar, TB_ENABLEBUTTON, IDMS_PROPERTIES, FALSE);
            SendMessage(hWndToolbar, TB_ENABLEBUTTON, IDAM_DELETE, FALSE);
            EnableMenuItem(hMain, IDMS_PROPERTIES, MF_BYCOMMAND | MF_GRAYED);

            if (lvHitItem >= 0) {
                LV_ITEM lvI;
                lvI.mask = LVIF_PARAM;
                lvI.iItem  = lvHitItem;
                lvI.lParam = 0;
                ListView_GetItem(hWndList, &lvI);
                if (lvI.lParam) {
                    _currentEntry = (LPAPXSERVENTRY)lvI.lParam;
                    SendMessageW(hWndStatus, WM_SETTEXT, 0, (LPARAM)_currentEntry->szServiceDescription);
                    if (uMsg == WM_RBUTTONDOWN) {
                        hLoad = LoadMenu(_gui_store->hInstance,
                                         MAKEINTRESOURCE(IDM_POPUPMENU));

                        hMenu = GetSubMenu(hLoad, 0);
                    }
                    switch (_currentEntry->stServiceStatus.dwCurrentState) {
                        case SERVICE_RUNNING:
                            if (_currentEntry->stServiceStatus.dwControlsAccepted & SERVICE_ACCEPT_STOP) {
                                SendMessage(hWndToolbar, TB_ENABLEBUTTON, IDMS_STOP, TRUE);
                                EnableMenuItem(hMain, IDMS_STOP, MF_BYCOMMAND | MF_ENABLED);
                                if (hMenu)
                                    EnableMenuItem(hMenu, IDMS_STOP, MF_BYCOMMAND | MF_ENABLED);
                            }
                            if (_currentEntry->stServiceStatus.dwControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE) {
                                SendMessage(hWndToolbar, TB_ENABLEBUTTON, IDMS_PAUSE, TRUE);
                                SendMessage(hWndToolbar, TB_ENABLEBUTTON, IDMS_RESTART, TRUE);
                                EnableMenuItem(hMain, IDMS_PAUSE, MF_BYCOMMAND | MF_ENABLED);
                                EnableMenuItem(hMain, IDMS_RESTART, MF_BYCOMMAND | MF_ENABLED);
                                if (hMenu) {
                                    EnableMenuItem(hMenu, IDMS_PAUSE, MF_BYCOMMAND | MF_ENABLED);
                                    EnableMenuItem(hMenu, IDMS_RESTART, MF_BYCOMMAND | MF_ENABLED);
                                }
                            }                            
                        break;
                        case SERVICE_PAUSED:
                            SendMessage(hWndToolbar, TB_ENABLEBUTTON, IDMS_RESTART, TRUE);
                            SendMessage(hWndToolbar, TB_ENABLEBUTTON, IDMS_START, TRUE);
                            EnableMenuItem(hMain, IDMS_RESTART, MF_BYCOMMAND | MF_ENABLED);
                            EnableMenuItem(hMain, IDMS_START, MF_BYCOMMAND | MF_ENABLED);
                            if (hMenu) {
                                EnableMenuItem(hMenu, IDMS_RESTART, MF_BYCOMMAND | MF_ENABLED);
                                EnableMenuItem(hMenu, IDMS_START, MF_BYCOMMAND | MF_ENABLED);
                            }
                        break;
                        case SERVICE_STOPPED:
                            if (_currentEntry->lpConfig->dwStartType != SERVICE_DISABLED) {
                                SendMessage(hWndToolbar, TB_ENABLEBUTTON, IDMS_START, TRUE);
                                EnableMenuItem(hMain, IDMS_START, MF_BYCOMMAND | MF_ENABLED);
                                if (hMenu)
                                    EnableMenuItem(hMenu, IDMS_START, MF_BYCOMMAND | MF_ENABLED);
                            }
                        break;
                        default:

                        break;
                    }
                }
                SendMessage(hWndToolbar, TB_ENABLEBUTTON, IDMS_PROPERTIES, TRUE);
                EnableMenuItem(hMain, IDMS_PROPERTIES, MF_BYCOMMAND | MF_ENABLED);
                SendMessage(hWndToolbar, TB_ENABLEBUTTON, IDAM_DELETE, TRUE);
                EnableMenuItem(hMain, IDAM_DELETE, MF_BYCOMMAND | MF_ENABLED);
                SetMenuDefaultItem(GetSubMenu(hMain, 1), IDMS_PROPERTIES, FALSE);

                if (hMenu) {
                    POINT pt; 
                    EnableMenuItem(hMenu, IDMS_PROPERTIES, MF_BYCOMMAND | MF_ENABLED);
                    SetMenuDefaultItem(hMenu, IDMS_PROPERTIES, FALSE);

                    GetCursorPos(&pt); 
                    TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
                        pt.x, pt.y, 0, _gui_store->hMainWnd, NULL);
                    DestroyMenu(hLoad);
                    hMenu = NULL;
                    hLoad = NULL;
                }
                /* Post the Properties message to main window */
                if (uMsg == WM_LBUTTONDBLCLK) {
                    PostMessage(_gui_store->hMainWnd, WM_COMMAND, 
                                MAKEWPARAM(IDMS_PROPERTIES, 0), 0);
                }
            }
            else {
                SetMenuDefaultItem(GetSubMenu(hMain, 1), -1, FALSE);
                SendMessageW(hWndStatus, WM_SETTEXT, 0, (LPARAM)(L""));
            }
            return TRUE;
        break;
    }
    return CallWindowProc(ListViewWinMain, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK ListViewHeadSubclass(HWND hWnd, UINT uMsg, WPARAM wParam,
                                      LPARAM lParam)
{
    DWORD     i, n = 0;

    switch (uMsg) {
        case WM_LBUTTONUP:
            CallWindowProc(ListViewWinHead, hWnd, uMsg, wParam, lParam);
            for (i = 0; i < NUMLVITEMS; i++) {
                if (lvItems[i].iWidth) {
                    lvItems[i].iWidth = ListView_GetColumnWidth(hWndList, n++);
                }
            }
            return TRUE;
        break;

    }
    return CallWindowProc(ListViewWinHead, hWnd, uMsg, wParam, lParam);
}

HWND CreateServiceList(HWND hWndParent)
{
    HWND        hWndList;
    RECT        rc;
    LV_COLUMN   lvC;
    DWORD       i;
    HBITMAP     hStatBmp;
    HIMAGELIST  hStatImg;
    DWORD       dwStyle;

    dwStyle =   WS_TABSTOP | 
                WS_CHILD | 
                WS_VISIBLE |
                LVS_SINGLESEL |
                LVS_REPORT;

    GetWindowRect(hWndToolbar, &rc);
    _toolbarHeight = rc.bottom - rc.top;
    GetWindowRect(hWndStatus, &rc);
    _statbarHeight = rc.bottom - rc.top;
    /* Get the size and position of the parent window. */
    GetClientRect(hWndParent, &rc);

    /* Create the list view window */
    hWndList = CreateWindowEx(WS_EX_CLIENTEDGE,
                              WC_LISTVIEW, TEXT(""), 
                              dwStyle,
                              0, _toolbarHeight, 
                              rc.right - rc.left, 
                              rc.bottom - rc.top - _toolbarHeight - _statbarHeight,
                              hWndParent,
                              (HMENU)IDC_LISTVIEW,
                              _gui_store->hInstance, NULL);
    if (hWndList == NULL)
        return NULL;
    GetClientRect(hWndList, &rc);
    hStatImg = ImageList_Create(16, 16, ILC_COLOR4, 0, 16);
    hStatBmp = LoadImage(GetModuleHandleA(NULL), MAKEINTRESOURCE(IDB_SSTATUS),
                         IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT);

    ImageList_Add(hStatImg, hStatBmp, NULL);
    DeleteObject(hStatBmp);

    ListView_SetImageList(hWndList, hStatImg, LVSIL_SMALL);
    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    _listviewCols = 0;
    for (i = 0; i < NUMLVITEMS; i++) {
        lvC.iSubItem = i;
        lvC.cx       = lvItems[i].iWidth;
        lvC.pszText  = lvItems[i].szLabel;
        lvC.fmt      = lvItems[i].iFmt;
        if (lvItems[i].iWidth > 0) {
            ListView_InsertColumn(hWndList, i, &lvC ); 
            _listviewCols++;
        }
    }
#ifdef LVS_EX_FULLROWSELECT
    ListView_SetExtendedListViewStyleEx(hWndList, 0,
                LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
#endif 
    hWndListHdr = ListView_GetHeader(hWndList);
    /* Sub-class */
    ListViewWinMain = (WNDPROC)((SIZE_T)SetWindowLong(hWndList, GWL_WNDPROC, 
                                                      (LONG)((SIZE_T)ListViewMainSubclass))); 

    ListViewWinHead = (WNDPROC)((SIZE_T)SetWindowLong(hWndListHdr, GWL_WNDPROC, 
                                                      (LONG)((SIZE_T)ListViewHeadSubclass))); 


    return hWndList;
}

static DWORD  _propertyChanged;
/* Service property pages */
void CALLBACK __propertyCallback(HWND hwndPropSheet, UINT uMsg, LPARAM lParam)
{
    switch(uMsg) {
        case PSCB_PRECREATE:       
           {
                LPDLGTEMPLATE  lpTemplate = (LPDLGTEMPLATE)lParam;
                if (!(lpTemplate->style & WS_SYSMENU))
                    lpTemplate->style |= WS_SYSMENU;
                apxCenterWindow(hwndPropSheet, _gui_store->hMainWnd);
                _propertyChanged = 0;
            }
        break;
        case PSCB_INITIALIZED:
        break;

    }
}

BOOL __generalPropertySave(HWND hDlg) 
{
    APXHANDLE hSrv;
    WCHAR szN[256];
    WCHAR szD[256];
    DWORD dwStartType = SERVICE_NO_CHANGE;
    int i;

    if (!(TST_BIT_FLAG(_propertyChanged, 1)))
        return TRUE;
    CLR_BIT_FLAG(_propertyChanged, 1);

    if (!(hSrv = apxCreateService(hPool, SC_MANAGER_CREATE_SERVICE, FALSE)))
        return FALSE;
    if (!apxServiceOpen(hSrv, _currentEntry->szServiceName, SERVICE_ALL_ACCESS)) {
        apxCloseHandle(hSrv);
        return FALSE;
    }
    GetDlgItemTextW(hDlg, IDC_PPSGDISP, szN, 255);
    GetDlgItemTextW(hDlg, IDC_PPSGDESC, szD, 1023);
    i = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PPSGCMBST));
    if (i == 0)
        dwStartType = SERVICE_AUTO_START;
    else if (i == 1)
        dwStartType = SERVICE_DEMAND_START;
    else if (i == 2)
        dwStartType = SERVICE_DISABLED;
    apxServiceSetNames(hSrv, NULL, szN, szD, NULL, NULL);
    apxServiceSetOptions(hSrv, SERVICE_NO_CHANGE, dwStartType, SERVICE_NO_CHANGE);

    apxCloseHandle(hSrv);
    if (!(TST_BIT_FLAG(_propertyChanged, 2)))
        PostMessage(_gui_store->hMainWnd, WM_COMMAND, MAKEWPARAM(IDMS_REFRESH, 0), 0);

    return TRUE;
}

BOOL __generalLogonSave(HWND hDlg) 
{
    APXHANDLE hSrv;
    WCHAR szU[64];
    WCHAR szP[64];
    WCHAR szC[64];
    DWORD dwStartType = SERVICE_NO_CHANGE;

    if (!(TST_BIT_FLAG(_propertyChanged, 2)))
        return TRUE;
    CLR_BIT_FLAG(_propertyChanged, 2);

    if (!(hSrv = apxCreateService(hPool, SC_MANAGER_CREATE_SERVICE, FALSE)))
        return FALSE;
    if (!apxServiceOpen(hSrv, _currentEntry->szServiceName, SERVICE_ALL_ACCESS)) {
        apxCloseHandle(hSrv);
        return FALSE;
    }
    GetDlgItemTextW(hDlg, IDC_PPSLUSER,  szU, 63);
    GetDlgItemTextW(hDlg, IDC_PPSLPASS,  szP, 63);
    GetDlgItemTextW(hDlg, IDC_PPSLCPASS, szC, 63);
    
    if (lstrlenW(szU) && lstrcmpiW(szU, STAT_SYSTEM)) {
        if (szP[0] != L' ' &&  szC[0] != L' ' && !lstrcmpW(szP, szC))
            apxServiceSetNames(hSrv, NULL, NULL, NULL, szU, szP);
        else {
            MessageBoxW(hDlg, apxLoadResourceW(IDS_VALIDPASS, 0),
                        apxLoadResourceW(IDS_APPLICATION, 1),
                        MB_OK | MB_ICONSTOP);
            apxCloseHandle(hSrv);
            return FALSE;
        }
    }             
    else {
        if (IsDlgButtonChecked(hDlg, IDC_PPSLID) == BST_CHECKED)
            apxServiceSetOptions(hSrv,
                _currentEntry->stServiceStatus.dwServiceType | SERVICE_INTERACTIVE_PROCESS,
                SERVICE_NO_CHANGE, SERVICE_NO_CHANGE);
        else
            apxServiceSetOptions(hSrv,
                _currentEntry->stServiceStatus.dwServiceType & ~SERVICE_INTERACTIVE_PROCESS,
                SERVICE_NO_CHANGE, SERVICE_NO_CHANGE);
    }
    apxCloseHandle(hSrv);
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
                SetDlgItemText(hDlg, IDC_PPSGSTATUS, STAT_DISABLED);
            }
            else
                SetDlgItemText(hDlg, IDC_PPSGSTATUS, STAT_STOPPED);
        break;
        default:
        break;
    }
}

LRESULT CALLBACK __generalProperty(HWND hDlg,
                                   UINT uMessage,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
    LPPSHNOTIFY lpShn;
    WCHAR       szBuf[1024]; 

    switch (uMessage) { 
        case WM_INITDIALOG: 
            {
                PROPSHEETPAGE *lpPage = (PROPSHEETPAGE *)lParam;
                LPAPXSERVENTRY lpService;
                BOOL           bLocalSystem = TRUE;
                
                lpService = ( LPAPXSERVENTRY)lpPage->lParam;
                SendMessage(GetDlgItem(hDlg, IDC_PPSGDISP), EM_LIMITTEXT, 255, 0);
                SendMessage(GetDlgItem(hDlg, IDC_PPSGDESC), EM_LIMITTEXT, 1023, 0);

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
                        GetDlgItemTextW(hDlg, IDC_PPSGDISP, szBuf, 255);
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
                        GetDlgItemTextW(hDlg, IDC_PPSGDESC, szBuf, 1023);
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
                        SetWindowLong(hDlg, DWL_MSGRESULT,
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
    WCHAR       szBuf[1024]; 
    switch (uMessage) { 
        case WM_INITDIALOG:
            {
                BOOL           bAccount = FALSE;
                
                SendMessage(GetDlgItem(hDlg, IDC_PPSLUSER), EM_LIMITTEXT, 63, 0);
                SendMessage(GetDlgItem(hDlg, IDC_PPSLPASS), EM_LIMITTEXT, 63, 0);
                SendMessage(GetDlgItem(hDlg, IDC_PPSLCPASS), EM_LIMITTEXT, 63, 0);
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
                        GetDlgItemTextW(hDlg, IDC_PPSLUSER, szBuf, 63);
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
                        WCHAR szP[64];
                        WCHAR szC[64];
                        GetDlgItemTextW(hDlg, IDC_PPSLPASS, szP, 63);
                        GetDlgItemTextW(hDlg, IDC_PPSLCPASS, szC, 63);
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
                        SetWindowLong(hDlg, DWL_MSGRESULT,
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

INT_PTR ShowServiceProperties(HWND hWnd)
{
    PROPSHEETPAGEW   psP[5];
    PROPSHEETHEADERW psH; 
    WCHAR           szT[1024] = {0};

    __initPpage(&psP[0], IDD_PROPPAGE_SGENERAL, IDS_PPGENERAL, 
                __generalProperty);
    __initPpage(&psP[1], IDD_PROPPAGE_LOGON, IDS_PPLOGON, 
                __logonProperty);

    if (_currentEntry && _currentEntry->lpConfig)
        lstrcpyW(szT, _currentEntry->lpConfig->lpDisplayName);
    else
        return (INT_PTR)0;
    lstrcatW(szT, L" Service Properties");

    psH.dwSize           = sizeof(PROPSHEETHEADER);
    psH.dwFlags          = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_USECALLBACK | PSH_NOCONTEXTHELP;
    psH.hwndParent       = hWnd;
    psH.hInstance        = _gui_store->hInstance;
    psH.pszIcon          = MAKEINTRESOURCEW(IDI_MAINICON);
    psH.pszCaption       = szT;
    psH.nPages           = 2;
    psH.ppsp             = (LPCPROPSHEETPAGEW) &psP;
    psH.pfnCallback      = (PFNPROPSHEETCALLBACK)__propertyCallback;
    psH.nStartPage       = 0;   

    return PropertySheetW(&psH);
}

static LRESULT CALLBACK __filtersDlgProc(HWND hDlg, UINT uMsg,
                                         WPARAM wParam, LPARAM lParam)
{

    switch (uMsg) {
        case WM_INITDIALOG:
            apxCenterWindow(hDlg, _gui_store->hMainWnd);
            if (_filterIname)
                SetDlgItemTextW(hDlg, IDC_FINAME, _filterIname);
            if (_filterIimage)
                SetDlgItemTextW(hDlg, IDC_FISIMG, _filterIimage);
            if (_filterXname)
                SetDlgItemTextW(hDlg, IDC_FXNAME, _filterXname);
            if (_filterXimage)
                SetDlgItemTextW(hDlg, IDC_FXSIMG, _filterXimage);
            return TRUE;
        break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    {
                        apxFree(_filterIname);
                        _filterIname = apxGetDlgTextW(hPool, hDlg, IDC_FINAME);
                        apxFree(_filterXname);
                        _filterXname = apxGetDlgTextW(hPool, hDlg, IDC_FXNAME);
                        apxFree(_filterIimage);
                        _filterIimage = apxGetDlgTextW(hPool, hDlg, IDC_FISIMG);
                        apxFree(_filterXimage);
                        _filterXimage = apxGetDlgTextW(hPool, hDlg, IDC_FXSIMG);
                        PostMessage(_gui_store->hMainWnd, WM_COMMAND,
                                    MAKEWPARAM(IDMS_REFRESH, 0), 0);
                    }
                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
        break;
    }
    return FALSE;
} 

void EditServiceFilters(HWND hWnd)
{

   DialogBox(_gui_store->hInstance,
              MAKEINTRESOURCE(IDD_FILTER),
              hWnd,
              (DLGPROC)__filtersDlgProc);

}

static LRESULT CALLBACK __selectColsDlgProc(HWND hDlg, UINT uMsg,
                                            WPARAM wParam, LPARAM lParam)
{

    switch (uMsg) {
        case WM_INITDIALOG:
            apxCenterWindow(hDlg, _gui_store->hMainWnd);
            CheckDlgButton(hDlg, IDC_CCOL1, lvItems[1].iWidth != 0);
            CheckDlgButton(hDlg, IDC_CCOL2, lvItems[2].iWidth != 0);
            CheckDlgButton(hDlg, IDC_CCOL3, lvItems[3].iWidth != 0);
            CheckDlgButton(hDlg, IDC_CCOL4, lvItems[4].iWidth != 0);
            CheckDlgButton(hDlg, IDC_CCOL5, lvItems[5].iWidth != 0);

            return TRUE;
        break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    if (IsDlgButtonChecked(hDlg, IDC_CCOL1) != BST_CHECKED)
                        lvItems[1].iWidth = 0;
                    else if (lvItems[1].iWidth == 0)
                        lvItems[1].iWidth = lvItems[1].iDefault;
                    if (IsDlgButtonChecked(hDlg, IDC_CCOL2) != BST_CHECKED)
                        lvItems[2].iWidth = 0;
                    else if (lvItems[2].iWidth == 0)
                        lvItems[2].iWidth = lvItems[2].iDefault;
                    if (IsDlgButtonChecked(hDlg, IDC_CCOL3) != BST_CHECKED)
                        lvItems[3].iWidth = 0;
                    else if (lvItems[3].iWidth == 0)
                        lvItems[3].iWidth = lvItems[3].iDefault;
                    if (IsDlgButtonChecked(hDlg, IDC_CCOL4) != BST_CHECKED)
                        lvItems[4].iWidth = 0;
                    else if (lvItems[4].iWidth == 0)
                        lvItems[4].iWidth = lvItems[4].iDefault;
                    if (IsDlgButtonChecked(hDlg, IDC_CCOL5) != BST_CHECKED)
                        lvItems[5].iWidth = 0;
                    else if (lvItems[5].iWidth == 0)
                        lvItems[5].iWidth = lvItems[5].iDefault;
                    CloseWindow(hWndList);
                    hWndList = CreateServiceList(_gui_store->hMainWnd); 

                    PostMessage(_gui_store->hMainWnd, WM_COMMAND,
                                MAKEWPARAM(IDMS_REFRESH, 0), 0);

                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
        break;
    }
    return FALSE;
} 

void SelectDisplayColumns(HWND hWnd)
{

   DialogBox(_gui_store->hInstance,
              MAKEINTRESOURCE(IDD_SELCOL),
              hWnd,
              (DLGPROC)__selectColsDlgProc);

}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg,
                             WPARAM wParam, LPARAM lParam) 
{
    RECT rcP;
    LPNMHDR     lpNmHdr; 

    switch (uMsg) {
        case WM_CREATE:

            hWndToolbar = CreateToolbarEx(hWnd, 
                                          WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT,  
                                          IDC_TOOLBAR, NUMTOOLBUTTONS,
                                          _gui_store->hInstance, IDB_TOOLBAR,
                                          (LPCTBBUTTON)&tbButtons,
                                          NUMTBBUTTONS, 16,16,16,16, sizeof(TBBUTTON)); 
            hWndStatus = CreateStatusWindow(0x0800 /* SBT_TOOLTIPS */
                                            | WS_CHILD | WS_VISIBLE,
                                            TEXT(""), hWnd, IDC_STATBAR); 

            hWndList = CreateServiceList(hWnd); 
            RefreshServices(0);
            CheckMenuRadioItem(GetMenu(hWnd), IDMV_UFAST, IDMV_UPAUSED,
                               _currentTimer, MF_BYCOMMAND);
            RestoreRefreshTimer();
            return FALSE;
        break;
        case WM_SETFOCUS:
            SetFocus(hWndList);
            return FALSE;
        break;
        case WM_SIZE:
            MoveWindow(hWndToolbar, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
            MoveWindow(hWndList, 0, _toolbarHeight, LOWORD(lParam),
                       HIWORD(lParam) -  _toolbarHeight - _statbarHeight, TRUE);
            MoveWindow(hWndStatus, 0, HIWORD(lParam) - _statbarHeight, 
                       LOWORD(lParam), HIWORD(lParam), TRUE);
            GetWindowRect(hWnd, &rcP);
            _gui_store->stState.rcPosition.top    = rcP.top;
            _gui_store->stState.rcPosition.left   = rcP.left;
            _gui_store->stState.rcPosition.right  = ABS(rcP.right - rcP.left);
            _gui_store->stState.rcPosition.bottom = ABS(rcP.bottom - rcP.top);
            if (wParam == SIZE_MAXIMIZED)
                _gui_store->stState.dwShow = SW_SHOWMAXIMIZED;
            else
                _gui_store->stState.dwShow = SW_SHOW;

            return FALSE;
        break;   
        case WM_TIMER: 
            if (wParam == WM_TIMER_REFRESH) {
                KillTimer(hWnd, WM_TIMER_REFRESH);
                RefreshServices(1);
                RestoreRefreshTimer();
            }
        break;
        case WM_NOTIFY: 
            lpNmHdr = (NMHDR FAR *)lParam;
            switch (lpNmHdr->code) { 
                case TTN_GETDISPINFO: 
                    { 
                        LPTOOLTIPTEXT lpttt; 
                        lpttt = (LPTOOLTIPTEXT)lParam; 
                        lpttt->hinst = _gui_store->hInstance; 
                        /* Specify the resource identifier of the descriptive 
                         * text for the given button. 
                         */
                        lpttt->lpszText = MAKEINTRESOURCE(lpttt->hdr.idFrom);
                    }
                break; 
                case LVN_COLUMNCLICK :
                    {
                        LPNMLISTVIEW pLvi = (LPNMLISTVIEW)lParam;
                        /* Find the real column */
                        if (pLvi->iSubItem >= 0) {
                            INT i, x = 0;
                            for (i = 0; i < NUMLVITEMS; i++) {
                                if (lvItems[i].iWidth && x < pLvi->iSubItem)
                                    ++x;
                                else
                                    break;
                            }
                            if (lvItems[x].bSortable) {
                                if (_sortColumn == x)
                                    _sortOrder *= (-1);
                                else {
                                    _sortColumn = x;
                                    _sortOrder  = 1;
                                }
                                ListView_SortItemsEx(hWndList, CompareFunc, NULL);
                                if ((lvHitItem = ListView_GetSelectionMark(hWndList)) >= 0) {
                                    ListView_EnsureVisible(hWndList,lvHitItem, FALSE);
                                }
                            }
                        }
                    }
                break;
                /* 
                 * Process other notifications here. 
                 */ 
                default: 
                break; 
            } 
        break;
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case IDMS_REFRESH:
                    KillTimer(hWnd, WM_TIMER_REFRESH);
                    RefreshServices(1);
                    RestoreRefreshTimer();
                break;
                case IDAM_TRY:
                    apxManageTryIconA(hWnd, NIM_ADD, NULL,
                                      apxLoadResourceA(IDS_APPLICATION, 0),
                                      NULL);

                    KillTimer(hWnd, WM_TIMER_REFRESH);
                    ShowWindow(hWnd, SW_HIDE);
                break;
                case IDAM_EXIT:
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                break;
                case IDMH_ABOUT:
                    apxAboutBox(hWnd);
                break;
                case IDMS_START:
                    apxProgressBox(hWnd, apxLoadResource(IDS_HSSTART, 0),
                                   _currentEntry->lpConfig->lpDisplayName,
                                   __startServiceCallback, NULL);
                break;
                case IDMS_STOP:
                    apxProgressBox(hWnd, apxLoadResource(IDS_HSSTOP, 0),
                                   _currentEntry->lpConfig->lpDisplayName,
                                   __stopServiceCallback, NULL);
                break;
                case IDMS_PAUSE:
                    apxProgressBox(hWnd, apxLoadResource(IDS_HSPAUSE, 0),
                                   _currentEntry->lpConfig->lpDisplayName,
                                   __pauseServiceCallback, NULL);
                break;
                case IDMS_RESTART:
                    apxProgressBox(hWnd, apxLoadResource(IDS_HSRESTART, 0),
                                   _currentEntry->lpConfig->lpDisplayName,
                                   __restartServiceCallback, NULL);
                break;
                case IDMS_PROPERTIES:
                    KillTimer(hWnd, WM_TIMER_REFRESH);
                    ShowServiceProperties(hWnd);
                    RestoreRefreshTimer();
                break;
                case IDMV_UFAST:
                case IDMV_USLOW:
                case IDMV_UPAUSED:
                    _currentTimer = LOWORD(wParam);
                    CheckMenuRadioItem(GetMenu(hWnd), IDMV_UFAST, IDMV_UPAUSED,
                                       _currentTimer, MF_BYCOMMAND);
                    RestoreRefreshTimer();
                break;
                case IDAM_DELETE:
                    {
                        TCHAR szT[1024+128];
#ifndef _UNICODE
                        CHAR dName[1024];
                        WideToAscii(_currentEntry->lpConfig->lpDisplayName, dName);
#endif
                        if (!_currentEntry || !_currentEntry->lpConfig)
                            return FALSE;

                        wsprintf(szT, apxLoadResource(IDS_DELSERVICET, 0),
#ifdef _UNICODE
                                 _currentEntry->lpConfig->lpDisplayName);
#else
                                 dName);
#endif
                        KillTimer(hWnd, WM_TIMER_REFRESH);
                        if (apxYesNoMessage(apxLoadResource(IDS_DELSERVICEC, 0),
                                            szT, TRUE)) {
                            APXHANDLE hSrv;
                            if ((hSrv = apxCreateService(hPool, SC_MANAGER_CONNECT, FALSE))) {
                                if (apxServiceOpen(hSrv, _currentEntry->szServiceName,
                                                   SERVICE_ALL_ACCESS)) {
                                    apxServiceDelete(hSrv);
                                    RefreshServices(1);
                                }
                                apxCloseHandle(hSrv);
                            }
                        }
                        RestoreRefreshTimer();
                    }
                break;
                case IDMV_FILTER:
                    EditServiceFilters(hWnd);
                break;
                case IDMV_SELECTCOLUMNS:
                    SelectDisplayColumns(hWnd);
                break;
                case IDAM_SAVE:
                    if (_exportFilename) {
                        ExportListView(_exportFilename, _exportIndex);
                        return FALSE;
                    }
                case IDAM_SAVEAS:
                    if (_exportFilename)
                        apxFree(_exportFilename);
                    _exportFilename = apxGetFileNameA(hWnd, EXPORT_TITLE,
                                                      EXPORT_EXTFILT,
                                                      ".csv", NULL, FALSE,
                                                      &_exportIndex);
                    if (_exportFilename)
                        ExportListView(_exportFilename, _exportIndex);
                break;
                case IDAM_NEW:
                    /* TODO: New service wizard */
                    MessageBox(hWnd, apxLoadResource(IDS_NOTIMPLEMENTED, 0),
                               apxLoadResource(IDS_APPLICATION, 1),
                               MB_OK | MB_ICONINFORMATION);
                break;
                default:
                    return DefWindowProc(hWnd, uMsg, wParam, lParam); 
                break;
            }
        break;
        case WM_TRAYMESSAGE:
            switch(lParam) {
                case WM_LBUTTONDBLCLK:
                    ShowWindow(hWnd, SW_SHOW);
                    apxManageTryIconA(hWnd, NIM_DELETE, NULL, NULL, NULL);
                    RestoreRefreshTimer();
                break;
            }
        break;
        case WM_MOUSEWHEEL:
            {
                int nScroll;
                if ((SHORT)HIWORD(wParam) < 0)
                    nScroll = _gui_store->nWhellScroll;
                else
                    nScroll = _gui_store->nWhellScroll * (-1);
                ListView_Scroll(hWndList, 0, nScroll * 16);
            }
        break;
        case WM_QUIT:
            OutputDebugString(TEXT("WM_QUIT"));
            return DefWindowProc(hWnd, uMsg, wParam, lParam); 
        break;
        case WM_DESTROY:
            DestroyListView();
            apxManageTryIconA(hWnd, NIM_DELETE, NULL, NULL, NULL);
            OutputDebugString(TEXT("WM_DESTROY"));
            PostQuitMessage(0);
            return FALSE;
        break;
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam); 
        break;
    }

    return FALSE; 
}

/* Browse service callback */
BOOL ServiceCallback(APXHANDLE hObject, UINT uMsg,
                     WPARAM wParam, LPARAM lParam)
{
    LPAPXSERVENTRY  lpEnt = (LPAPXSERVENTRY)wParam;
    LV_ITEM         lvI;
    LV_ITEM         lvO;
    INT             row = 0x7FFFFFFF;
    WCHAR           szPid[16];
    LPAPXSERVENTRY  lpEntry = NULL;
    int             i;

    AplZeroMemory(&lvI, sizeof(LV_ITEM)); 
    lvI.mask        = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
    lvI.iItem       = 0x7FFFFFFF;
    switch (lpEnt->stServiceStatus.dwCurrentState) {
        case SERVICE_RUNNING:
            lvI.iImage      = 0;
            lvI.pszText     = STAT_STARTED;
        break;
        case SERVICE_STOPPED:
            lvI.iImage      = 1;
            lvI.pszText     = STAT_NONE;
        break;
        case SERVICE_PAUSED:
            lvI.iImage      = 2;
            lvI.pszText     = STAT_PAUSED;
        break;
        default:
            lvI.iImage      = 3;
            lvI.pszText     = STAT_NONE;
        break;
    }
    /* Search the items if this is a refresh callback */
    if (uMsg == (WM_USER + 2)) {
        for (i = 0; i < ListView_GetItemCount(hWndList); i++) {
            lvO.iItem = i;
            lvO.mask = LVIF_PARAM;
            ListView_GetItem(hWndList, &lvO);
            lpEntry = (LPAPXSERVENTRY)lvO.lParam;
            if (!lstrcmpW(lpEntry->szServiceName, lpEnt->szServiceName)) {
                row = lvO.iItem;
                /* release the old config */
                apxFree(lpEntry->lpConfig);
                break;
            }
            else
                lpEntry = NULL;
        }
    }
    if (row == 0x7FFFFFFF || !lpEntry)
        lpEntry = (LPAPXSERVENTRY) apxPoolAlloc(hPool, sizeof(APXSERVENTRY));
    AplCopyMemory(lpEntry, lpEnt, sizeof(APXSERVENTRY));
    lvI.lParam = (LPARAM)lpEntry;
    if (row == 0x7FFFFFFF)
        row = ListView_InsertItem(hWndList, &lvI);
    else {
        lvI.iItem = row;
        ListView_SetItem(hWndList, &lvI);
    }
    if (row == -1)
        return TRUE; 

    ListView_SetItemTextW(hWndList, row, 1, lpEnt->lpConfig->lpDisplayName); 
    i = 2;
    if (lvItems[2].iWidth > 0) {
        if (lpEnt->szServiceDescription)
            ListView_SetItemTextW(hWndList, row, i, lpEnt->szServiceDescription);
        i++;
    }
    if (lvItems[3].iWidth > 0) {
        if (lpEnt->dwStart == SERVICE_DEMAND_START) {
            ListView_SetItemTextW(hWndList, row, i, START_MANUAL);
        }
        else if (lpEnt->dwStart == SERVICE_AUTO_START) {
            ListView_SetItemTextW(hWndList, row, i, START_AUTO);
        }
        else if (lpEnt->dwStart == SERVICE_DISABLED) {
            ListView_SetItemTextW(hWndList, row, i, START_DISABLED);
        }
        else if (lpEnt->dwStart == SERVICE_BOOT_START) {
            ListView_SetItemTextW(hWndList, row, i, START_BOOT);
        }        
        else if (lpEnt->dwStart == SERVICE_SYSTEM_START) {
            ListView_SetItemTextW(hWndList, row, i, START_SYSTEM);
        }
        i++;
    }
    if (lvItems[4].iWidth > 0) {
        ListView_SetItemTextW(hWndList, row, i++, *lpEnt->szObjectName ? 
                              lpEnt->szObjectName : STAT_SYSTEM);
    }
    if (lvItems[5].iWidth > 0) {
        if (lpEnt->stStatusProcess.dwProcessId) {
            wsprintfW(szPid, L"%d", lpEnt->stStatusProcess.dwProcessId);
            ListView_SetItemTextW(hWndList, row, i, szPid);
        }
        i++;
    }
    _currentLitem++;
    return TRUE;
}

static BOOL loadConfiguration()
{
    DWORD dwSize, i;
    /* Load the GUI State first */    
    if (IS_INVALID_HANDLE(hRegistry))
        return FALSE;

    dwSize = sizeof(APXGUISTATE);
    if (apxRegistryGetBinaryA(hRegistry, APXREG_USER, DISPLAY_KEY, DISPLAY_STATE,
                              (LPBYTE)(&(_gui_store->stState)), &dwSize)) {
        _gui_store->stStartupInfo.wShowWindow = (WORD)_gui_store->stState.dwShow;
        if (_gui_store->stStartupInfo.wShowWindow == SW_MAXIMIZE) {
            _gui_store->stState.rcPosition.top    = CW_USEDEFAULT;
            _gui_store->stState.rcPosition.left   = CW_USEDEFAULT;
            _gui_store->stState.rcPosition.bottom = CW_USEDEFAULT;
            _gui_store->stState.rcPosition.right  = CW_USEDEFAULT;
        }
        if (_gui_store->stState.nColumnWidth[0] > 50)
            lvItems[0].iWidth = _gui_store->stState.nColumnWidth[0];
        if (_gui_store->stState.nColumnWidth[1] > 50)
            lvItems[1].iWidth = _gui_store->stState.nColumnWidth[1];

        for (i = 2; i < NUMLVITEMS; i++) {
            lvItems[i].iWidth = _gui_store->stState.nColumnWidth[i];
        }
        _sortColumn   = _gui_store->stState.nUser[0];
        _sortOrder    = _gui_store->stState.nUser[1];
        _currentTimer = _gui_store->stState.nUser[2];
    }
    _filterIname  = apxRegistryGetStringW(hRegistry, APXREG_USER,
                                          SFILT_KEY, SFILT_INAME);
    _filterXname  = apxRegistryGetStringW(hRegistry, APXREG_USER,
                                          SFILT_KEY, SFILT_XNAME);
    _filterIimage = apxRegistryGetStringW(hRegistry, APXREG_USER,
                                          SFILT_KEY, SFILT_ISIMG);
    _filterIimage = apxRegistryGetStringW(hRegistry, APXREG_USER,
                                          SFILT_KEY, SFILT_XSIMG);
    /* try to load system defalut filters */
    if (!_filterIname)
        _filterIname  = apxRegistryGetStringW(hRegistry, APXREG_SOFTWARE,
                                              SFILT_KEY, SFILT_INAME);
    if (!_filterXname)
        _filterXname  = apxRegistryGetStringW(hRegistry, APXREG_SOFTWARE,
                                              SFILT_KEY, SFILT_XNAME);
    if (!_filterIimage)
        _filterIimage = apxRegistryGetStringW(hRegistry, APXREG_SOFTWARE,
                                              SFILT_KEY, SFILT_ISIMG);
    if (!_filterXimage)
        _filterXimage = apxRegistryGetStringW(hRegistry, APXREG_SOFTWARE,
                                              SFILT_KEY, SFILT_XSIMG);

    return TRUE;
}

#define REGSET_FILTER(str, key) \
    APXMACRO_BEGIN              \
    if (str && lstrlenW(str))   \
        apxRegistrySetStrW(hRegistry, APXREG_USER, SFILT_KEY, key, str);    \
    else                                                                    \
        apxRegistryDeleteW(hRegistry, APXREG_USER, SFILT_KEY, key);         \
    APXMACRO_END

static BOOL saveConfiguration()
{
    DWORD i;
    if (IS_INVALID_HANDLE(hRegistry))
        return FALSE;
    for (i = 0; i < NUMLVITEMS; i++) {
        _gui_store->stState.nColumnWidth[i] = lvItems[i].iWidth;
    }
    _gui_store->stState.nUser[0] = _sortColumn;
    _gui_store->stState.nUser[1] = _sortOrder;
    _gui_store->stState.nUser[2] = _currentTimer;

    apxRegistrySetBinaryA(hRegistry, APXREG_USER, DISPLAY_KEY, DISPLAY_STATE,
                          (LPBYTE)(&(_gui_store->stState)), sizeof(APXGUISTATE));
    REGSET_FILTER(_filterIname,  SFILT_INAME);
    REGSET_FILTER(_filterXname,  SFILT_XNAME);
    REGSET_FILTER(_filterIimage, SFILT_ISIMG);
    REGSET_FILTER(_filterXimage, SFILT_XSIMG);

    return TRUE;
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

    apxHandleManagerInitialize();
    hPool     = apxPoolCreate(NULL, 0);
    hService  = apxCreateService(hPool, GENERIC_ALL, TRUE);

    _gui_store = apxGuiInitialize(MainWndProc, APSVCMGR_CLASS);

    if (!_gui_store) {
        goto cleanup;
    }
    hRegistry = apxCreateRegistry(hPool, KEY_ALL_ACCESS, NULL,
                                  apxLoadResource(IDS_APPLICATION, 0),
                                  APXREG_USER);
    loadConfiguration();
    _gui_store->hMainWnd = CreateWindow(_gui_store->szWndClass,
                                        apxLoadResource(IDS_APPLICATION, 0),
                                        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                        _gui_store->stState.rcPosition.left,
                                        _gui_store->stState.rcPosition.top,
                                        _gui_store->stState.rcPosition.right,
                                        _gui_store->stState.rcPosition.bottom,
                                        NULL, NULL,
                                        _gui_store->hInstance,
                                        NULL);
    if (!_gui_store->hMainWnd) {
        goto cleanup;
    }

    ShowWindow(_gui_store->hMainWnd, _gui_store->stStartupInfo.wShowWindow);
    UpdateWindow(_gui_store->hMainWnd); 
    SetMenuDefaultItem(GetMenu(_gui_store->hMainWnd), IDMS_PROPERTIES, FALSE);

    while (GetMessage(&msg, NULL, 0, 0))  {
        if(!TranslateAccelerator(_gui_store->hMainWnd,
                                 _gui_store->hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    saveConfiguration();

cleanup:
    apxFree(_filterIname);
    apxFree(_filterXname);
    apxFree(_filterIimage);
    apxFree(_filterXimage);
    apxFree(_exportFilename);
    apxCloseHandle(hService);
    apxHandleManagerDestroy();
    ExitProcess(0);
    return 0;
}
