/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/* @version $Id$ */

#include <windows.h>       /* required for all Windows applications */
#include <stdio.h>         /* for sprintf                           */
#include <io.h>                                                               
#include <fcntl.h>
#include <shellapi.h>
#include <winuser.h> 

#include "resource.h"
#include "moni_inst.h"                                                            
                                                                              


#define WINWIDTH  680
#define WINHEIGHT 460

#define CLASSMAIN
#define VM_ICON_MESS    WM_USER+1
#define VM_ID_TIMER     WM_USER+2
#define VM_START_ICON   WM_USER+3
#define VM_ID_TIMER1    WM_USER+4
#define VM_ID_TIMER2    WM_USER+5

BOOL InitApplication(HANDLE hInstance);

HANDLE hInst;        /* current instance */
SC_HANDLE hManager=NULL;                                     
SC_HANDLE hService=NULL;
int optmode;        /* start, stop or check. */
int flagdown = 0;

#define VDMONISTART 0
#define VDMONISTOP  1
#define VDMONICHECK 2

#define NORMALWINDOW WS_OVERLAPPED|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX
#define ICONWINDOW   WS_CAPTION|WS_POPUPWINDOW|WS_MINIMIZEBOX|WS_MAXIMIZEBOX                                     


/* display a message read from resources */

static void DisplayMess(HWND hDlg,int item)                                            
{                                                                               
char MessBox[256];                                                              
                                                                                
  if (LoadString(hInst,item,MessBox,sizeof(MessBox)))                         
    MessageBox(hDlg, MessBox,"ERROR" , MB_OK);                                  
  else {                                                                        
    sprintf(MessBox,"ERROR %d",item);                                           
    MessageBox(hDlg, MessBox,"ERROR" , MB_OK);                                  
    }                                                                           
}
/*
 * MyTaskBarAddIcon - adds an icon to the taskbar status area. 
 * Returns TRUE if successful or FALSE otherwise. 
 * hwnd - handle of the window to receive callback messages 
 * uID - identifier of the icon 
 * hicon - handle of the icon to add 
 * lpszTip - ToolTip text 
 */
BOOL MyTaskBarAddIcon(HWND hWnd)
{ 
    BOOL res; 
    NOTIFYICONDATA  notifyicondata;
    HICON hicon;
 
    notifyicondata.cbSize=sizeof(notifyicondata); 
    notifyicondata.hWnd=hWnd; 
    notifyicondata.uID=ID_TASKICON; 
    notifyicondata.uFlags= NIF_ICON|NIF_MESSAGE|NIF_TIP; 
    notifyicondata.uCallbackMessage=VM_ICON_MESS;
    hicon = LoadIcon(hInst,"OnServe");
    notifyicondata.hIcon = hicon;
    strcpy(notifyicondata.szTip,"Jakarta Service");


    res = Shell_NotifyIcon(NIM_ADD,&notifyicondata);
 
    if (hicon) 
        DestroyIcon(hicon); 
 
    return res; 
} 
/*  
 * MyTaskBarDeleteIcon - deletes an icon from the taskbar 
 *     status area. 
 * Returns TRUE if successful or FALSE otherwise. 
 * hwnd - handle of the window that added the icon 
 * uID - identifier of the icon to delete 
 */
BOOL MyTaskBarDeleteIcon(HWND hwnd) 
{ 
    BOOL res; 
    NOTIFYICONDATA tnid; 
 
    tnid.cbSize = sizeof(NOTIFYICONDATA); 
    tnid.hWnd = hwnd; 
    tnid.uID = ID_TASKICON; 
         
    res = Shell_NotifyIcon(NIM_DELETE, &tnid); 
    return res; 
}

/* start the vdcom process */
void StartVdcom(HWND hDlg)
{
STARTUPINFO StartupInfo;                                                        
PROCESS_INFORMATION ProcessInformation;
                                     
  memset(&StartupInfo,'\0',sizeof(StartupInfo));                                
  StartupInfo.cb = sizeof(STARTUPINFO);                                         
   
  if (!CreateProcess(NULL,"vdcom.exe",NULL,NULL,FALSE,NORMAL_PRIORITY_CLASS,      
         NULL,NULL, &StartupInfo, &ProcessInformation)) {
      DisplayMess(hDlg,CANNOT_START_VDCOM);
      return;
     }
  /* the handle to the process */
  CloseHandle(ProcessInformation.hProcess); 
  CloseHandle(ProcessInformation.hThread);
}
/* start the vdconf process */
void StartVdconf(HWND hDlg)
{
STARTUPINFO StartupInfo;                                                        
PROCESS_INFORMATION ProcessInformation;
                                     
  memset(&StartupInfo,'\0',sizeof(StartupInfo));                                
  StartupInfo.cb = sizeof(STARTUPINFO);                                         
   
  if (!CreateProcess(NULL,"vdconf.exe",NULL,NULL,FALSE,NORMAL_PRIORITY_CLASS,      
         NULL,NULL, &StartupInfo, &ProcessInformation)) {
      DisplayMess(hDlg,CANNOT_START_VDCONF);
      return;
     }
  /* the handle to the process */
  CloseHandle(ProcessInformation.hProcess); 
  CloseHandle(ProcessInformation.hThread);
}

