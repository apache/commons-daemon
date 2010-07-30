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

/* jsvc monitor service module:
 * Implements the body of the service.
 * It reads the register entry and starts the jsvc.
 */

#include <windows.h>
#include <winuser.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <time.h>
#ifdef CYGWIN
#else
#include <tchar.h>
#endif
#include "moni_inst.h"

/* globals */
SERVICE_STATUS          ssStatus;
SERVICE_STATUS_HANDLE   sshStatusHandle;
DWORD                   dwErr;
HANDLE                  hServerStopEvent = NULL;
HANDLE                  hMonitorProcess = NULL;

/*
 * NT/other detection
 * from src/os/win32/service.c (httpd-1.3!).
 */
 
BOOL isWindowsNT(void)
{
    static BOOL once = FALSE;
    static BOOL isNT = FALSE;
 
    if (!once)
    {
        OSVERSIONINFO osver;
        osver.dwOSVersionInfoSize = sizeof(osver);
        if (GetVersionEx(&osver))
            if (osver.dwPlatformId == VER_PLATFORM_WIN32_NT)
                isNT = TRUE;
        once = TRUE;
    }
    return isNT;
}

/* Event logger routine */

VOID AddToMessageLog(LPTSTR lpszMsg)
{
    TCHAR   szMsg[256];
    HANDLE  hEventSource;
    LPCTSTR lpszStrings[2];


        dwErr = GetLastError();

        /* Use event logging to log the error. */

    if (isWindowsNT())
            hEventSource = RegisterEventSource(NULL, TEXT(SZSERVICENAME));
    else
        hEventSource = NULL;

#ifdef CYGWIN
    sprintf(szMsg, TEXT("%s ERROR: %d"), TEXT(SZSERVICENAME), dwErr);
#else
        _stprintf(szMsg, TEXT("%s ERROR: %d"), TEXT(SZSERVICENAME), dwErr);
#endif
        lpszStrings[0] = szMsg;
        lpszStrings[1] = lpszMsg;

        if (hEventSource != NULL) {
            ReportEvent(hEventSource, /* handle of event source */
                EVENTLOG_ERROR_TYPE,  /* event type */
                0,                    /* event category */
                0,                    /* event ID */
                NULL,                 /* current user's SID */
                2,                    /* strings in lpszStrings */
                0,                    /* no bytes of raw data */
                lpszStrings,          /* array of error strings */
                NULL);                /* no raw data */

            (VOID) DeregisterEventSource(hEventSource);
        } else {
        /* Default to a trace file */
        FILE *log;
        log = fopen("c:/jakarta-service.log","a+");
        if (log != NULL) {
                struct tm *newtime;
                time_t long_time;

                time( &long_time );
                newtime = localtime( &long_time );

                if (dwErr)
            fprintf(log,"%.24s:%s: %s\n",asctime(newtime),szMsg, lpszMsg);
                else
            fprintf(log,"%.24s: %s\n",asctime(newtime), lpszMsg);
        fclose(log);
        }
    }
}

/*
 *
 *  FUNCTION: ServiceStop
 *
 *  PURPOSE: Stops the service
 *
 *  PARAMETERS:
 *    none
 *
 *  RETURN VALUE:
 *    none
 *
 *  COMMENTS:
 *    If a ServiceStop procedure is going to
 *    take longer than 3 seconds to execute,
 *    it should spawn a thread to execute the
 *    stop code, and return.  Otherwise, the
 *    ServiceControlManager will believe that
 *    the service has stopped responding.
 *    
 */
VOID ServiceStop()
{
    if ( hServerStopEvent )
        SetEvent(hServerStopEvent);
}

/*
 * Wait for the monitor process to stop
 */
int WaitForMonitor(int num)
{
    DWORD   qreturn;
    int     i;

    for (i=0;i<num;i++) {
        if (hMonitorProcess == NULL) break;
        if (GetExitCodeProcess(hMonitorProcess, &qreturn)) {
            if (qreturn == STILL_ACTIVE) {
                Sleep(1000);
                continue;
            }
            hMonitorProcess = NULL;
            break;
        }
        break;
    }
    if (i==num)
        return -1;
    return 0;
}
/*  This group of functions are provided for the service/console app
 *  to register itself a HandlerRoutine to accept tty or service messages
 *  adapted from src/os/win32/Win9xConHook.c (httpd-1.3!).
 */

