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

#define BALLON_TIMEOUT  1000
/* Offset for listview dots */
#define DOTOFFSET       0

static HMODULE      _st_sys_riched;
static APXGUISTORE  _st_sys_gui;
static HIMAGELIST   _st_sel_users_il = NULL;
static WNDPROC      _st_sel_users_lvm;

typedef struct PROGRESS_DLGPARAM {
    LPCTSTR szHead;
    LPCWSTR szText;
    LPVOID  cbData;
    LPAPXFNCALLBACK fnCb;
    HANDLE  hThread;
    HWND    hDialog;
} PROGRESS_DLGPARAM, *LPPROGRESS_DLGPARAM;

APXLVITEM lvUsers[] = {
    { 0, FALSE, 180, 180, LVCFMT_LEFT, TEXT("User") },
    { 0, TRUE,  180, 180, LVCFMT_LEFT, TEXT("Full Name") },
    { 0, TRUE,  235, 235, LVCFMT_LEFT, TEXT("Comment") }
};


#define NUMLVUSERS    (sizeof(lvUsers) / sizeof(lvUsers[0]))

static UINT __getWhellScrollLines()
{
    HWND hdlMsWheel;
    UINT ucNumLines = 3;  /* 3 is the default */
    UINT uiMsh_MsgScrollLines;
   
    APX_OSLEVEL os = apxGetOsLevel();
    /* In Windows 9x & Windows NT 3.51, query MSWheel for the
     * number of scroll lines. In Windows NT 4.0 and later,
     * use SystemParametersInfo.
     */
    if (os < APX_WINVER_NT_4) {
        hdlMsWheel = FindWindow(MSH_WHEELMODULE_CLASS, 
                                MSH_WHEELMODULE_TITLE);
        if (hdlMsWheel) {
            uiMsh_MsgScrollLines = RegisterWindowMessage(MSH_SCROLL_LINES);
            if (uiMsh_MsgScrollLines)
                ucNumLines = (int)SendMessage(hdlMsWheel,
                                              uiMsh_MsgScrollLines, 
                                              0, 0);
        }
    }
    else {
        SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0,
                             &ucNumLines, 0);
    }
    return ucNumLines;
}

/* Initialize the Gui
 */
LPAPXGUISTORE apxGuiInitialize(WNDPROC lpfnWndProc, LPCTSTR szAppName)
{
    INITCOMMONCONTROLSEX stCmn;
    WNDCLASSEX wcex;

    _st_sys_gui.hInstance = GetModuleHandleA(NULL);
    GetStartupInfo(&_st_sys_gui.stStartupInfo);

    lstrcpy(_st_sys_gui.szWndClass, szAppName);
    lstrcat(_st_sys_gui.szWndClass, TEXT("_CLASS"));

    /* Single instance or general application mutex */
    lstrcpy(_st_sys_gui.szWndMutex, szAppName);
    lstrcat(_st_sys_gui.szWndMutex, TEXT("_MUTEX"));


    stCmn.dwSize = sizeof(INITCOMMONCONTROLSEX);
    stCmn.dwICC = ICC_WIN95_CLASSES | ICC_USEREX_CLASSES | ICC_COOL_CLASSES |
                  ICC_INTERNET_CLASSES | ICC_PAGESCROLLER_CLASS | ICC_BAR_CLASSES;
                  
    InitCommonControlsEx(&stCmn);

    _st_sys_riched      = LoadLibraryA("RICHED32.DLL"); 
    _st_sys_gui.hIconSm = LoadImage(_st_sys_gui.hInstance, MAKEINTRESOURCE(IDI_MAINICON),
                                     IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR); 
    _st_sys_gui.hIcon   = LoadImage(_st_sys_gui.hInstance, MAKEINTRESOURCE(IDI_MAINICON),
                                     IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR); 
    _st_sys_gui.hIconHg = LoadImage(_st_sys_gui.hInstance, MAKEINTRESOURCE(IDI_MAINICON),
                                     IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR); 
    _st_sys_gui.hAccel  = LoadAccelerators(_st_sys_gui.hInstance,
                                            MAKEINTRESOURCE(IDC_APPLICATION));
    _st_sys_gui.stState.rcPosition.left   = CW_USEDEFAULT;
    _st_sys_gui.stState.rcPosition.top    = CW_USEDEFAULT;
    _st_sys_gui.stState.rcPosition.right  = CW_USEDEFAULT;
    _st_sys_gui.stState.rcPosition.bottom = CW_USEDEFAULT;

    _st_sys_gui.nWhellScroll = __getWhellScrollLines();

    wcex.cbSize = sizeof(WNDCLASSEX); 

    wcex.style          = 0;
    wcex.lpfnWndProc    = lpfnWndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = _st_sys_gui.hInstance;
    wcex.hIcon          = _st_sys_gui.hIcon;
    wcex.hIconSm        = _st_sys_gui.hIconSm;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_INACTIVEBORDER+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_APPLICATION);
    wcex.lpszClassName  = _st_sys_gui.szWndClass;

    if (RegisterClassEx(&wcex)) {
        return &_st_sys_gui;
    }
    else
        return NULL;
}


