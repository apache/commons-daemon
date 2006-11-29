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
 
#ifndef _GUI_H_INCLUDED_
#define _GUI_H_INCLUDED_

__APXBEGIN_DECLS


#define IDC_STATIC              -1
#define IDC_APPLICATION         100
#define IDI_MAINICON            101
#define IDC_STATBAR             102
#define IDB_SUSERS              103

#define IDS_APPLICATION         150
#define IDS_APPDESCRIPTION      151
#define IDS_APPVERSION          152
#define IDS_APPCOPYRIGHT        153
#define IDS_APPFULLNAME         154

#define IDD_ABOUTBOX            250
#define IDC_LICENSE             251
#define IDR_LICENSE             252
#define IAB_SYSINF              253
#define IDC_ABOUTAPP            254

#define IDD_PROGRESS            260
#define IDDP_HEAD               261
#define IDDP_TEXT               262
#define IDDP_PROGRESS           263

#define IDD_SELUSER             270
#define IDSU_SELNAME            271
#define IDSU_SELECTED           272
#define IDSU_LIST               273
#define IDSU_COMBO              274


#define WM_TRAYMESSAGE          (WM_APP+1) 

#define SNDMSGW SendMessageW
#define SNDMSGA SendMessageA
#define ComboBox_AddStringW(hwndCtl, lpsz)       ((int)(DWORD)SNDMSGW((hwndCtl), CB_ADDSTRING, 0L, (LPARAM)(LPCWSTR)(lpsz)))

#define ListView_SetItemTextW(hwndLV, i, iSubItem_, pszText_) \
{ LV_ITEMW _ms_lvi;\
  _ms_lvi.iSubItem = iSubItem_;\
  _ms_lvi.pszText = pszText_;\
  SNDMSG((hwndLV), LVM_SETITEMTEXTW, (WPARAM)(i), (LPARAM)(LV_ITEMW *)&_ms_lvi);\
} ((void)0)

#define ListView_GetItemTextA(hwndLV, i, iSubItem_, pszText_, cchTextMax_) \
{ LV_ITEMA _ms_lvi;\
  _ms_lvi.iSubItem = iSubItem_;\
  _ms_lvi.cchTextMax = cchTextMax_;\
  _ms_lvi.pszText = pszText_;\
  SNDMSGA((hwndLV), LVM_GETITEMTEXTA, (WPARAM)(i), (LPARAM)(LV_ITEMA *)&_ms_lvi);\
}  ((void)0)

#define ListView_GetItemTextW(hwndLV, i, iSubItem_, pszText_, cchTextMax_) \
{ LV_ITEMW _ms_lvi;\
  _ms_lvi.iSubItem = iSubItem_;\
  _ms_lvi.cchTextMax = cchTextMax_;\
  _ms_lvi.pszText = pszText_;\
  SNDMSG((hwndLV), LVM_GETITEMTEXTW, (WPARAM)(i), (LPARAM)(LV_ITEMW *)&_ms_lvi);\
}  ((void)0)

#define ListView_InsertItemW(hwnd, pitem)   \
    (int)SNDMSGW((hwnd), LVM_INSERTITEMW, 0, (LPARAM)(const LV_ITEMW *)(pitem))

typedef struct APXLVITEM {
    INT         iPosition;
    BOOL        bSortable;
    INT         iWidth;
    INT         iDefault;
    INT         iFmt;
    LPTSTR      szLabel;
} APXLVITEM, *LPAPXLVITEM; 

typedef struct APXGUISTATE {
    DWORD       dwShow;
    RECT        rcPosition;
    COLORREF    bgColor;
    COLORREF    fgColor;
    COLORREF    txColor;
    INT         nColumnWidth[32];
    INT         nUser[8];
} APXGUISTATE, *LPAPXGUISTATE;

typedef struct APXGUISTORE {
    HANDLE  hInstance;
    HICON   hIcon;
    HICON   hIconSm;
    HICON   hIconHg;
    HWND    hMainWnd;
    HACCEL  hAccel;
    TCHAR   szWndClass[256];
    TCHAR   szWndMutex[256];
    APXGUISTATE stState;
    STARTUPINFO stStartupInfo;
    UINT        nWhellScroll;

} APXGUISTORE, *LPAPXGUISTORE;

LPAPXGUISTORE apxGuiInitialize(WNDPROC lpfnWndProc, LPCTSTR szAppName);

BOOL        apxCenterWindow(HWND hwndChild, HWND hwndParent);

LPSTR       apxLoadResourceA(UINT wID, UINT nBuf);
LPWSTR      apxLoadResourceW(UINT wID, UINT nBuf);


void        apxAppendMenuItem(HMENU hMenu, UINT idMenu, LPCTSTR szName,
                              BOOL bDefault, BOOL bEnabled);
void        apxAppendMenuItemBmp(HMENU hMenu, UINT idMenu, LPCTSTR szName);

void        apxManageTryIconA(HWND hWnd, DWORD dwMessage, LPCSTR szInfoTitle,
                              LPCSTR szInfo, HICON hIcon);
void        apxManageTryIconW(HWND hWnd, DWORD dwMessage, LPCWSTR szInfoTitle,
                              LPCWSTR szInfo, HICON hIcon);
#ifdef _UNICODE
#define apxLoadResource apxLoadResourceW
#else
#define apxLoadResource apxLoadResourceA
#endif

void        apxAboutBox(HWND hWnd);
int         apxProgressBox(HWND hWnd, LPCTSTR szHeader,
                           LPCWSTR szText,
                           LPAPXFNCALLBACK fnProgressCallback,
                           LPVOID cbData);
BOOL        apxYesNoMessage(LPCTSTR szTitle, LPCTSTR szMessage, BOOL bStop);

BOOL        apxCalcStringEllipsis(HDC hDC, LPTSTR  szString, 
                                  int cchMax, UINT uColWidth);
LPWSTR      apxGetDlgTextW(APXHANDLE hPool, HWND hDlg, int nIDDlgItem);
LPSTR       apxGetDlgTextA(APXHANDLE hPool, HWND hDlg, int nIDDlgItem);

#ifdef _UNICODE
#define apxGetDlgText  apxGetDlgTextW
#else
#define apxGetDlgText  apxGetDlgTextA
#endif

LPSTR       apxBrowseForFolderA(HWND hWnd, LPCSTR szTitle, LPCSTR szName);
LPWSTR      apxBrowseForFolderW(HWND hWnd, LPCWSTR szTitle, LPCWSTR szName);

#ifdef _UNICODE
#define apxBrowseForFolder  apxBrowseForFolderW
#else
#define apxBrowseForFolder  apxBrowseForFolderA
#endif

LPSTR       apxGetFileNameA(HWND hWnd, LPCSTR szTitle, LPCSTR szFilter,
                            LPCSTR szDefExt, LPCSTR szDefPath, BOOL bOpenOrSave,
                            LPDWORD lpdwFindex);

LPWSTR      apxGetFileNameW(HWND hWnd, LPCWSTR szTitle, LPCWSTR szFilter,
                            LPCWSTR szDefExt, LPCWSTR szDefPath, BOOL bOpenOrSave,
                            LPDWORD lpdwFindex);

#ifdef _UNICODE
#define apxGetFileName  apxGetFileNameW
#else
#define apxGetFileName  apxGetFileNameA
#endif

LPCWSTR     apxDlgSelectUser(HWND hWnd, LPWSTR szUser);

__APXEND_DECLS

#endif /* _GUI_H_INCLUDED_ */