/* This is the WndProc procedure for our invisible window.
 * When our subclasssed tty window receives the WM_CLOSE, WM_ENDSESSION,
 * or WM_QUERYENDSESSION messages, the message is dispatched to our hidden
 * window (this message process), and we call the installed HandlerRoutine
 * that was registered by the app.
 */
#ifndef ENDSESSION_LOGOFF
#define ENDSESSION_LOGOFF    0x80000000
#endif
static LRESULT CALLBACK ttyConsoleCtrlWndProc(HWND hwnd, UINT msg,
                                              WPARAM wParam, LPARAM lParam)
{
    int   qreturn;
    if (msg == WM_CREATE) {
        AddToMessageLog(TEXT("ttyConsoleCtrlWndProc WM_CREATE"));
        return 0;
    } else if (msg == WM_DESTROY) {
        AddToMessageLog(TEXT("ttyConsoleCtrlWndProc WM_DESTROY"));
        return 0;
    } else if (msg == WM_CLOSE) {
        /* Call StopService?. */
        AddToMessageLog(TEXT("ttyConsoleCtrlWndProc WM_CLOSE"));
        return 0; /* May return 1 if StopService failed. */
    } else if ((msg == WM_QUERYENDSESSION) || (msg == WM_ENDSESSION)) {
        if (lParam & ENDSESSION_LOGOFF) {
            /* Here we have nothing to our hidden windows should stay. */
            AddToMessageLog(TEXT("ttyConsoleCtrlWndProc LOGOFF"));
            return(1); /* Otherwise it cancels the logoff */
        } else {
            /* Stop Service. */
            AddToMessageLog(TEXT("ttyConsoleCtrlWndProc SHUTDOWN"));
            ServiceStop();

            /* Wait until it stops. */
            qreturn = WaitForMonitor(3);

            if (msg == WM_QUERYENDSESSION) {
                AddToMessageLog(
                    TEXT("ttyConsoleCtrlWndProc SHUTDOWN (query)"));
                if (qreturn) {
                    AddToMessageLog(
                        TEXT("Cancelling shutdown: cannot stop service"));
                    return(0);
                }
            } else
                AddToMessageLog(TEXT("ttyConsoleCtrlWndProc SHUTDOWN"));
            return(1); /* Otherwise it cancels the shutdown. */
        }
    }
    return (DefWindowProc(hwnd, msg, wParam, lParam));
}                                                                               
/* ttyConsoleCreateThread is the process that runs within the user app's
 * context.  It creates and pumps the messages of a hidden monitor window,
 * watching for messages from the system, or the associated subclassed tty
 * window.  Things can happen in our context that can't be done from the
 * tty's context, and visa versa, so the subclass procedure and this hidden
 * window work together to make it all happen.
 */
static DWORD WINAPI ttyConsoleCtrlThread()
{
    HWND monitor_hwnd;
    WNDCLASS wc;
    MSG msg;
    wc.style         = CS_GLOBALCLASS;
    wc.lpfnWndProc   = ttyConsoleCtrlWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 8;
    wc.hInstance     = NULL;
    wc.hIcon         = NULL;
    wc.hCursor       = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = "ApacheJakartaService";
 
    if (!RegisterClass(&wc)) {
        AddToMessageLog(TEXT("RegisterClass failed"));
        return 0;
    }
 
    /* Create an invisible window */
    monitor_hwnd = CreateWindow(wc.lpszClassName, 
                                "ApacheJakartaService",
                                WS_OVERLAPPED & ~WS_VISIBLE,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                NULL, NULL,
                                GetModuleHandle(NULL), NULL);
 
    if (!monitor_hwnd) {
        AddToMessageLog(TEXT("RegisterClass failed"));
        return 0;
    }
 
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
 
    return 0;
}
/*
 * Register the process to resist logoff and start the thread that will receive
 * the shutdown via a hidden window.
 */