/* test if service is running */
BOOL IsRunning(HWND hDlg)
{
  SERVICE_STATUS  svcStatus;

    if (!QueryServiceStatus(hService, &svcStatus)) {
        DisplayMess(hDlg,ERROR_STATUS);
        PostQuitMessage(0);
        return(FALSE);
    }
    else {
        if (SERVICE_RUNNING == svcStatus.dwCurrentState)
            return(TRUE);
    }
    return(FALSE);

}
/* test if service is stoppped */
BOOL IsStopped(HWND hDlg)
{
  SERVICE_STATUS  svcStatus;

    if (!QueryServiceStatus(hService, &svcStatus)) {
        DisplayMess(hDlg,ERROR_STATUS);
        PostQuitMessage(0);
        return(FALSE);
    }
    else {
        if (SERVICE_STOPPED == svcStatus.dwCurrentState)
            return(TRUE);
    }
    return(FALSE);

}
/* Yes/No dialog box */
BOOL CALLBACK StopYesNo(
        HWND hDlg,                /* window handle of the dialog box */
        UINT message,             /* type of message                 */
        WPARAM wParam,            /* message-specific information    */
        LPARAM lParam)
{
    SERVICE_STATUS  svcStatus;
    
    switch (message) {
    case WM_COMMAND:                     /* message: received a command */
        if (LOWORD(wParam) == IDOK) {     /* "OK" box selected */
            if (!IsStopped(hDlg)) {
                ControlService(hService, SERVICE_CONTROL_STOP, &svcStatus);
                EndDialog(hDlg, TRUE);
            }
            else
                EndDialog(hDlg, FALSE);
        }
        if (LOWORD(wParam) == IDCANCEL)
            EndDialog(hDlg, FALSE);
        return(TRUE);
    } /* End switch message */
    return (FALSE);                            /* Didn't process a message    */
    UNREFERENCED_PARAMETER(lParam);
}
/* Please wait stopping dialog box */
BOOL CALLBACK PleaseWait(
        HWND hDlg,                /* window handle of the dialog box */
        UINT message,             /* type of message                 */
        WPARAM wParam,            /* message-specific information    */
        LPARAM lParam)
{
    
    switch (message) {
    case WM_INITDIALOG:
        SetTimer(hDlg,VM_ID_TIMER2,5000,NULL); /* wait 5 seconds. */
    case WM_COMMAND:                     /* message: received a command */
        if (LOWORD(wParam) == IDCANCEL)
            EndDialog(hDlg, FALSE);
        return(TRUE);
    case WM_TIMER:
        if(IsStopped(hDlg))
            EndDialog(hDlg, TRUE);
        else
            SetTimer(hDlg,VM_ID_TIMER2,5000,NULL); /* wait 5 seconds. */
        return(TRUE);
    } /* End switch message */
    return (FALSE);                            /* Didn't process a message    */
    UNREFERENCED_PARAMETER(lParam);
}
BOOL CALLBACK StartYesNo(
        HWND hDlg,                /* window handle of the dialog box */
        UINT message,             /* type of message                 */
        WPARAM wParam,            /* message-specific information    */
        LPARAM lParam)
{

    switch (message) {
    case WM_INITDIALOG:
        if (!StartService(hService,0,NULL)) {
            DisplayMess(hDlg,CANNOT_START);
            PostQuitMessage(0);
        }
        /* test if running, if not set a timer. */
        /* wait until service is start */
        if (IsRunning(hDlg)) 
            EndDialog(hDlg, TRUE);
        else
            SetTimer(hDlg,VM_ID_TIMER1,5000,NULL); /* wait 5 seconds. */
        return(TRUE);
        
    case WM_TIMER:
        if(IsRunning(hDlg))
            EndDialog(hDlg, TRUE);
        else
            SetTimer(hDlg,VM_ID_TIMER1,5000,NULL); /* wait 5 seconds. */
        return(TRUE);
        
    case WM_COMMAND:                    /* message: received a command */
        if (LOWORD(wParam) == IDCANCEL) {
            if (IsRunning(hDlg)) 
                EndDialog(hDlg, TRUE);
            else
                PostQuitMessage(0);
        }
        return(TRUE);
        
    } /* End switch message */
    return (FALSE);                            /* Didn't process a message    */
    UNREFERENCED_PARAMETER(lParam);
}