BOOL apxCenterWindow(HWND hwndChild, HWND hwndParent)
{
    RECT    rChild, rParent, rWorkArea;
    int     wChild, hChild, wParent, hParent;
    int     xNew, yNew;
    BOOL    bResult;

    /* Get the Height and Width of the child window */
    GetWindowRect(hwndChild, &rChild);
    wChild = rChild.right - rChild.left;
    hChild = rChild.bottom - rChild.top;
    if (hwndParent == NULL)
        hwndParent = GetDesktopWindow();
    /* Get the Height and Width of the parent window */
    GetWindowRect(hwndParent, &rParent);
    wParent = rParent.right - rParent.left;
    hParent = rParent.bottom - rParent.top;

    if (wParent < wChild && hParent < hChild) {
        GetWindowRect(GetDesktopWindow(), &rParent);
        wParent = rParent.right - rParent.left;
        hParent = rParent.bottom - rParent.top;
    }
    /* Get the limits of the 'workarea' */
    bResult = SystemParametersInfo(SPI_GETWORKAREA, sizeof(RECT),
        &rWorkArea, 0);
    if (!bResult) {
        rWorkArea.left = rWorkArea.top = 0;
        rWorkArea.right = GetSystemMetrics(SM_CXSCREEN);
        rWorkArea.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    /* Calculate new X position, then adjust for workarea */
    xNew = rParent.left + ((wParent - wChild) /2);
    if (xNew < rWorkArea.left)
        xNew = rWorkArea.left;
    else if ((xNew+wChild) > rWorkArea.right)
        xNew = rWorkArea.right - wChild;

    /* Calculate new Y position, then adjust for workarea */
    yNew = rParent.top  + ((hParent - hChild) /2);
    if (yNew < rWorkArea.top)
        yNew = rWorkArea.top;
    else if ((yNew+hChild) > rWorkArea.bottom)
        yNew = rWorkArea.bottom - hChild;

    /* Set it, and return */
    return SetWindowPos(hwndChild, NULL, xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

/***************************************************************************
 * Function: LoadRcString
 *
 * Purpose: Loads a resource string from string table and returns a pointer
 *          to the string.
 *
 * Parameters: wID - resource string id
 *
 */

/** Load the resource string with the ID given, and return a
 * pointer to it.  Notice that the buffer is common memory so
 * the string must be used before this call is made a second time.
 */

LPSTR apxLoadResourceA(UINT wID, UINT nBuf)

{
    static CHAR szBuf[4][SIZ_BUFLEN];
    if (nBuf > 4)
        return "";
    if (LoadStringA(_st_sys_gui.hInstance,wID ,szBuf[nBuf], SIZ_BUFMAX) > 0)
        return szBuf[nBuf];
    else
        return "";
}

LPWSTR apxLoadResourceW(UINT wID, UINT nBuf)

{
    static WCHAR szBuf[4][SIZ_BUFLEN];
    if (nBuf > 4)
        return L"";
    if (LoadStringW(_st_sys_gui.hInstance,wID ,szBuf[nBuf], SIZ_BUFMAX) > 0)
        return szBuf[nBuf];
    else
        return L"";
}

/* Add the item to the Try popup menu
 */
void apxAppendMenuItem(HMENU hMenu, UINT idMenu, LPCTSTR szName,
                       BOOL bDefault, BOOL bEnabled)
{
    MENUITEMINFO miI;
    
    AplZeroMemory(&miI, sizeof(MENUITEMINFO));
    miI.cbSize = sizeof(MENUITEMINFO);
    miI.fMask  = MIIM_TYPE | MIIM_STATE;
    if (szName && lstrlen(szName)) {
        miI.fMask |= MIIM_ID;
        miI.fType = MFT_STRING;
        miI.wID   = idMenu;
        if (bDefault)
            miI.fState = MFS_DEFAULT;
        if (!bEnabled)
            miI.fState |= MFS_DISABLED;
        miI.dwTypeData = (LPTSTR)szName;
    }
    else {
        miI.fType = MFT_SEPARATOR;
    }
    InsertMenuItem(hMenu, idMenu, FALSE, &miI);
}

/* Add the item to the Try popup menu
 */
void apxAppendMenuItemBmp(HMENU hMenu, UINT idMenu, LPCTSTR szName)
{
    MENUITEMINFO miI;
    HBITMAP hBmp;

    hBmp = LoadImage(_st_sys_gui.hInstance, szName,
                     IMAGE_BITMAP, 0, 0,
                     LR_CREATEDIBSECTION | LR_SHARED);

    AplZeroMemory(&miI, sizeof(MENUITEMINFO));
    miI.cbSize = sizeof(MENUITEMINFO);
    miI.fMask  = MIIM_BITMAP | MFT_MENUBARBREAK;

    miI.hbmpItem = hBmp;
    InsertMenuItem(hMenu, idMenu, FALSE, &miI);
}

/* Try icon helper
 * Add/Change/Delete icon from the windows try.
 */
void apxManageTryIconA(HWND hWnd, DWORD dwMessage, LPCSTR szInfoTitle,
                       LPCSTR szInfo, HICON hIcon)
{
    static BOOL inTry = FALSE;
    NOTIFYICONDATAA nId;
    AplZeroMemory(&nId, sizeof(NOTIFYICONDATAA));

    nId.cbSize = sizeof(NOTIFYICONDATAA);
    nId.hWnd = hWnd;
    nId.uID = 0xFF;
    nId.uCallbackMessage = WM_TRAYMESSAGE;
    nId.uFlags =  NIF_MESSAGE;
    
    if (dwMessage == NIM_ADD && inTry)
        return;
    if (dwMessage != NIM_DELETE) {
        nId.uFlags |= NIF_ICON;
        if (! szInfoTitle) {
            nId.uFlags |= NIF_TIP;
            lstrcpynA(nId.szTip, szInfo, 63);
        }
        else if (szInfo) {
            nId.uFlags |= NIF_INFO;
            lstrcpynA(nId.szInfo, szInfo, 255);
            lstrcpynA(nId.szInfoTitle, szInfoTitle, 63);
            nId.dwInfoFlags = NIIF_INFO;
            nId.uTimeout    = BALLON_TIMEOUT;
        }

        nId.hIcon = hIcon ? hIcon : _st_sys_gui.hIconSm;
        inTry = TRUE;
    }
    else
        inTry = FALSE;

    Shell_NotifyIconA(dwMessage, &nId);
} 

void apxManageTryIconW(HWND hWnd, DWORD dwMessage, LPCWSTR szInfoTitle,
                       LPCWSTR szInfo, HICON hIcon)
{
    
    NOTIFYICONDATAW nId;
    AplZeroMemory(&nId, sizeof(NOTIFYICONDATAW));

    nId.cbSize = sizeof(NOTIFYICONDATAW);
    nId.hWnd = hWnd;
    nId.uID = 0xFF;
    nId.uCallbackMessage = WM_TRAYMESSAGE;
    nId.uFlags =  NIF_MESSAGE;

    if (dwMessage != NIM_DELETE) {
        nId.uFlags |= NIF_ICON;
        if (! szInfoTitle) {
            nId.uFlags |= NIF_TIP;
            lstrcpynW(nId.szTip, szInfo, 63);
        }
        else if (szInfo) {
            nId.uFlags |= NIF_INFO;
            lstrcpynW(nId.szInfo, szInfo, 255);
            lstrcpynW(nId.szInfoTitle, szInfoTitle, 63);
            nId.dwInfoFlags = NIIF_INFO;
            nId.uTimeout    = BALLON_TIMEOUT;
        }
        nId.hIcon = hIcon ? hIcon : _st_sys_gui.hIconSm;
    }

    Shell_NotifyIconW(dwMessage, &nId);
} 

static void __apxShellAbout(HWND hWnd)
{
    TCHAR szApplication[512];

    wsprintf(szApplication , TEXT("About - %s#Windows"), 
             apxLoadResource(IDS_APPLICATION, 0));

    ShellAbout(hWnd, szApplication,
               apxLoadResource(IDS_APPDESCRIPTION, 1),
               _st_sys_gui.hIconHg);
}


static LRESULT CALLBACK __apxAboutDlgProc(HWND hDlg, UINT uMsg,
                                          WPARAM wParam, LPARAM lParam)
{
    static  HWND  hRich = NULL;
    static  POINT ptScroll;
    HRSRC   hRsrc;
    HGLOBAL hGlob;
    LPSTR   szTxt;

    switch (uMsg) {
        case WM_INITDIALOG:
            apxCenterWindow(hDlg, _st_sys_gui.hMainWnd);
            hRich = GetDlgItem(hDlg, IDC_LICENSE);
            hRsrc = FindResource(GetModuleHandleA(NULL), MAKEINTRESOURCE(IDR_LICENSE),
                                 TEXT("RTF"));
            hGlob = LoadResource(GetModuleHandleA(NULL), hRsrc);
            szTxt = (LPSTR)LockResource(hGlob);
            
            SendMessageA(hRich, WM_SETTEXT, 0, (LPARAM)szTxt);
            SetDlgItemText(hDlg, IDC_ABOUTAPP, apxLoadResource(IDS_APPFULLNAME, 0));
            ptScroll.x = 0;
            ptScroll.y = 0;
            return TRUE;
        break;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
            else if (LOWORD(wParam) == IAB_SYSINF)
                __apxShellAbout(hDlg);
        break;
        case WM_MOUSEWHEEL:
            {
                int nScroll, nLines;
                if ((SHORT)HIWORD(wParam) < 0)
                    nScroll = _st_sys_gui.nWhellScroll;
                else
                    nScroll = _st_sys_gui.nWhellScroll * (-1);
                ptScroll.y += (nScroll * 11);
                if (ptScroll.y < 0)
                    ptScroll.y = 0;
                nLines = (int)SendMessage(hRich, EM_GETLINECOUNT, 0, 0) + 1;
                if (ptScroll.y / 11 > nLines)
                    ptScroll.y = nLines * 11;
                SendMessage(hRich, EM_SETSCROLLPOS, 0, (LPARAM)&ptScroll);
            }
        break;

    }
    return FALSE;
} 

void apxAboutBox(HWND hWnd)
{
    DialogBox(_st_sys_gui.hInstance,
              MAKEINTRESOURCE(IDD_ABOUTBOX),
              hWnd,
              (DLGPROC)__apxAboutDlgProc);
}

static DWORD WINAPI __apxProgressWorkerThread(LPVOID lpParameter)
{
    LPPROGRESS_DLGPARAM lpDlgParam = (LPPROGRESS_DLGPARAM)lpParameter;

    (*lpDlgParam->fnCb)(NULL, WM_USER+1, 0, (LPARAM)lpDlgParam->hDialog); 
    CloseHandle(lpDlgParam->hThread);
    ExitThread(0);
    return 0;
}

static LRESULT CALLBACK __apxProgressDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPPROGRESS_DLGPARAM lpDlgParam;
    DWORD dwId;
    switch (uMsg) {
        case WM_INITDIALOG:
            lpDlgParam = (LPPROGRESS_DLGPARAM)lParam;
            apxCenterWindow(hDlg, _st_sys_gui.hMainWnd);
            if (lpDlgParam && lpDlgParam->szHead && lpDlgParam->szText) {
                SetDlgItemText(hDlg, IDDP_HEAD, lpDlgParam->szHead);
                SetDlgItemTextW(hDlg, IDDP_TEXT, lpDlgParam->szText);
            }
            lpDlgParam->hDialog = hDlg;
            lpDlgParam->hThread = CreateThread(NULL, 0, __apxProgressWorkerThread,
                                               lpDlgParam, 0, &dwId); 
        break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
                break;
            }
        break;
        case WM_USER+1:
            SendMessage(GetDlgItem(hDlg, IDDP_PROGRESS), PBM_STEPIT, 0, 0);
        break;
    }
    return FALSE;
} 

int apxProgressBox(HWND hWnd, LPCTSTR szHeader,
                   LPCWSTR szText,
                   LPAPXFNCALLBACK fnProgressCallback,
                   LPVOID cbData)
{
    PROGRESS_DLGPARAM dlgParam;
    int rv;

    dlgParam.szHead  = szHeader;
    dlgParam.szText  = szText;
    dlgParam.cbData  = cbData;
    dlgParam.fnCb    = fnProgressCallback;
    dlgParam.hThread = NULL;
    rv =  (int)DialogBoxParam(_st_sys_gui.hInstance,
                              MAKEINTRESOURCE(IDD_PROGRESS),
                              hWnd,
                              (DLGPROC)__apxProgressDlgProc,
                              (LPARAM)&dlgParam);
    return rv;
}

BOOL apxYesNoMessage(LPCTSTR szTitle, LPCTSTR szMessage, BOOL bStop)
{
    UINT uType = MB_YESNO;
    int rv;

    if (bStop)
        uType |= MB_DEFBUTTON2 | MB_ICONEXCLAMATION;
    else
        uType |= MB_DEFBUTTON1 | MB_ICONQUESTION;

    rv = MessageBox(_st_sys_gui.hMainWnd, szMessage, szTitle, uType); 
    
    return (rv == IDYES);
}

LPWSTR apxGetDlgTextW(APXHANDLE hPool, HWND hDlg, int nIDDlgItem)
{
    DWORD l, n;
    LPWSTR szT = NULL;

    l = (DWORD)SendMessageW(GetDlgItem(hDlg, nIDDlgItem), WM_GETTEXTLENGTH, 0, 0);
    if (l > 0) {
        szT = apxPoolAlloc(hPool, (l + 1) * sizeof(WCHAR));
        n = GetDlgItemTextW(hDlg, nIDDlgItem, szT, l + 1);
        if (n == 0) {
            apxFree(szT);
            szT = NULL;
        }
    }
    return szT;
}

LPSTR apxGetDlgTextA(APXHANDLE hPool, HWND hDlg, int nIDDlgItem)
{
    DWORD l, n;
    LPSTR szT = NULL;

    l = (DWORD)SendMessageA(GetDlgItem(hDlg, nIDDlgItem), WM_GETTEXTLENGTH, 0, 0);
    if (l > 0) {
        szT = apxPoolAlloc(hPool, (l + 1));
        n = GetDlgItemTextA(hDlg, nIDDlgItem, szT, l + 1);
        if (n == 0) {
            apxFree(szT);
            szT = NULL;
        }
    }
    return szT;
}

/* Browse for folder dialog.
 */
LPSTR apxBrowseForFolderA(HWND hWnd, LPCSTR szTitle, LPCSTR szName)
{
    BROWSEINFOA  bi;
    LPITEMIDLIST il, ir;
    LPMALLOC     pMalloc;
    CHAR         szPath[MAX_PATH+1];
    LPSTR        rv = NULL;

    AplZeroMemory(&bi, sizeof(BROWSEINFOW));
    SHGetSpecialFolderLocation(hWnd, CSIDL_DRIVES, &il);
    bi.lpszTitle      = szTitle;
    bi.pszDisplayName = szPath;
    bi.hwndOwner      = hWnd;
    bi.ulFlags        = BIF_EDITBOX;
    bi.lpfn           = NULL;
    bi.lParam         = 0;
    bi.iImage         = 0;
    bi.pidlRoot       = il;
    
    if ((ir = SHBrowseForFolderA(&bi)) != NULL) {
        if (SHGetPathFromIDListA(ir, szPath))
            rv = apxStrdupA(szPath);
    }
    if (SHGetMalloc(&pMalloc)) {
        pMalloc->lpVtbl->Free(pMalloc, il);
        pMalloc->lpVtbl->Release(pMalloc);
    }

    return rv;    
} 

LPWSTR apxBrowseForFolderW(HWND hWnd, LPCWSTR szTitle, LPCWSTR szName)
{
    BROWSEINFOW  bi;
    LPITEMIDLIST il, ir;
    LPMALLOC     pMalloc;
    WCHAR        szPath[MAX_PATH+1];
    LPWSTR       rv = NULL;

    AplZeroMemory(&bi, sizeof(BROWSEINFOW));
    SHGetSpecialFolderLocation(hWnd, CSIDL_DRIVES, &il);
    bi.lpszTitle      = szTitle;
    bi.pszDisplayName = szPath;
    bi.hwndOwner      = hWnd;
    bi.ulFlags        = BIF_EDITBOX;
    bi.lpfn           = NULL;
    bi.lParam         = 0;
    bi.iImage         = 0;
    bi.pidlRoot       = il;
    
    if ((ir = SHBrowseForFolderW(&bi)) != NULL) {
        if (SHGetPathFromIDListW(ir, szPath))
            rv = apxStrdupW(szPath);
    }
    if (SHGetMalloc(&pMalloc)) {
        pMalloc->lpVtbl->Free(pMalloc, il);
        pMalloc->lpVtbl->Release(pMalloc);
    }

    return rv;    
} 

LPSTR apxGetFileNameA(HWND hWnd, LPCSTR szTitle, LPCSTR szFilter,
                      LPCSTR szDefExt, LPCSTR szDefPath, BOOL bOpenOrSave,
                      LPDWORD lpdwFindex)
{
    OPENFILENAMEA lpOf;
    CHAR    szFile[SIZ_BUFLEN];
    BOOL    rv;

    AplZeroMemory(&lpOf, sizeof(OPENFILENAMEA));
    szFile[0] = '\0';
    lpOf.lStructSize     = sizeof(OPENFILENAMEA); 
    lpOf.hwndOwner       = hWnd;
    lpOf.hInstance       = _st_sys_gui.hInstance;
    lpOf.lpstrTitle      = szTitle;
    lpOf.lpstrFilter     = szFilter;
    lpOf.lpstrDefExt     = szDefExt;
    lpOf.lpstrInitialDir = szDefPath;
    lpOf.lpstrFile       = szFile;
    lpOf.nMaxFile        = SIZ_BUFMAX;
    lpOf.Flags = OFN_LONGNAMES | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

    if (bOpenOrSave)
        rv = GetOpenFileNameA(&lpOf);
    else
        rv = GetSaveFileNameA(&lpOf);
    
    if (rv) {
        if (lpdwFindex)
            *lpdwFindex = lpOf.nFilterIndex;
        return apxStrdupA(szFile);
    }
    else
        return NULL;
}

LPWSTR apxGetFileNameW(HWND hWnd, LPCWSTR szTitle, LPCWSTR szFilter,
                       LPCWSTR szDefExt, LPCWSTR szDefPath, BOOL bOpenOrSave,
                       LPDWORD lpdwFindex)
{
    OPENFILENAMEW lpOf;
    WCHAR    szFile[SIZ_BUFLEN];
    BOOL    rv;

    AplZeroMemory(&lpOf, sizeof(OPENFILENAMEW));
    szFile[0] = L'\0';
    lpOf.lStructSize     = sizeof(OPENFILENAMEW); 
    lpOf.hwndOwner       = hWnd;
    lpOf.hInstance       = _st_sys_gui.hInstance;
    lpOf.lpstrTitle      = szTitle;
    lpOf.lpstrFilter     = szFilter;
    lpOf.lpstrDefExt     = szDefExt;
    lpOf.lpstrInitialDir = szDefPath;
    lpOf.lpstrFile       = szFile;
    lpOf.nMaxFile        = SIZ_BUFMAX;
    lpOf.Flags = OFN_LONGNAMES | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

    if (bOpenOrSave)
        rv = GetOpenFileNameW(&lpOf);
    else
        rv = GetSaveFileNameW(&lpOf);
    
    if (rv) {
        if (lpdwFindex)
            *lpdwFindex = lpOf.nFilterIndex;
        return apxStrdupW(szFile);
    }
    else
        return NULL;
}

static __apxSelectUserDlgResize(HWND hDlg, INT nWidth, INT nHeight)
{
    /* Combo box */
    MoveWindow(GetDlgItem(hDlg, IDSU_COMBO),
        70, 10, 
        nWidth - 70,
        120,
        TRUE);
    /* List Window */
    MoveWindow(GetDlgItem(hDlg, IDSU_LIST),
        0, 36, 
        nWidth,
        nHeight - 74,
        TRUE);

    /* Name label */
    MoveWindow(GetDlgItem(hDlg, IDSU_SELNAME),
        16,
        nHeight - 30, 
        50,
        24,
        TRUE);

    /* Edit Box */
    MoveWindow(GetDlgItem(hDlg, IDSU_SELECTED),
        70, 
        nHeight - 32, 
        nWidth - 300,
        24,
        TRUE);

    /* OK Button */
    MoveWindow(GetDlgItem(hDlg, IDOK),
        nWidth - 200,
        nHeight - 32, 
        80,
        24,
        TRUE);

    /* Cancel Button */
    MoveWindow(GetDlgItem(hDlg, IDCANCEL),
        nWidth - 110,
        nHeight - 32, 
        80,
        24,
        TRUE);


}

static LRESULT CALLBACK
__apxSelectUserCreateLvSubclass(HWND hWnd, UINT uMsg, WPARAM wParam,
                                LPARAM lParam)
{
    static POINTS  mouseClick;
    int iS;
    LVHITTESTINFO iHit;
    switch (uMsg) {
        case WM_LBUTTONDBLCLK:
            /* Call the original window proc */
            CallWindowProc(_st_sel_users_lvm, hWnd, uMsg, wParam, lParam);
            mouseClick = MAKEPOINTS(lParam);
            iHit.pt.x = mouseClick.x;
            iHit.pt.y = mouseClick.y;
            iS = ListView_HitTest(hWnd, &iHit);
            if (iS >= 0) {
                DWORD    i;
                WCHAR    szUser[SIZ_RESLEN] = L"";
                LPWSTR   szP;
                HWND     hCombo;
                HWND     hDlg = GetParent(hWnd);
                hCombo = GetDlgItem(hDlg, IDSU_COMBO);
                if ((i = ComboBox_GetCurSel(hCombo)) == 0) {
                    lstrcpyW(szUser, L".\\");
                }
                else {
                    COMBOBOXEXITEMW cbEi;
                    cbEi.mask  = CBEIF_TEXT;
                    cbEi.iItem = i;
                    cbEi.cchTextMax = SIZ_RESMAX;
                    cbEi.pszText    = szUser;
                    SendMessage(hCombo, CBEM_GETITEM, 0, (LPARAM)&cbEi);
                    lstrcatW(szUser, L"\\");
                }
                szP = &szUser[lstrlenW(szUser)];
                ListView_GetItemTextW(hWnd, iS, 0, szP, SIZ_RESMAX);
                if (*szP) {
                    SetDlgItemTextW(hDlg, IDSU_SELECTED, szUser);
                }
            }
            return TRUE;
       break;
    }
    return CallWindowProc(_st_sel_users_lvm, hWnd, uMsg, wParam, lParam);
}

#define SUMIN_WIDTH     600
#define SUMIN_HEIGHT    200

static void __apxSelectUserCreateLv(HWND hDlg)
{
    DWORD       i;
    LV_COLUMN   lvC;
    HBITMAP     hBmp;

    HWND        hList = GetDlgItem(hDlg, IDSU_LIST);

    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    for (i = 0; i < NUMLVUSERS; i++) {
        lvC.iSubItem = i;
        lvC.cx       = lvUsers[i].iWidth;
        lvC.pszText  = lvUsers[i].szLabel;
        lvC.fmt      = lvUsers[i].iFmt;
        ListView_InsertColumn(hList, i, &lvC ); 
    }
#ifdef LVS_EX_FULLROWSELECT
    ListView_SetExtendedListViewStyleEx(hList, 0,
    LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
#endif 
    _st_sel_users_il = ImageList_Create(16, 16, ILC_COLOR4, 0, 16);
    hBmp = LoadImage(GetModuleHandleA(NULL), MAKEINTRESOURCE(IDB_SUSERS),
                     IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT);

    ImageList_Add(_st_sel_users_il, hBmp, NULL);
    DeleteObject(hBmp);

    ListView_SetImageList(hList, _st_sel_users_il, LVSIL_SMALL);
    _st_sel_users_lvm = (WNDPROC)((SIZE_T)SetWindowLong(hList, GWLP_WNDPROC, 
                                                        (LONG)((SIZE_T)__apxSelectUserCreateLvSubclass))); 

}

static void __apxSelectUserPopulate(HWND hDlg, LPCWSTR szComputer)
{
    PNET_DISPLAY_USER   pBuff, p;
    INT             row = 0x7FFFFFFF;
    DWORD           res, dwRec, i = 0;

    HWND        hList = GetDlgItem(hDlg, IDSU_LIST);
   
    ListView_DeleteAllItems(hList);

    do { 
        res = NetQueryDisplayInformation(szComputer, 1, i, 1000, MAX_PREFERRED_LENGTH,
                                         &dwRec, &pBuff);
        if ((res == ERROR_SUCCESS) || (res == ERROR_MORE_DATA)) {
            p = pBuff;
            for (;dwRec > 0; dwRec--) {
                LV_ITEMW        lvI;
                AplZeroMemory(&lvI, sizeof(LV_ITEMW)); 
                lvI.mask        = LVIF_IMAGE | LVIF_TEXT;
                lvI.iItem       = 0x7FFFFFFF;
                lvI.pszText     = p->usri1_name;
                if (p->usri1_flags & UF_ACCOUNTDISABLE)
                    lvI.iImage = 5;
                else
                    lvI.iImage = 4;
                row = ListView_InsertItemW(hList, &lvI);
                if (row != -1) {
                    if (p->usri1_full_name) {
                        ListView_SetItemTextW(hList, row, 1,
                                              p->usri1_full_name);
                    }
                    if (p->usri1_comment) {
                        ListView_SetItemTextW(hList, row, 2,
                                              p->usri1_comment);
                    }
                }
                i = p->usri1_next_index;
                p++;
            }
            NetApiBufferFree(pBuff);
        }
    } while (res == ERROR_MORE_DATA);

}

static void __apxSelectUserCreateCbex(HWND hDlg)
{
    COMBOBOXEXITEMW     cbEi;
    LPBYTE              lpNetBuf;
    LPWKSTA_INFO_100    lpWksta;
    DWORD               res;
    HWND hCombo = GetDlgItem(hDlg, IDSU_COMBO);

#ifndef _UNICODE
    SendMessageW(hCombo, CBEM_SETUNICODEFORMAT, TRUE, 0);    
#endif

    cbEi.mask = CBEIF_TEXT | CBEIF_INDENT |
                CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;

    res = NetWkstaGetInfo(NULL, 101, (LPBYTE *)&lpWksta);
    if (res != ERROR_SUCCESS) {
        EnableWindow(hCombo, FALSE);
        return;
    }
    /* add localhost computer */
    cbEi.iItem      = 0;
    cbEi.pszText    = (LPWSTR)lpWksta->wki100_computername;
    cbEi.iIndent    = 0;
    cbEi.iImage         = 1;
    cbEi.iSelectedImage = 1;
    SendMessageW(hCombo, CBEM_INSERTITEMW, 0, (LPARAM)&cbEi);
    NetApiBufferFree(lpWksta);

    ComboBox_SetCurSel(hCombo, 0);
    res = NetGetDCName(NULL, NULL, &lpNetBuf);
    if ((res == ERROR_SUCCESS) || (res == ERROR_MORE_DATA)) {

        cbEi.iItem      = 1;
        cbEi.pszText    = ((LPWSTR)lpNetBuf) + 2;
        cbEi.iIndent    = 0;
        cbEi.iImage         = 0;
        cbEi.iSelectedImage = 0;
        SendMessageW(hCombo, CBEM_INSERTITEMW, 0, (LPARAM)&cbEi);
        EnableWindow(hCombo, TRUE);
        NetApiBufferFree(lpNetBuf);
    }
    else
        EnableWindow(hCombo, FALSE);
    
    SendMessageW(hCombo, CBEM_SETIMAGELIST, 0, (LPARAM)_st_sel_users_il);
}

static LRESULT CALLBACK __apxSelectUserDlgProc(HWND hDlg, UINT uMsg,
                                               WPARAM wParam, LPARAM lParam)
{
    static HWND     hList;
    static LPWSTR   lpUser;
    RECT r, *l;

    switch (uMsg) {
        case WM_INITDIALOG:
            /* Set the application icon */
            SetClassLong(hDlg, GCLP_HICON,
                         (LONG)(SIZE_T)LoadIcon(_st_sys_gui.hInstance,
                                        MAKEINTRESOURCE(IDI_MAINICON))); 
            apxCenterWindow(hDlg, _st_sys_gui.hMainWnd);
            hList = GetDlgItem(hDlg, IDSU_LIST); 
            __apxSelectUserCreateLv(hDlg);
            __apxSelectUserCreateCbex(hDlg);
            GetClientRect(hDlg, &r);
            /* Resize the controls */
            __apxSelectUserDlgResize(hDlg, r.right - r.left,
                                     r.bottom - r.top);
            lpUser = (LPWSTR)lParam;
            __apxSelectUserPopulate(hDlg, NULL);
            return TRUE;
        break;
        case WM_SIZING:
            l = (LPRECT)lParam;
            /* limit the window size */
            switch (wParam) {
                case WMSZ_BOTTOM:
                case WMSZ_BOTTOMRIGHT:
                    if ((l->bottom - l->top) < SUMIN_HEIGHT)
                        l->bottom = l->top + SUMIN_HEIGHT;
                    if ((l->right - l->left) < SUMIN_WIDTH)
                        l->right = l->left + SUMIN_WIDTH;
                break;
                case WMSZ_TOPLEFT:
                    if ((l->bottom - l->top) < SUMIN_HEIGHT)
                        l->top = l->bottom - SUMIN_HEIGHT;
                    if ((l->right - l->left) < SUMIN_WIDTH)
                        l->left = l->right - SUMIN_WIDTH;
                break;
                case WMSZ_TOP:
                case WMSZ_RIGHT:
                case WMSZ_TOPRIGHT:
                    if ((l->bottom - l->top) < SUMIN_HEIGHT)
                        l->top = l->bottom - SUMIN_HEIGHT;
                    if ((l->right - l->left) < SUMIN_WIDTH)
                        l->right = l->left + SUMIN_WIDTH;
                break;
                case WMSZ_BOTTOMLEFT:
                case WMSZ_LEFT:
                    if ((l->bottom - l->top) < SUMIN_HEIGHT)
                        l->bottom = l->top + SUMIN_HEIGHT;
                    if ((l->right - l->left) < SUMIN_WIDTH)
                        l->left = l->right - SUMIN_WIDTH;
                break;
            }
        break;
        case WM_SIZE:
            __apxSelectUserDlgResize(hDlg, LOWORD(lParam),
                                     HIWORD(lParam));
            GetClientRect(hDlg, &r);
            InvalidateRect(hDlg, &r, FALSE);
        break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
                case IDCANCEL:
                    /* Clear the user name buffer */
                    *lpUser = L'\0';
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
                break;
               case IDSU_SELECTED:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        /* enable OK button if there is a user */
                        GetDlgItemTextW(hDlg, IDSU_SELECTED, lpUser, SIZ_RESMAX);
                        if (lstrlenW(lpUser))
                            Button_Enable(GetDlgItem(hDlg, IDOK), TRUE);
                        else
                            Button_Enable(GetDlgItem(hDlg, IDOK), FALSE);
                    }
                break;
                case IDSU_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        COMBOBOXEXITEMW cbEi;
                        DWORD i;
                        WCHAR szServer[SIZ_RESLEN] = L"\\\\";
                        HWND  hCombo = GetDlgItem(hDlg, IDSU_COMBO);
                        if ((i = ComboBox_GetCurSel(hCombo)) >= 0) {
                            cbEi.mask  = CBEIF_TEXT;
                            cbEi.iItem = i;
                            cbEi.cchTextMax = SIZ_RESMAX;
                            cbEi.pszText    = &szServer[2];
                            SendMessageW(hCombo, CBEM_GETITEM, 0, (LPARAM)&cbEi);
                        }
                        if (szServer[2])
                            __apxSelectUserPopulate(hDlg, szServer);
                    }
                break;

            }
        break;
        case WM_MOUSEWHEEL:
            {
                int nScroll;
                if ((SHORT)HIWORD(wParam) < 0)
                    nScroll = _st_sys_gui.nWhellScroll;
                else
                    nScroll = _st_sys_gui.nWhellScroll * (-1);
            }
        break;

    }
    return FALSE;
} 


LPCWSTR apxDlgSelectUser(HWND hWnd, LPWSTR szUser)
{
    szUser[0] = L'\0';

    DialogBoxParam(_st_sys_gui.hInstance,
                   MAKEINTRESOURCE(IDD_SELUSER),
                   hWnd,
                   (DLGPROC)__apxSelectUserDlgProc,
                   (LPARAM)szUser);
    if (_st_sel_users_il)
        ImageList_Destroy(_st_sel_users_il);
    _st_sel_users_il = NULL;
    if (szUser[0] != '\0')
        return szUser;
    else
        return NULL;
}