BOOL Windows9xServiceCtrlHandler()
{
    HANDLE hThread;
    DWORD tid;
    HINSTANCE hkernel;
    DWORD (WINAPI *register_service_process)(DWORD, DWORD) = NULL;

    /* If we have not yet done so */
    FreeConsole();

    /* Make sure the process will resist logoff */
    hkernel = LoadLibrary("KERNEL32.DLL");
    if (!hkernel) {
        AddToMessageLog(TEXT("LoadLibrary KERNEL32.DLL failed"));
        return 0;
    }
    register_service_process = (DWORD (WINAPI *)(DWORD, DWORD)) 
        GetProcAddress(hkernel, "RegisterServiceProcess");
    if (register_service_process == NULL) {
        AddToMessageLog(TEXT("dlsym RegisterServiceProcess failed"));
        return 0;
    }
    if (!register_service_process(0,TRUE)) {
        FreeLibrary(hkernel);
        AddToMessageLog(TEXT("register_service_process failed"));
        return 0;
    }
    AddToMessageLog(TEXT("jsvc registered as a service"));

    /*
     * To be handle notice the shutdown, we need a thread and window.
     */
     hThread = CreateThread(NULL, 0, ttyConsoleCtrlThread,
                               (LPVOID)NULL, 0, &tid);
     if (hThread) {
        CloseHandle(hThread);
        return TRUE;
     }
    AddToMessageLog(TEXT("jsvc shutdown listener start failed"));
    return TRUE;
}

/* 
 *
 *  FUNCTION: ReportStatusToSCMgr()
 *
 *  PURPOSE: Sets the current status of the service and
 *           reports it to the Service Control Manager
 *
 *  PARAMETERS:
 *    dwCurrentState - the state of the service
 *    dwWin32ExitCode - error code to report
 *    dwWaitHint - worst case estimate to next checkpoint
 *
 *  RETURN VALUE:
 *    TRUE  - success
 *    FALSE - failure
 *
 *  COMMENTS:
 *
 */
BOOL ReportStatusToSCMgr(DWORD dwCurrentState,
                         DWORD dwWin32ExitCode,
                         DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;
    BOOL fResult = TRUE;


        if (dwCurrentState == SERVICE_START_PENDING)
            ssStatus.dwControlsAccepted = 0;
        else
            ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

        ssStatus.dwCurrentState = dwCurrentState;
        ssStatus.dwWin32ExitCode = dwWin32ExitCode;
        ssStatus.dwWaitHint = dwWaitHint;

        if ( ( dwCurrentState == SERVICE_RUNNING ) ||
             ( dwCurrentState == SERVICE_STOPPED ) )
            ssStatus.dwCheckPoint = 0;
        else
            ssStatus.dwCheckPoint = dwCheckPoint++;


        /* Report the status of the service to the service control manager. */

        if (!(fResult = SetServiceStatus( sshStatusHandle, &ssStatus))) {
            AddToMessageLog(TEXT("SetServiceStatus"));
        }
    return fResult;
}

/*
 * Report event to the Service Manager
 */
int ReportManager(int event)
{
    if (isWindowsNT())
        return(ReportStatusToSCMgr(
            event,                 /* service state */
            NO_ERROR,              /* exit code */
            3000));                /* wait hint */
    return(1);
} 

/*
 *
 *  FUNCTION: ServiceStart
 *
 *  PURPOSE: Actual code of the service
 *           that does the work.
 *
 *  PARAMETERS:
 *    dwArgc   - number of command line arguments
 *    lpszArgv - array of command line arguments
 *
 *  RETURN VALUE:
 *    none
 *
 *  COMMENTS:
 *    The default behavior is to read the registry and start jsvc.
 *    The service stops when hServerStopEvent is signalled, the jsvc
 *    is stopped via TermPid(pid) (see kills.c).
 *
 */