/* Display the menu */

void ShowMenu(HWND hWnd)
{
   HMENU hMenu,hMenu1;
   POINT point;

   hMenu = LoadMenu(hInst,"MENU");
   SetMenu(hWnd,hMenu);
   hMenu1 = GetSubMenu(hMenu,0);

   GetCursorPos(&point);

   SetForegroundWindow(hWnd); /* MS bug. */
   TrackPopupMenuEx(hMenu1,TPM_RIGHTALIGN|TPM_BOTTOMALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
                    point.x,point.y,hWnd,NULL);
   PostMessage(hWnd, WM_USER, 0, 0); /* MS bug. */
   DestroyMenu(hMenu);

}                                                                             
/****************************************************************************\
*
*    FUNCTION:  InitInstance(HANDLE, int)
*
*    PURPOSE:  Saves instance handle and creates main window
*
*\***************************************************************************/

BOOL InitInstance(
    HANDLE          hInstance,          /* Current instance identifier.       */
    int             nCmdShow)           /* Param for first ShowWindow() call. */
{
    HWND            hWnd;               /* Main window handle.                */
    DWORD           dwStyle,dwExStyle;


    /* Save the instance handle in static variable, which will be used in  */
    /* many subsequence calls from this application to Windows.            */

    hInst = hInstance;

    /* Create a main window for this application instance.  */

    if (optmode == VDMONISTART || optmode == VDMONICHECK) {
        dwStyle = ICONWINDOW;
        dwExStyle = WS_EX_APPWINDOW;
    }
    else {
        dwStyle = NORMALWINDOW;
        dwExStyle = 0;
    }

    hWnd = CreateWindowEx(
        dwExStyle,
        "OnServe",                      /* See RegisterClass() call.          */
        "OnServe Monitor Control",      /* Text for window title bar.   */
        dwStyle,                        /* Window style.*/
        CW_USEDEFAULT,                  /* Default horizontal position.       */
        CW_USEDEFAULT,                  /* Default vertical position.         */
        WINWIDTH,                       /* Windows width.                     */
        WINHEIGHT,                      /* Windows height.                    */
        NULL,                           /* Overlapped windows have no parent. */
        NULL,                           /* Use the window class menu.         */
        hInstance,                      /* This instance owns this window.    */
        NULL                            /* Pointer not needed.                */
    );


    /* If window could not be created, return "failure" */

    if (!hWnd) {
        return (FALSE);
    }

    return (TRUE);               /* Returns the value from PostQuitMessage */

}
/****************************************************************************\
*
*    FUNCTION: MainWndProc(HWND, unsigned, WORD, LONG)
*
*    PURPOSE:  Processes main window messages
*
*\***************************************************************************/

