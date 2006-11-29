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

/** Console layout saved in the registry as binary */
typedef struct stAPXCONSOLE_LAYOUT {
    /** Main window size */
    POINT   ptSize;
    /** Main window size and position */
    RECT    rcPos;
    /** Client window size */
    POINT   ptClientSize;
    /** Client size and position */
    RECT    rcClientPos;
    /** Scroll position */
    POINT   ptScrollPos;
    /** Maximum Scroll position */
    POINT   ptScrollMax;
    /** Caret position */
    POINT   ptCaretPos;
    /** Background color */
    COLORREF clrBackground;
    /** Text color */
    COLORREF clrText;
    /** Selected text background color */
    COLORREF clrSelectBackground;
    /** Selected rext color */
    COLORREF clrSelectText;
    /** Character size */
    POINT    ptCharSize;
    /** Current Screen size */
    POINT    ptScreenSize;    
    /** Font size */
    INT      nFontSize;
    /** Caret size */
    POINT    ptCaretSize;
    /** Caret Bilnk time in ms */
    UINT     nCaretBlinkTime;
    /** Typeface name of the font (32 char max including the null terminator) */
    TCHAR   szFontName[32];
    
} stAPXCONSOLE_LAYOUT, *APXCONSOLE_LAYOUT;

typedef struct stAPXCONSOLE {
    /** Application instance handle */
    HINSTANCE   hInstance;
    /** Console Screen Buffer */
    TCHAR       lpScreenBuffer;   
    /** The size of the Console Screen Buffer */
    DWORD       dwScreenBufferSize;
    /** Main window Handle */
    HWND        hWnd;
    /** Console window Handle */
    HWND        hConsole;
    /** Console Window Layout */
    stAPXCONSOLE_LAYOUT stLayout;
    /** Console Keyboard Buffer */
    TCHAR       lpKeyboardBuffer;
    /** The size of the Keyboard Buffer */
    DWORD       dwKeyboardBufferSize;
    /** Console Handler routine */
    PHANDLER_ROUTINE fnHandlerRoutine;
    /** Console Reference Count */
    DWORD       dwReferenceCount;
} stAPXCONSOLE, *APXCONSOLE;

APXHANDLE  __st_sys_console = NULL;

static BOOL __apxConsoleCallback(APXHANDLE hObject, UINT uMsg,
                                 WPARAM wParam, LPARAM lParam)
{
    APXCONSOLE lpConsole = APXHANDLE_DATA(hObject);

    if (hObject->dwType != APXHANDLE_TYPE_CONSOLE)
        return FALSE;
    switch (uMsg) {
        case WM_CLOSE:
            
        break;
        default:
        break;
    }
    return TRUE;
}

APXHANDLE
apxAllocConsoleHandle(APXHANDLE hPool)
{
    APXHANDLE hConsole;

    hConsole = apxHandleCreate(hPool, APXHANDLE_HAS_EVENT |
                               APXHANDLE_OWNS_LOCK, NULL,
                               sizeof(stAPXCONSOLE),
                               __apxConsoleCallback);

    return hConsole;
}

BOOL
apxFreeConsoleHandle(APXHANDLE hConsole)
{
    APXCONSOLE lpConsole = APXHANDLE_DATA(hConsole);
    if (hConsole->dwType != APXHANDLE_TYPE_CONSOLE)
        return FALSE;
        
    return apxCloseHandle(hConsole);
}





/** Standard Console Functions */
BOOL
apxAllocConsole()
{

    if (!__st_sys_console) {
        __st_sys_console = apxAllocConsoleHandle(NULL);
        return TRUE;
    }
    else
        return FALSE;
}

BOOL
apxFreeConsole()
{
    if (apxFreeConsoleHandle(__st_sys_console)) {
        __st_sys_console = NULL;
        return TRUE;
    }
    else
        return FALSE;
}