VOID ServiceStart (DWORD dwArgc, LPTSTR *lpszArgv)
{
char    Data[512];
DWORD   qreturn;
STARTUPINFO StartupInfo;
PROCESS_INFORMATION ProcessInformation;
char *qptr;


    /* Service initialization */
    ProcessInformation.hProcess = NULL;

    /* report the status to the service control manager. */

    AddToMessageLog(TEXT("ServiceStart: starting"));
    if (!ReportManager(SERVICE_START_PENDING))
        goto cleanup;

    /*
     * create the event object. The control handler function signals
     * this event when it receives the "stop" control code.
     *
     */
    hServerStopEvent = CreateEvent(
        NULL,    /* no security attributes */
        TRUE,    /* manual reset event */
        FALSE,   /* not-signalled */
        NULL);   /* no name */

    if ( hServerStopEvent == NULL)
        goto cleanup;

    /* report the status to the service control manager. */
    if (!ReportManager(SERVICE_START_PENDING))
        goto cleanup;

    /* Read the registry and set environment. */
    if (OnServeSetEnv()) {
      AddToMessageLog(TEXT("ServiceStart: read environment failed"));
      goto cleanup;
      }

    if (!ReportManager(SERVICE_START_PENDING))
        goto cleanup;

    /* set the start path for jsvc.exe */
    qptr = getenv("JAKARTA_HOME");
    if (qptr==NULL || strlen(qptr)==0) {
        AddToMessageLog(TEXT("ServiceStart: read JAKARTA_HOME failed"));
        goto cleanup;
    }

    /* Build the start jsvc command according to the registry information. */
    strcpy(Data,qptr);
    BuildCommand(Data);
    AddToMessageLog(TEXT(Data));

    /* create the jsvc process. */
    AddToMessageLog(TEXT("ServiceStart: start jsvc"));
    memset(&StartupInfo,'\0',sizeof(StartupInfo));
    StartupInfo.cb = sizeof(STARTUPINFO);

    if (!CreateProcess(NULL,Data,NULL,NULL,FALSE,
         DETACHED_PROCESS|NORMAL_PRIORITY_CLASS,
         NULL,NULL, &StartupInfo, &ProcessInformation))
      goto cleanup;
    AddToMessageLog(TEXT("ServiceStart: jsvc started"));
    CloseHandle(ProcessInformation.hThread);
    ProcessInformation.hThread = NULL;
    hMonitorProcess = ProcessInformation.hProcess;

    if (!ReportManager(SERVICE_START_PENDING))
        goto cleanup;

    /* wait until the process is completly created. */
/* With the DETACHED_PROCESS it does not work...
    if (WaitForInputIdle(ProcessInformation.hProcess , INFINITE)) {
      AddToMessageLog(TEXT("ServiceStart: jsvc stopped after creation"));
      goto cleanup;
      }
 */

    /*
     * jsvc is now running.
     * report the status to the service control manager.
     */

    if (!ReportManager(SERVICE_RUNNING))
        goto cleanup;

    /* End of initialization */

    /*
     *
     * Service is now running, perform work until shutdown:
     * Check every 60 seconds if the monitor is up.
     *
     */

    for (;;) {
      qreturn = WaitForSingleObject(hServerStopEvent,60000); /* each minutes. */

      if (qreturn == WAIT_FAILED) break;/* something have gone wrong. */

      if (qreturn == WAIT_TIMEOUT) {
        /* timeout check the monitor. */
        if (GetExitCodeProcess(ProcessInformation.hProcess, &qreturn)) {
          if (qreturn == STILL_ACTIVE) continue;
          }
        AddToMessageLog(TEXT("ServiceStart: jsvc crashed"));
        hMonitorProcess = NULL;
        CloseHandle(hServerStopEvent);
        CloseHandle(ProcessInformation.hProcess);
        exit(0); /* exit ungracefully so
                  * Service Control Manager 
                  * will attempt a restart. */
                    
        break; /*failed. */
        }

      /* stop the monitor by signal(event) */
      sprintf(Data,"ServiceStart: stopping jsvc: %d",
              ProcessInformation.dwProcessId);
      AddToMessageLog(Data);

      if (TermPid(ProcessInformation.dwProcessId)) {
        AddToMessageLog(TEXT("ServiceStart: jsvc stop failed"));
        break;
        }
      WaitForMonitor(6);
      AddToMessageLog(TEXT("ServiceStart: jsvc stopped"));
      break; /* finished!!! */
      }

  cleanup:

    AddToMessageLog(TEXT("ServiceStart: stopped"));
    if (hServerStopEvent)
        CloseHandle(hServerStopEvent);

    if (ProcessInformation.hProcess)
        CloseHandle(ProcessInformation.hProcess);

}