LRESULT CALLBACK MainWndProc(
        HWND hWnd,                /* window handle                   */
        UINT message,             /* type of message                 */
        WPARAM wParam,            /* additional information          */
        LPARAM lParam)            /* additional information          */
{
   SERVICE_STATUS  svcStatus;

   switch (message) {
   case WM_CREATE:

      /* acces to service manager. */
      hManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
      if (hManager==NULL) {
          DisplayMess(hWnd,NO_ACCESS);
          PostQuitMessage(0);
          break;
      }
      /* access to monitor service.  */
      hService = OpenService(hManager, SZSERVICENAME, SERVICE_ALL_ACCESS);
      if (hService==NULL) {
          DisplayMess(hWnd,NO_ACCESS_MONI);
          PostQuitMessage(0);
          break;
      }
      /* check if running. */
      if (QueryServiceStatus(hService, &svcStatus)) {       
          /* and see if the service is stopped */
          if (SERVICE_STOPPED == svcStatus.dwCurrentState &&
              optmode == VDMONISTOP) {
              DisplayMess(hWnd,ALREADY_STOP);
              PostQuitMessage(0);
              break;
          }
          else if (SERVICE_RUNNING == svcStatus.dwCurrentState &&
              optmode == VDMONISTART) {
              DisplayMess(hWnd,ALREADY_START);
              PostQuitMessage(0);
              break;
          }

      }
      else {
          DisplayMess(hWnd,ERROR_STATUS);
          PostQuitMessage(0);
          break;
      }

      /* DialogBoxes Yes/No */
      if (optmode == VDMONISTART)
          DialogBox(hInst,                     /* current instance         */
                "StartYesNo",                /* resource to use          */
                hWnd,                        /* parent handle            */
                StartYesNo);                 /* instance address         */
      else if (optmode == VDMONISTOP)
          if (DialogBox(hInst,"StopYesNo",hWnd,StopYesNo))
              DialogBox(hInst,"PleaseWait",hWnd,PleaseWait);

    
      if (optmode == VDMONISTOP)
          PostQuitMessage(0);
      else {
          SetTimer(hWnd,VM_ID_TIMER,5000,NULL);
          MyTaskBarAddIcon(hWnd);
      }

      break;   /* WM_CREATE */

   case VM_ICON_MESS:
       switch ((UINT)lParam) {
       case WM_LBUTTONDBLCLK:
           StartVdcom(hWnd);
           break;
       case WM_RBUTTONDOWN:
             flagdown = 1;
           break;
       case WM_RBUTTONUP:
           if (flagdown == 1) {
           flagdown = 2;
           /* show a  menu. */
           ShowMenu(hWnd);
           }
           break;
       }
       break;


   case WM_TIMER:
       /* check if service is running */
       if (QueryServiceStatus(hService, &svcStatus)) {       
          /* and see if the service is stopped */
           if (SERVICE_STOPPED == svcStatus.dwCurrentState) {
               MyTaskBarDeleteIcon(hWnd);
               PostQuitMessage(0);
           }
           SetTimer(hWnd,VM_ID_TIMER,5000,NULL);
           return(0);
       }
       break;

   case VM_START_ICON:
       /* add the icon and timer. */
       MyTaskBarAddIcon(hWnd);
       SetTimer(hWnd,VM_ID_TIMER,5000,NULL);
       break;

   case WM_COMMAND:
       /* command for the popup menu. */
       switch (LOWORD(wParam)) {
       case ID_START_VDCOM:
           StartVdcom(hWnd);
           break;
       case ID_STOP_VDMONI:
           if (DialogBox(hInst,"StopYesNo",hWnd,StopYesNo))
              DialogBox(hInst,"PleaseWait",hWnd,PleaseWait);
           break;
       case ID_PROPRETY:
           StartVdconf(hWnd);
           break;

       }
       break;

  /*
   *   Clean up.
   */
   case WM_DESTROY:
       if (hService!=NULL)
           CloseServiceHandle(hService);
       if (hManager!=NULL) 
           CloseServiceHandle(hManager);
      PostQuitMessage(0);
      break;

   default:                       /* Passes it on if unproccessed    */
      return (DefWindowProc(hWnd, message, wParam, lParam));

   }
   return (0);

}

/****************************************************************************
*
*    FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)
*
*    PURPOSE: calls initialization function, processes message loop
*
*\***************************************************************************/

WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{

    MSG msg;

    UNREFERENCED_PARAMETER( lpCmdLine );
    if (strcmp(lpCmdLine,"start")==0) 
        optmode = VDMONISTART;
    else if (strcmp(lpCmdLine,"stop")==0)
        optmode = VDMONISTOP;
    else
        optmode = VDMONICHECK;

    if (!hPrevInstance)                  /* Other instances of app running? */
        if (!InitApplication(hInstance)) /* Initialize shared things        */
            return (FALSE);              /* Exits if unable to initialize   */

    /*
    *   Perform initializations that apply to a specific instance
    */
    if (!InitInstance(hInstance, nCmdShow))
        return (FALSE);

    /*
    *   Acquire and dispatch messages until a WM_QUIT message is received.
    */
    while (GetMessage(&msg,        /* message structure                      */
            NULL,                  /* handle of window receiving the message */
            0,             /* lowest message to examine              */
            0))            /* highest message to examine             */
        {
        TranslateMessage(&msg);    /* Translates virtual key codes           */
        DispatchMessage(&msg);     /* Dispatches message to window           */
   }
    return (msg.wParam);           /* Returns the value from PostQuitMessage */
}

/****************************************************************************
*
*    FUNCTION: InitApplication(HANDLE)
*
*    PURPOSE: Initializes window data and registers window class
*
*\***************************************************************************/

BOOL InitApplication(HANDLE hInstance)       /* current instance             */
{
    WNDCLASS  wc;

    /* Fill in window class structure with parameters that describe the       */
    /* main window.                                                           */

    wc.style = 0;                       /* Class style(s).                    */
    wc.lpfnWndProc = MainWndProc;       /* Function to retrieve messages for  */
                                        /* windows of this class.             */
    wc.cbClsExtra = 0;                  /* No per-class extra data.           */
    wc.cbWndExtra = 0;                  /* No per-window extra data.          */
    wc.hIcon = LoadIcon (hInstance, "onserve"); /* Icon name from .RC        */
    wc.hInstance = hInstance;          /* Application that owns the class.   */
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  "onservemenu";   /* Name of menu resource in .RC file. */
    wc.lpszClassName = "OnServe"; /* Name used in call to CreateWindow. */

    /* Register the window class and return success/failure code. */

    return (RegisterClass(&wc));

}