/*
 *
 *  FUNCTION: service_ctrl
 *
 *  PURPOSE: This function is called by the SCM whenever
 *           ControlService() is called on this service.
 *
 *  PARAMETERS:
 *    dwCtrlCode - type of control requested
 *
 *  RETURN VALUE:
 *    none
 *
 *  COMMENTS:
 *
 */
VOID WINAPI service_ctrl(DWORD dwCtrlCode)
{
    /* Handle the requested control code. */
    switch(dwCtrlCode)
    {
        /* Stop the service.
         *
         * SERVICE_STOP_PENDING should be reported before
         * setting the Stop Event - hServerStopEvent - in
         * ServiceStop().  This avoids a race condition
         * which may result in a 1053 - The Service did not respond...
         * error.
         */
        case SERVICE_CONTROL_STOP:
            ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 0);
            ServiceStop();
            return;

        /* Update the service status. */
        case SERVICE_CONTROL_INTERROGATE:
            break;

        /* invalid control code */
        default:
            break;

    }

    ReportStatusToSCMgr(ssStatus.dwCurrentState, NO_ERROR, 0);
}

/*
 *
 *  FUNCTION: service_main
 *
 *  PURPOSE: To perform actual initialization of the service
 *
 *  PARAMETERS:
 *    dwArgc   - number of command line arguments
 *    lpszArgv - array of command line arguments
 *
 *  RETURN VALUE:
 *    none
 *
 *  COMMENTS:
 *    This routine performs the service initialization and then calls
 *    the user defined ServiceStart() routine to perform majority
 *    of the work.
 *
 */
void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv)
{

    AddToMessageLog(TEXT("service_main:starting"));

    /* register our service control handler: */
    sshStatusHandle = RegisterServiceCtrlHandler( TEXT(SZSERVICENAME),
                                                   service_ctrl);

    if (!sshStatusHandle) {
    AddToMessageLog(TEXT("service_main:RegisterServiceCtrlHandler failed"));
        goto cleanup;
    }

    /* SERVICE_STATUS members that don't change in example */
    ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ssStatus.dwServiceSpecificExitCode = 0;


    /* report the status to the service control manager. */
    if (!ReportStatusToSCMgr(SERVICE_START_PENDING,NO_ERROR,3000))   {
    AddToMessageLog(TEXT("service_main:ReportStatusToSCMgr failed"));
        goto cleanup;
    }


    ServiceStart( dwArgc, lpszArgv );

cleanup:
    /*
     * try to report the stopped status to the service control manager.
     */
    if (sshStatusHandle)
        (VOID)ReportStatusToSCMgr(
                            SERVICE_STOPPED,
                            dwErr,
                            0);

    AddToMessageLog(TEXT("service_main:stopped"));
    return;
}

/*
 *
 *  FUNCTION: main
 *
 *  PURPOSE: entrypoint for service
 *
 *  PARAMETERS:
 *    argc - number of command line arguments
 *    argv - array of command line arguments
 *
 *  RETURN VALUE:
 *    none
 *
 *  COMMENTS:
 *    main() either performs the command line task, or
 *    call StartServiceCtrlDispatcher to register the
 *    main service thread.  When the this call returns,
 *    the service has stopped, so exit.
 *
 */
#ifdef CYGWIN
int main(int argc, char **argv)
#else
void _CRTAPI1 main(int argc, char **argv)
#endif
{
    SERVICE_TABLE_ENTRY dispatchTable[] =
    {
        { TEXT(SZSERVICENAME), (LPSERVICE_MAIN_FUNCTION)service_main },
        { NULL, NULL }
    };

    AddToMessageLog(TEXT("StartService starting"));
    if (isWindowsNT()) {
            if (!StartServiceCtrlDispatcher(dispatchTable)) {
        AddToMessageLog(TEXT("StartServiceCtrlDispatcher failed."));
        return;
        }
        AddToMessageLog(TEXT("StartService started"));
    } else {
        Windows9xServiceCtrlHandler();
        ServiceStart(argc,argv);
        AddToMessageLog(TEXT("StartService stopped"));
    }
}
