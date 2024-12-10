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
 * prunsrv -- Service Runner.
 * Contributed by Mladen Turk <mturk@apache.org>
 * 05 Aug 2003
 * ====================================================================
 */

/* Force the JNI vprintf functions */
#define _DEBUG_JNI  1
#include "apxwin.h"
#include "private.h"
#include "prunsrv.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <fcntl.h>
#include <io.h>         /* _open_osfhandle */
#include <share.h>

#ifndef  MIN
#define  MIN(a,b)    (((a)<(b)) ? (a) : (b))
#endif

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#define ONE_MINUTE    (60 * 1000)

#ifdef _WIN64
#define KREG_WOW6432  KEY_WOW64_32KEY
#define PRG_BITS      64
#else
#define KREG_WOW6432  0
#define PRG_BITS      32
#endif

typedef struct APX_STDWRAP {
    LPCWSTR szLogPath;
    LPCWSTR szStdOutFilename;
    LPCWSTR szStdErrFilename;
    FILE   *fpStdOutFile;
    FILE   *fpStdErrFile;
} APX_STDWRAP;

/* Use static variables instead of #defines */
static LPCWSTR  PRSRV_AUTO        = L"auto";
static LPCWSTR  PRSRV_JAVA        = L"java";
static LPCWSTR  PRSRV_JVM         = L"jvm";
static LPCWSTR  PRSRV_JDK         = L"jdk";
static LPCWSTR  PRSRV_JRE         = L"jre";
static LPCWSTR  PRSRV_DELAYED     = L"delayed";
static LPCWSTR  PRSRV_MANUAL      = L"manual";
static LPCWSTR  PRSRV_JBIN        = L"\\bin\\java.exe";
static LPCWSTR  PRSRV_PBIN        = L"\\bin";
static LPCWSTR  PRSRV_SIGNAL      = L"SIGNAL";
static LPCWSTR  PRSV_JVMOPTS9     = L"JDK_JAVA_OPTIONS=";
static LPCWSTR  STYPE_INTERACTIVE = L"interactive";

static LPWSTR       _service_name = NULL;
/* Allowed procrun commands */
static LPCWSTR _commands[] = {
    L"TS",      /*  1 Run Service as console application (default)*/
    L"RS",      /*  2 Run Service */
    L"ES",      /*  3 Execute start */
    L"SS",      /*  4 Stop Service */
    L"US",      /*  5 Update Service parameters */
    L"IS",      /*  6 Install Service */
    L"DS",      /*  7 Delete Service */
    L"PS",      /*  8 Print Service Configuration */
    L"?",       /*  9 Help */
    L"VS",      /* 10 Version */
    NULL
};

static LPCWSTR _altcmds[] = {
    L"run",         /*  1 Run Service as console application (default)*/
    L"service",     /*  2 Run Service */
    L"start",       /*  3 Start Service */
    L"stop",        /*  4 Stop Service */
    L"update",      /*  5 Update Service parameters */
    L"install",     /*  6 Install Service */
    L"delete",      /*  7 Delete Service */
	L"print",       /*  8 Print Service Configuration */
    L"help",        /*  9 Help */
    L"version",     /* 10 Version */
    NULL
};

/* Allowed procrun parameters */
static APXCMDLINEOPT _options[] = {

/* 0  */    { L"Description",       L"Description",     NULL,           APXCMDOPT_STR | APXCMDOPT_SRV, NULL, 0},
/* 1  */    { L"DisplayName",       L"DisplayName",     NULL,           APXCMDOPT_STR | APXCMDOPT_SRV, NULL, 0},
/* 2  */    { L"Install",           L"ImagePath",       NULL,           APXCMDOPT_STE | APXCMDOPT_SRV, NULL, 0},
/* 3  */    { L"ServiceUser",       L"ServiceUser",     NULL,           APXCMDOPT_STR | APXCMDOPT_SRV, NULL, 0},
/* 4  */    { L"ServicePassword",   L"ServicePassword", NULL,           APXCMDOPT_STR | APXCMDOPT_SRV, NULL, 0},
/* 5  */    { L"Startup",           L"Startup",         NULL,           APXCMDOPT_STR | APXCMDOPT_SRV, NULL, 0},
/* 6  */    { L"Type",              L"Type",            NULL,           APXCMDOPT_STR | APXCMDOPT_SRV, NULL, 0},
/* 7  */    { L"DependsOn",         L"DependOnService", NULL,           APXCMDOPT_MSZ | APXCMDOPT_SRV, NULL, 0},

/* 8  */    { L"Environment",       L"Environment",     NULL,           APXCMDOPT_MSZ | APXCMDOPT_REG, NULL, 0},
/* 9  */    { L"User",              L"User",            NULL,           APXCMDOPT_STR | APXCMDOPT_REG, NULL, 0},
/* 10 */    { L"Password",          L"Password",        NULL,           APXCMDOPT_BIN | APXCMDOPT_REG, NULL, 0},
/* 11 */    { L"LibraryPath",       L"LibraryPath",     NULL,           APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},

/* 12 */    { L"JavaHome",          L"JavaHome",        L"Java",        APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},
/* 13 */    { L"Jvm",               L"Jvm",             L"Java",        APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},
/* 14 */    { L"JvmOptions",        L"Options",         L"Java",        APXCMDOPT_MSZ | APXCMDOPT_REG, NULL, 0},
/* 15 */    { L"JvmOptions9",       L"Options9",        L"Java",        APXCMDOPT_MSZ | APXCMDOPT_REG, NULL, 0},
/* 16 */    { L"Classpath",         L"Classpath",       L"Java",        APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},
/* 17 */    { L"JvmMs",             L"JvmMs",           L"Java",        APXCMDOPT_INT | APXCMDOPT_REG, NULL, 0},
/* 18 */    { L"JvmMx",             L"JvmMx",           L"Java",        APXCMDOPT_INT | APXCMDOPT_REG, NULL, 0},
/* 19 */    { L"JvmSs",             L"JvmSs",           L"Java",        APXCMDOPT_INT | APXCMDOPT_REG, NULL, 0},

/* 20 */    { L"StopImage",         L"Image",           L"Stop",        APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},
/* 21 */    { L"StopPath",          L"WorkingPath",     L"Stop",        APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},
/* 22 */    { L"StopClass",         L"Class",           L"Stop",        APXCMDOPT_STR | APXCMDOPT_REG, NULL, 0},
/* 23 */    { L"StopParams",        L"Params",          L"Stop",        APXCMDOPT_MSZ | APXCMDOPT_REG, NULL, 0},
/* 24 */    { L"StopMethod",        L"Method",          L"Stop",        APXCMDOPT_STR | APXCMDOPT_REG, NULL, 0},
/* 25 */    { L"StopMode",          L"Mode",            L"Stop",        APXCMDOPT_STR | APXCMDOPT_REG, NULL, 0},
/* 26 */    { L"StopTimeout",       L"Timeout",         L"Stop",        APXCMDOPT_INT | APXCMDOPT_REG, NULL, 0},

/* 27 */    { L"StartImage",        L"Image",           L"Start",       APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},
/* 28 */    { L"StartPath",         L"WorkingPath",     L"Start",       APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},
/* 29 */    { L"StartClass",        L"Class",           L"Start",       APXCMDOPT_STR | APXCMDOPT_REG, NULL, 0},
/* 30 */    { L"StartParams",       L"Params",          L"Start",       APXCMDOPT_MSZ | APXCMDOPT_REG, NULL, 0},
/* 31 */    { L"StartMethod",       L"Method",          L"Start",       APXCMDOPT_STR | APXCMDOPT_REG, NULL, 0},
/* 32 */    { L"StartMode",         L"Mode",            L"Start",       APXCMDOPT_STR | APXCMDOPT_REG, NULL, 0},

/* 33 */    { L"LogPath",           L"Path",            L"Log",         APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},
/* 34 */    { L"LogPrefix",         L"Prefix",          L"Log",         APXCMDOPT_STR | APXCMDOPT_REG, NULL, 0},
/* 35 */    { L"LogLevel",          L"Level",           L"Log",         APXCMDOPT_STR | APXCMDOPT_REG, NULL, 0},
/* 36 */    { L"StdError",          L"StdError",        L"Log",         APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},
/* 37 */    { L"StdOutput",         L"StdOutput",       L"Log",         APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},
/* 38 */    { L"LogJniMessages",    L"LogJniMessages",  L"Log",         APXCMDOPT_INT | APXCMDOPT_REG, NULL, 1},
/* 39 */    { L"PidFile",           L"PidFile",         L"Log",         APXCMDOPT_STR | APXCMDOPT_REG, NULL, 0},
/* 40 */    { L"Rotate",            L"Rotate",          L"Log",         APXCMDOPT_INT | APXCMDOPT_REG, NULL, 0},
            /* NULL terminate the array */
            { NULL }
};

#define GET_OPT_V(x)  _options[x].szValue
#define GET_OPT_I(x)  _options[x].dwValue
#define GET_OPT_T(x)  _options[x].dwType

#define ST_DESCRIPTION      GET_OPT_T(0)
#define ST_DISPLAYNAME      GET_OPT_T(1)
#define ST_INSTALL          GET_OPT_T(2)
#define ST_SUSER            GET_OPT_T(3)
#define ST_SPASSWORD        GET_OPT_T(4)
#define ST_STARTUP          GET_OPT_T(5)
#define ST_TYPE             GET_OPT_T(6)

#define SO_DESCRIPTION      GET_OPT_V(0)
#define SO_DISPLAYNAME      GET_OPT_V(1)
#define SO_INSTALL          GET_OPT_V(2)
#define SO_SUSER            GET_OPT_V(3)
#define SO_SPASSWORD        GET_OPT_V(4)
#define SO_STARTUP          GET_OPT_V(5)
#define SO_TYPE             GET_OPT_V(6)

#define SO_DEPENDSON        GET_OPT_V(7)
#define SO_ENVIRONMENT      GET_OPT_V(8)

#define SO_USER             GET_OPT_V(9)
#define SO_PASSWORD         GET_OPT_V(10)
#define SO_LIBPATH          GET_OPT_V(11)

#define SO_JAVAHOME         GET_OPT_V(12)
#define SO_JVM              GET_OPT_V(13)
#define SO_JVMOPTIONS       GET_OPT_V(14)
#define SO_JVMOPTIONS9      GET_OPT_V(15)
#define SO_CLASSPATH        GET_OPT_V(16)
#define SO_JVMMS            GET_OPT_I(17)
#define SO_JVMMX            GET_OPT_I(18)
#define SO_JVMSS            GET_OPT_I(19)

#define SO_STOPIMAGE        GET_OPT_V(20)
#define SO_STOPPATH         GET_OPT_V(21)
#define SO_STOPCLASS        GET_OPT_V(22)
#define SO_STOPPARAMS       GET_OPT_V(23)
#define SO_STOPMETHOD       GET_OPT_V(24)
#define SO_STOPMODE         GET_OPT_V(25)
#define SO_STOPTIMEOUT      GET_OPT_I(26)

#define SO_STARTIMAGE       GET_OPT_V(27)
#define SO_STARTPATH        GET_OPT_V(28)
#define SO_STARTCLASS       GET_OPT_V(29)
#define SO_STARTPARAMS      GET_OPT_V(30)
#define SO_STARTMETHOD      GET_OPT_V(31)
#define SO_STARTMODE        GET_OPT_V(32)

#define SO_LOGPATH          GET_OPT_V(33)
#define SO_LOGPREFIX        GET_OPT_V(34)
#define SO_LOGLEVEL         GET_OPT_V(35)

#define SO_STDERROR         GET_OPT_V(36)
#define SO_STDOUTPUT        GET_OPT_V(37)
#define SO_JNIVFPRINTF      GET_OPT_I(38)
#define SO_PIDFILE          GET_OPT_V(39)
#define SO_LOGROTATE        GET_OPT_I(40)

static SERVICE_STATUS        _service_status;
static SERVICE_STATUS_HANDLE _service_status_handle = NULL;
/* Set if launched by SCM   */
static BOOL                  _service_mode = FALSE;
/* JVM used as worker       */
static BOOL                  _jni_startup  = FALSE;
/* JVM used for shutdown    */
static BOOL                  _jni_shutdown = FALSE;
/* Java used as worker       */
static BOOL                  _java_startup  = FALSE;
/* Java used for shutdown    */
static BOOL                  _java_shutdown = FALSE;
/* We have request to shutdown the exe running */
static BOOL                  _exe_shutdown = FALSE;
/* Global variables and objects */
static APXHANDLE    gPool;
static APXHANDLE    gWorker;
static APX_STDWRAP  gStdwrap;           /* stdio/stderr redirection */
static int          gExitval;
static LPWSTR       gStartPath;

static LPWSTR   _jni_jvmpath              = NULL;   /* Path to jvm dll */
static LPSTR    _jni_jvmoptions           = NULL;   /* jvm options */
static LPSTR    _jni_jvmoptions9          = NULL;   /* java 9+ options */

static LPSTR    _jni_classpath            = NULL;
static LPCWSTR  _jni_rparam               = NULL;    /* Startup  arguments */
static LPCWSTR  _jni_sparam               = NULL;    /* Shutdown arguments */
static LPSTR    _jni_rmethod              = NULL;    /* Startup  method */
static LPSTR    _jni_smethod              = NULL;    /* Shutdown method */
static LPSTR    _jni_rclass               = NULL;    /* Startup  class */
static LPSTR    _jni_sclass               = NULL;    /* Shutdown class */
static HANDLE gShutdownEvent = NULL;
static HANDLE gSignalEvent   = NULL;
static HANDLE gSignalThread  = NULL;
static HANDLE gPidfileHandle = NULL;
static LPWSTR gPidfileName   = NULL;
static BOOL   gSignalValid   = TRUE;
static APXJAVA_THREADARGS gRargs;
static APXJAVA_THREADARGS gSargs;

DWORD WINAPI eventThread(LPVOID lpParam)
{
    DWORD dwRotateCnt = SO_LOGROTATE;

    for (;;) {
        DWORD dw = WaitForSingleObject(gSignalEvent, 1000);
        if (dw == WAIT_TIMEOUT) {
            /* Do process maintenance */
            if (SO_LOGROTATE != 0 && --dwRotateCnt == 0) {
                /* Perform log rotation. */

                 dwRotateCnt = SO_LOGROTATE;
            }
            continue;
        }
        if (dw == WAIT_OBJECT_0 && gSignalValid) {
            if (!GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, 0)) {
                /* Invoke Thread dump */
                if (gWorker && _jni_startup)
                    apxJavaDumpAllStacks(gWorker);
            }
            ResetEvent(gSignalEvent);
            continue;
        }
        break;
    }
    ExitThread(0);
    return 0;
    UNREFERENCED_PARAMETER(lpParam);
}

/* redirect console stdout/stderr to files
 * so that Java messages can get logged
 * If stderrfile is not specified it will
 * go to stdoutfile.
 */
static BOOL redirectStdStreams(APX_STDWRAP *lpWrapper, LPAPXCMDLINE lpCmdline)
{
    BOOL aErr = FALSE;
    BOOL aOut = FALSE;

    /* Allocate console if we have none
     */
    if (GetConsoleWindow() == NULL) {
        HWND hc;
        AllocConsole();
        if ((hc = GetConsoleWindow()) != NULL)
            ShowWindow(hc, SW_HIDE);
    }
    /* redirect to file or console */
    if (lpWrapper->szStdOutFilename) {
        if (lstrcmpiW(lpWrapper->szStdOutFilename, PRSRV_AUTO) == 0) {
            WCHAR lsn[1024];
            aOut = TRUE;
            lstrlcpyW(lsn, 1020, lpCmdline->szApplication);
            lstrlcatW(lsn, 1020, L"-stdout");
            lstrlocaseW(lsn);
            lpWrapper->szStdOutFilename = apxLogFile(gPool,
                                                     lpWrapper->szLogPath,
                                                     lsn,
                                                     NULL, TRUE,
                                                     SO_LOGROTATE);
        }
        /* Delete the file if not in append mode
         * XXX: See if we can use the params instead of that.
         */
        if (!aOut)
            DeleteFileW(lpWrapper->szStdOutFilename);
        if ((lpWrapper->fpStdOutFile = _wfsopen(lpWrapper->szStdOutFilename,
                                               L"a",
                                               _SH_DENYNO))) {
            _dup2(_fileno(lpWrapper->fpStdOutFile), 1);
            *stdout = *lpWrapper->fpStdOutFile;
            setvbuf(stdout, NULL, _IONBF, 0);
        }
        else {
            lpWrapper->szStdOutFilename = NULL;
        }
    }
    if (lpWrapper->szStdErrFilename) {
        if (lstrcmpiW(lpWrapper->szStdErrFilename, PRSRV_AUTO) == 0) {
            WCHAR lsn[1024];
            aErr = TRUE;
            lstrlcpyW(lsn, 1020, lpCmdline->szApplication);
            lstrlcatW(lsn, 1020, L"-stderr");
            lstrlocaseW(lsn);
            lpWrapper->szStdErrFilename = apxLogFile(gPool,
                                                     lpWrapper->szLogPath,
                                                     lsn,
                                                     NULL, TRUE,
                                                     SO_LOGROTATE);
        }
        if (!aErr)
            DeleteFileW(lpWrapper->szStdErrFilename);
        if ((lpWrapper->fpStdErrFile = _wfsopen(lpWrapper->szStdErrFilename,
                                               L"a",
                                               _SH_DENYNO))) {
            _dup2(_fileno(lpWrapper->fpStdErrFile), 2);
            *stderr = *lpWrapper->fpStdErrFile;
            setvbuf(stderr, NULL, _IONBF, 0);
        }
        else {
            lpWrapper->szStdOutFilename = NULL;
        }
    }
    else if (lpWrapper->fpStdOutFile) {
        _dup2(_fileno(lpWrapper->fpStdOutFile), 2);
        *stderr = *lpWrapper->fpStdOutFile;
         setvbuf(stderr, NULL, _IONBF, 0);
    }
    return TRUE;
}

/* Prints usage. */
static void printUsage(LPAPXCMDLINE lpCmdline, BOOL isHelp)
{
    int i = 0;
    fwprintf(stderr, L"Usage: %s command [ServiceName] [--options]\n",
             lpCmdline->szExecutable);
    fwprintf(stderr, L"  Commands:\n");
    if (isHelp)
        fwprintf(stderr, L"  help                   This page\n");
    fwprintf(stderr, L"  install [ServiceName]  Install Service\n");
    fwprintf(stderr, L"  update  [ServiceName]  Update Service parameters\n");
    fwprintf(stderr, L"  delete  [ServiceName]  Delete Service\n");
    fwprintf(stderr, L"  start   [ServiceName]  Start Service\n");
    fwprintf(stderr, L"  stop    [ServiceName]  Stop Service\n");
    fwprintf(stderr, L"  run     [ServiceName]  Run Service as console application\n");
    fwprintf(stderr, L"  print   [ServiceName]  Display the command to (re-)create the current configuration\n");
    fwprintf(stderr, L"  pause   [Num Seconds]  Sleep for n Seconds (defaults to 60)\n");
    fwprintf(stderr, L"  version                Display version\n");
    fwprintf(stderr, L"  Options:\n");
    while (_options[i].szName) {
        fwprintf(stderr, L"  --%s\n", _options[i].szName);
        ++i;
    }
}

/* Prints version. */
static void printVersion(void)
{
    fwprintf(stderr, L"Apache Commons Daemon Service Runner version %S/Win%d (%S)\n",
            PRG_VERSION, PRG_BITS, __DATE__);
    fwprintf(stderr, L"Copyright (c) 2000-2024 The Apache Software Foundation.\n\n"
                     L"For bug reporting instructions, please see:\n"
                     L"<URL:https://issues.apache.org/jira/browse/DAEMON>.");
}

/* Displays command line parameters. */
static void dumpCmdline()
{
    int i = 0;
    while (_options[i].szName) {
        if (_options[i].dwType & APXCMDOPT_INT)
            fwprintf(stderr, L"--%-16s %d\n", _options[i].szName,
                     _options[i].dwValue);
        else if (_options[i].szValue)
            fwprintf(stderr, L"--%-16s %s\n", _options[i].szName,
                     _options[i].szValue);
        else
            fwprintf(stderr, L"--%-16s <NULL>\n", _options[i].szName);
        ++i;
    }
}

// TODO: Figure out a way to move apxSetInprocEnvironment from here and
// prunmgr.c to utils.c
void apxSetInprocEnvironment()
{
    LPWSTR p, e;
    HMODULE hmodUcrt;
    WPUTENV wputenv_ucrt = NULL;

    if (!SO_ENVIRONMENT)
        return;    /* Nothing to do */

    hmodUcrt = LoadLibraryExA("ucrtbase.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (hmodUcrt != NULL) {
        wputenv_ucrt =  (WPUTENV) GetProcAddress(hmodUcrt, "_wputenv");
    }

    for (p = SO_ENVIRONMENT; *p; p++) {
        e = apxExpandStrW(gPool, p);
        _wputenv(e);
        if (wputenv_ucrt != NULL) {
            wputenv_ucrt(e);
        }
        apxFree(e);
        while (*p)
            p++;
    }
}

static void setInprocEnvironment9(LPCWSTR szOptions9)
{
    DWORD l, c;
    LPWSTR e, b;
    LPCWSTR p;

    l = __apxGetMultiSzLengthW(szOptions9, &c);

    if (!c)
        return;

    /* environment variable name */
    l += lstrlen(PRSV_JVMOPTS9);

    b = e = apxPoolCalloc(gPool, (l + 1) * sizeof(WCHAR));

    p = PRSV_JVMOPTS9;
    while (*p) {
        *b++ = *p++;
    }

    p = szOptions9;
    while (c > 0) {
        if (*p)
            *b++ = *p;
        else {
            *b++ = L' ';
            c--;
        }
        p++;
    }

    _wputenv(e);
    apxFree(e);
}

/* Sets environment variables required by some Java options
 * Currently only Native Memory Tracking.
 */
static void setInprocEnvironmentOptions(LPCWSTR szOptions)
{
    LPCWSTR p;
    DWORD len;
    LPWSTR e;

    apxLogWrite(APXLOG_MARK_DEBUG "Checking Java options for environment variable requirements");

    p = szOptions;
    while (*p) {
        apxLogWrite(APXLOG_MARK_DEBUG "Checking environment variable requirements for '%S'", p);
        if (wcsncmp(p, L"-XX:NativeMemoryTracking=", 25) == 0) {
            apxLogWrite(APXLOG_MARK_DEBUG "Match found '%S'", p);
            /* Advance 25 characters to the start of the value */
            p += 25;
            apxLogWrite(APXLOG_MARK_DEBUG "Setting is '%S'", p);
            /* Ignore setting if it is off */
            if (wcsncmp(p, L"off", 3)) {
                apxLogWrite(APXLOG_MARK_DEBUG "Creating environment entry");
                /* Allocated space for the setting value */
                len = lstrlen(p);
                /* Expand space to include env var name less pid and '=' */
                len += 11;
                /* Expand spave to include pid (4 bytes, signed - up to 10 characters */
                len += 10;
                /* Expand space to include the null terminator */
                len++;

                /* Allocate the space */
                e = apxPoolCalloc(gPool, len * sizeof(WCHAR));

                /* Create the environment variable needed by NMT */
                swprintf_s(e, len, L"NMT_LEVEL_%d=%s", GetCurrentProcessId(), p);

                apxLogWrite(APXLOG_MARK_DEBUG "Created environment entry '%S'", e);
                /* Set the environment variable */
               _wputenv(e);

               apxFree(e);
            }
            return;
        }

        /* advance to the terminating null */
        while(*p) {
          p++;
        }

        /* advance to the start of the next entry
         * will be null if there are no more entries
         */
        p++;
    }
}

/* Load the configuration from Registry
 * loads only nonspecified items.
 */
static BOOL loadConfiguration(LPAPXCMDLINE lpCmdline)
{
    APXHANDLE hRegistry;
    int i = 0;

    if (!lpCmdline->szApplication) {
        /* Handle empty service names */
        apxLogWrite(APXLOG_MARK_WARN "No service name provided.");
        return FALSE;
    }
    SetLastError(ERROR_SUCCESS);
    hRegistry = apxCreateRegistryW(gPool, KEY_READ | KREG_WOW6432,
                                   PRG_REGROOT,
                                   lpCmdline->szApplication,
                                   APXREG_SOFTWARE | APXREG_SERVICE);
    if (IS_INVALID_HANDLE(hRegistry)) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            apxLogWrite(APXLOG_MARK_WARN "The system cannot find the Registry key for service '%S'.",
                        lpCmdline->szApplication);
        else
            apxLogWrite(APXLOG_MARK_SYSERR);
        return FALSE;
    }
    /* browse through options */
    while (_options[i].szName) {
        DWORD dwFrom;

        dwFrom = (_options[i].dwType & APXCMDOPT_REG) ? APXREG_PARAMSOFTWARE : APXREG_SERVICE;
        if (!(_options[i].dwType & APXCMDOPT_FOUND)) {
            if (_options[i].dwType & APXCMDOPT_STR) {
                _options[i].szValue = apxRegistryGetStringW(hRegistry,
                                                            dwFrom,
                                                            _options[i].szSubkey,
                                                            _options[i].szRegistry);
                /* Expand environment variables */
                if (_options[i].szValue && (_options[i].dwType & APXCMDOPT_STE)) {
                    LPWSTR exp = apxExpandStrW(gPool, _options[i].szValue);
                    if (exp != _options[i].szValue)
                        apxFree(_options[i].szValue);
                    _options[i].szValue = exp;
                }
            }
            else if (_options[i].dwType & APXCMDOPT_INT) {
                _options[i].dwValue = apxRegistryGetNumberW(hRegistry,
                                                            dwFrom,
                                                            _options[i].szSubkey,
                                                            _options[i].szRegistry);
            }
            else if (_options[i].dwType & APXCMDOPT_MSZ) {
                _options[i].szValue = apxRegistryGetMzStrW(hRegistry,
                                                           dwFrom,
                                                           _options[i].szSubkey,
                                                           _options[i].szRegistry,
                                                           NULL,
                                                           &(_options[i].dwValue));
            }
        }
        /* Merge the command line options with registry */
        else if (_options[i].dwType & APXCMDOPT_ADD) {
            LPWSTR cv = _options[i].szValue;
            LPWSTR ov = NULL;
            if (_options[i].dwType & APXCMDOPT_MSZ) {
                ov = apxRegistryGetMzStrW(hRegistry, dwFrom,
                                          _options[i].szSubkey,
                                          _options[i].szRegistry,
                                          NULL,
                                          &(_options[i].dwValue));
                _options[i].szValue = apxMultiSzCombine(gPool, ov, cv,
                                                        &(_options[i].dwValue));
                if (ov)
                    apxFree(ov);
            }
        }
        ++i;
    }
    apxCloseHandle(hRegistry);
#ifdef _DEBUG
    dumpCmdline();
#endif
    return TRUE;
}

/* Saves changed configuration to registry.
 */
static BOOL saveConfiguration(LPAPXCMDLINE lpCmdline)
{
    APXHANDLE hRegistry;
    int i = 0;
    hRegistry = apxCreateRegistryW(gPool, KEY_WRITE | KREG_WOW6432,
                                   PRG_REGROOT,
                                   lpCmdline->szApplication,
                                   APXREG_SOFTWARE | APXREG_SERVICE);
    if (IS_INVALID_HANDLE(hRegistry)) {
        apxLogWrite(APXLOG_MARK_WARN "Can't save configuration: Invalid registry handle.");
        return FALSE;
    }
    /* TODO: Use array size */
    while (_options[i].szName) {
        /* Skip the service params */
        if ((_options[i].dwType & APXCMDOPT_SRV) ||
            !(_options[i].dwType & APXCMDOPT_FOUND)) {
                /* Skip non-modified version */
        }
        /* Update only modified params */
        else if (_options[i].dwType & APXCMDOPT_STR)
            apxRegistrySetStrW(hRegistry, APXREG_PARAMSOFTWARE,
                               _options[i].szSubkey,
                               _options[i].szRegistry,
                               _options[i].szValue);
        else if (_options[i].dwType & APXCMDOPT_INT)
            apxRegistrySetNumW(hRegistry, APXREG_PARAMSOFTWARE,
                               _options[i].szSubkey,
                               _options[i].szRegistry,
                               _options[i].dwValue);
        else if (_options[i].dwType & APXCMDOPT_MSZ)
            apxRegistrySetMzStrW(hRegistry, APXREG_PARAMSOFTWARE,
                               _options[i].szSubkey,
                               _options[i].szRegistry,
                               _options[i].szValue,
                               _options[i].dwValue);
        ++i;
    }
    apxCloseHandle(hRegistry);
    return TRUE;
}

/* Display current configuration. */
static BOOL printConfig(LPAPXCMDLINE lpCmdline)
{
    int i = 0;

    if (!loadConfiguration(lpCmdline)) {
		return FALSE;
	}

    fwprintf(stderr, L"%s.exe update ",lpCmdline->szExecutable);
    while (_options[i].szName) {
		if (_options[i].dwType & APXCMDOPT_INT) {
			fwprintf(stderr, L"--%s %d ", _options[i].szName, _options[i].dwValue);
		} else if (_options[i].dwType & APXCMDOPT_MSZ) {
			if (_options[i].szValue) {
				BOOL first = TRUE;
				LPCWSTR p = _options[i].szValue;
    	        fwprintf(stderr, L"--%s \"", _options[i].szName);

    	        while (*p) {
    	        	if (first) {
    	        		first = FALSE;
    	        	} else {
    	        		fwprintf(stderr, L"#");
    	        	}
    	        	// Skip to terminating NULL for this value
    	        	while (p[0]) {
    	        		if (p[0] == L'#') {
    	        			fwprintf(stderr, L"'%c'", p[0]);
    	        		} else {
    	        			fwprintf(stderr, L"%c", p[0]);
    	        		}
    	        		++p;
    	        	}
    	        	// Move to start of next value
    	        	++p;
    	        }
    	        fwprintf(stderr, L"\" ");
			}
        } else if (_options[i].szValue) {
            fwprintf(stderr, L"--%s \"%s\" ", _options[i].szName, _options[i].szValue);
        }
        // NULL options are ignored
        ++i;
    }
    return TRUE;
}

/* Common error reporting */
static void logGrantFileAccessFail(LPCWSTR szUser, LPCWSTR szPath, DWORD dwError)
{
    int     len = 0;
    CHAR    errmsg[SIZ_HUGLEN];
    WCHAR sPath[SIZ_PATHLEN];

    /* 
     * Both user and path may be null indicating that the default should be
     * used. Strictly, the default for path isn't fixed so it needs to be
     * calculated. The default for user is a constant so use that directly.
     */
    if (!szPath) {
        if (GetSystemDirectoryW(sPath, MAX_PATH) == 0) {
            lstrlcpyW(sPath, MAX_PATH, L"%windir%\\system32");
        }
        lstrlcatW(sPath, MAX_PATH, LOG_PATH_DEFAULT);
    }
    else {
        lstrlcpyW(sPath, MAX_PATH, szPath);
    }

    errmsg[0] = '\0';
    if (dwError != ERROR_SUCCESS) {
        len = apxGetMessage(dwError, errmsg, SIZ_DESLEN);
        errmsg[len] = '\0';
        if (len > 0) {
            if (errmsg[len - 1] == '\n')
                errmsg[--len] = '\0';
            if (len > 0 && errmsg[len - 1] == '\r')
                errmsg[--len] = '\0';
        }
    }

    apxLogWrite(APXLOG_MARK_WARN "Failed to grant service user '%S' write permissions to log path '%S' due to error '%d: %s'",
                (szUser ? szUser : DEFAULT_SERVICE_USER), sPath, dwError, errmsg);
}

/* Operations */
static BOOL docmdInstallService(LPAPXCMDLINE lpCmdline)
{
    APXHANDLE hService;
    BOOL  rv;
    BOOL  bDelayedStart = FALSE;
    DWORD dwStart = SERVICE_DEMAND_START;
    DWORD dwType  = SERVICE_WIN32_OWN_PROCESS;
    WCHAR szImage[SIZ_HUGLEN];
    WCHAR szName[SIZ_BUFLEN];

    apxLogWrite(APXLOG_MARK_DEBUG "Installing service...");
    hService = apxCreateService(gPool, SC_MANAGER_CREATE_SERVICE, FALSE);
    if (IS_INVALID_HANDLE(hService)) {
        apxLogWrite(APXLOG_MARK_ERROR "Unable to open the Service Manager.");
        return FALSE;
    }
    /* Check the startup mode */
    if (ST_STARTUP & APXCMDOPT_FOUND) {
    	if (lstrcmpiW(SO_STARTUP, PRSRV_AUTO) == 0) {
    		dwStart = SERVICE_AUTO_START;
    	} else if (lstrcmpiW(SO_STARTUP, PRSRV_DELAYED) == 0) {
    		dwStart = SERVICE_AUTO_START;
    		bDelayedStart = TRUE;
    	}
    }
    /* Check the service type */
    if ((ST_TYPE & APXCMDOPT_FOUND) &&
        lstrcmpiW(SO_TYPE, STYPE_INTERACTIVE) == 0) {

        // Need to run as LocalSystem to set the interactive flag
        LPCWSTR su = NULL;
        if (ST_SUSER & APXCMDOPT_FOUND) {
            su = SO_SUSER;
        }
        if (su && lstrcmpiW(su, L"LocalSystem") == 0) {
            dwType |= SERVICE_INTERACTIVE_PROCESS;
        } else {
            apxLogWrite(APXLOG_MARK_ERROR
                    "The parameter '--Type interactive' is only valid with '--ServiceUser LocalSystem'");
            return FALSE;
        }
    }

    /* Check if --Install is provided */
    if (!IS_VALID_STRING(SO_INSTALL)) {
        lstrlcpyW(szImage, SIZ_HUGLEN, lpCmdline->szExePath);
        lstrlcatW(szImage, SIZ_HUGLEN, L"\\");
        lstrlcatW(szImage, SIZ_HUGLEN, lpCmdline->szExecutable);
        lstrlcatW(szImage, SIZ_HUGLEN, L".exe");
    }
    else
        lstrlcpyW(szImage, SIZ_HUGLEN, SO_INSTALL);
    /* Replace not needed quotes */
    apxStrQuoteInplaceW(szImage);
    /* Add run-service command line option */
    lstrlcatW(szImage, SIZ_HUGLEN, L" ");
    lstrlcpyW(szName, SIZ_BUFLEN, L"//RS//");
    lstrlcatW(szName, SIZ_BUFLEN, lpCmdline->szApplication);
    apxStrQuoteInplaceW(szName);
    lstrlcatW(szImage, SIZ_HUGLEN, szName);
    SO_INSTALL = apxPoolStrdupW(gPool, szImage);
    /* Ensure that option gets saved in the registry */
    ST_INSTALL |= APXCMDOPT_FOUND;
#ifdef _DEBUG
    /* Display configured options */
    dumpCmdline();
#endif
    apxLogWrite(APXLOG_MARK_INFO "Installing service '%S' name '%S'.", lpCmdline->szApplication,
                SO_DISPLAYNAME);
    rv = apxServiceInstall(hService,
                          lpCmdline->szApplication,
                          SO_DISPLAYNAME,    /* --DisplayName  */
                          SO_INSTALL,
                          SO_DEPENDSON,      /* --DependendsOn */
                          dwType,
                          dwStart);
    /* Configure as delayed start */
    if (rv & bDelayedStart) {
    	if (!apxServiceSetOptions(hService,
    	                          NULL,
                                  dwType,
                                  dwStart,
                                  bDelayedStart,
                                  SERVICE_NO_CHANGE)) {
            apxLogWrite(APXLOG_MARK_WARN "Failed to configure service for delayed startup");
        }
    }
    /* Set the --Description */
    if (rv) {
        LPCWSTR sd = NULL;
        LPCWSTR su = NULL;
        LPCWSTR sp = NULL;
        DWORD dwResult;

        if (ST_DESCRIPTION & APXCMDOPT_FOUND) {
            sd = SO_DESCRIPTION;
            apxLogWrite(APXLOG_MARK_DEBUG "Setting service description '%S'.",
                        SO_DESCRIPTION);
        }
        if (ST_SUSER & APXCMDOPT_FOUND) {
            su = SO_SUSER;
            apxLogWrite(APXLOG_MARK_DEBUG "Setting service user '%S'.",
                        SO_SUSER);
        }
        if (ST_SPASSWORD & APXCMDOPT_FOUND) {
            sp = SO_SPASSWORD;
            apxLogWrite(APXLOG_MARK_DEBUG "Setting service password '%S'.",
                        SO_SPASSWORD);
        }
        apxServiceSetNames(hService, NULL, NULL, sd, su, sp);
        dwResult = apxSecurityGrantFileAccessToUser(SO_LOGPATH, su);
        if (dwResult) {
            logGrantFileAccessFail(su, SO_LOGPATH, dwResult);
        }
    }
    apxCloseHandle(hService);
    if (rv) {
        saveConfiguration(lpCmdline);
        apxLogWrite(APXLOG_MARK_INFO "Service '%S' installed.",
                    lpCmdline->szApplication);
    }
    else
        apxLogWrite(APXLOG_MARK_ERROR "Failed installing service '%S'.",
                    lpCmdline->szApplication);

    return rv;
}

static BOOL docmdDeleteService(LPAPXCMDLINE lpCmdline)
{
    APXHANDLE hService;
    BOOL  rv = FALSE;

    apxLogWrite(APXLOG_MARK_INFO "Deleting service...");
    hService = apxCreateService(gPool, SC_MANAGER_CONNECT, FALSE);
    if (IS_INVALID_HANDLE(hService)) {
        apxLogWrite(APXLOG_MARK_ERROR "Unable to open the Service Manager.");
        return FALSE;
    }
    /* Delete service will stop the service if running */
    if (apxServiceOpen(hService, lpCmdline->szApplication, SERVICE_ALL_ACCESS)) {
        WCHAR szWndManagerClass[SIZ_RESLEN];
        HANDLE hWndManager = NULL;
        lstrlcpyW(szWndManagerClass, SIZ_RESLEN, lpCmdline->szApplication);
        lstrlcatW(szWndManagerClass, SIZ_RESLEN, L"_CLASS");
        /* Close the monitor application if running */
        if ((hWndManager = FindWindowW(szWndManagerClass, NULL)) != NULL) {
            SendMessage(hWndManager, WM_CLOSE, 0, 0);
        }
        rv = apxServiceDelete(hService);
    }
    if (rv) {
        /* Delete all service registry settings */
        apxDeleteRegistryW(PRG_REGROOT, lpCmdline->szApplication, KREG_WOW6432, TRUE);
        apxLogWrite(APXLOG_MARK_DEBUG "Service '%S' deleted.",
                    lpCmdline->szApplication);
    }
    else {
        apxDisplayError(FALSE, NULL, 0, "Unable to delete service '%S'.",
                        lpCmdline->szApplication);
    }
    apxCloseHandle(hService);
    apxLogWrite(APXLOG_MARK_INFO "Delete service finished.");
    return rv;
}

static BOOL docmdStopService(LPAPXCMDLINE lpCmdline)
{
    APXHANDLE hService;
    BOOL  rv = FALSE;

    apxLogWrite(APXLOG_MARK_INFO "Stopping service '%S'...",
                lpCmdline->szApplication);
    hService = apxCreateService(gPool, GENERIC_ALL, FALSE);
    if (IS_INVALID_HANDLE(hService)) {
        apxLogWrite(APXLOG_MARK_ERROR "Unable to open the Service Manager.");
        return FALSE;
    }

    SetLastError(ERROR_SUCCESS);
    /* Open the service */
    if (apxServiceOpen(hService, lpCmdline->szApplication,
                       GENERIC_READ | GENERIC_EXECUTE)) {
        rv = apxServiceControl(hService,
                               SERVICE_CONTROL_STOP,
                               0,
                               NULL,
                               NULL);
        if (!rv) {
            /* Wait for the timeout if any */
            int  timeout     = SO_STOPTIMEOUT;
            if (timeout) {
                int i;
                for (i = 0; i < timeout; i++) {
                    rv = apxServiceCheckStop(hService);
                    apxLogWrite(APXLOG_MARK_DEBUG "apxServiceCheck returns %d.", rv);
                    if (rv)
                        break;
                }
            }
        }
        if (rv)
            apxLogWrite(APXLOG_MARK_INFO "Service '%S' stopped.",
                        lpCmdline->szApplication);
        else
            apxLogWrite(APXLOG_MARK_ERROR "Failed to stop service '%S'.",
                        lpCmdline->szApplication);
    }
    else
        apxDisplayError(FALSE, NULL, 0, "Unable to open service '%S'.",
                        lpCmdline->szApplication);
    apxCloseHandle(hService);
    apxLogWrite(APXLOG_MARK_INFO "Stop service finished.");
    return rv;
}

static BOOL docmdStartService(LPAPXCMDLINE lpCmdline)
{
    APXHANDLE hService;
    BOOL  rv = FALSE;

    apxLogWrite(APXLOG_MARK_INFO "Starting service '%S'...",
                lpCmdline->szApplication);
    hService = apxCreateService(gPool, GENERIC_ALL, FALSE);
    if (IS_INVALID_HANDLE(hService)) {
        apxLogWrite(APXLOG_MARK_ERROR "Unable to open the Service Manager.");
        return FALSE;
    }

    SetLastError(ERROR_SUCCESS);
    /* Open the service */
    if (apxServiceOpen(hService, lpCmdline->szApplication,
                       GENERIC_READ | GENERIC_EXECUTE)) {
        rv = apxServiceControl(hService,
                               SERVICE_CONTROL_CONTINUE,
                               0,
                               NULL,
                               NULL);
        if (rv)
            apxLogWrite(APXLOG_MARK_INFO "Started service '%S'.",
                        lpCmdline->szApplication);
        else
            apxLogWrite(APXLOG_MARK_ERROR "Failed to start service '%S'.",
                        lpCmdline->szApplication);

    }
    else
        apxDisplayError(FALSE, NULL, 0, "Unable to open service '%S'.",
                        lpCmdline->szApplication);
    apxCloseHandle(hService);
    apxLogWrite(APXLOG_MARK_INFO "Finished starting service '%S', returning %d.", lpCmdline->szApplication, rv);
    return rv;
}

static BOOL docmdUpdateService(LPAPXCMDLINE lpCmdline)
{
    APXHANDLE hService;
    BOOL  rv = TRUE;

    apxLogWrite(APXLOG_MARK_INFO "Updating service...");

    hService = apxCreateService(gPool, SC_MANAGER_CREATE_SERVICE, FALSE);
    if (IS_INVALID_HANDLE(hService)) {
        apxLogWrite(APXLOG_MARK_ERROR "Unable to open the Service Manager.");
        return FALSE;
    }
    SetLastError(0);
    /* Open the service */
    if (!apxServiceOpen(hService, lpCmdline->szApplication, SERVICE_ALL_ACCESS)) {
        /* Close the existing manager handler.
         * It will be reopened inside install.
         */
        apxCloseHandle(hService);
        /* In case service doesn't exist try to install it.
         * Install will fail if there is no minimum parameters required.
         */
        return docmdInstallService(lpCmdline);
    }
    else {
        DWORD dwStart = SERVICE_NO_CHANGE;
        DWORD dwResult;
        BOOL  bDelayedStart = FALSE;
        DWORD dwType  = SERVICE_NO_CHANGE;
        LPCWSTR su = NULL;
        LPCWSTR sp = NULL;
        if (ST_SUSER & APXCMDOPT_FOUND) {
            su = SO_SUSER;
            apxLogWrite(APXLOG_MARK_DEBUG "Setting service user '%S'.",
                        SO_SUSER);
        }
        if (ST_SPASSWORD & APXCMDOPT_FOUND) {
            sp = SO_SPASSWORD;
            apxLogWrite(APXLOG_MARK_DEBUG "Setting service password '%S'.",
                        SO_SPASSWORD);
        }
        rv = (rv && apxServiceSetNames(hService,
                                       NULL,                /* Never update the ImagePath */
                                       SO_DISPLAYNAME,
                                       SO_DESCRIPTION,
                                       su,
                                       sp));
        dwResult = apxSecurityGrantFileAccessToUser(SO_LOGPATH, su);
        if (dwResult) {
            logGrantFileAccessFail(su, SO_LOGPATH, dwResult);
        }
        /* Update the --Startup mode */
        if (ST_STARTUP & APXCMDOPT_FOUND) {
            if (!lstrcmpiW(SO_STARTUP, PRSRV_DELAYED)) {
                dwStart = SERVICE_DEMAND_START;
                bDelayedStart = TRUE;
            }
            else if (!lstrcmpiW(SO_STARTUP, PRSRV_AUTO))
                dwStart = SERVICE_AUTO_START;
            else if (!lstrcmpiW(SO_STARTUP, PRSRV_MANUAL))
                dwStart = SERVICE_DEMAND_START;
        }
        if (ST_TYPE & APXCMDOPT_FOUND) {
            if (!lstrcmpiW(SO_TYPE, STYPE_INTERACTIVE))
                dwType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;
        }
        rv = (rv && apxServiceSetOptions(hService,
                                         SO_DEPENDSON,
                                         dwType,
                                         dwStart,
                                         bDelayedStart,
                                         SERVICE_NO_CHANGE));

        apxLogWrite(APXLOG_MARK_INFO "Updated service '%S'.",
                    lpCmdline->szApplication);

        rv = (rv && saveConfiguration(lpCmdline));
    }
    apxCloseHandle(hService);
    if (rv)
        apxLogWrite(APXLOG_MARK_INFO "Finished updating service '%S'.", lpCmdline->szApplication);
    else
        apxLogWrite(APXLOG_MARK_INFO "Failed updating service '%S'.",
                                     lpCmdline->szApplication);
    return rv;
}

/* Report the service status to the SCM, including service specific exit code.
 *
 * dwCurrentState
 * https://docs.microsoft.com/en-us/windows/win32/api/winsvc/ns-winsvc-service_status
 *
 * SERVICE_CONTINUE_PENDING  0x00000005 The service continue is pending.
 * SERVICE_PAUSE_PENDING     0x00000006 The service pause is pending.
 * SERVICE_PAUSED            0x00000007 The service is paused.
 * SERVICE_RUNNING           0x00000004 The service is running.
 * SERVICE_START_PENDING     0x00000002 The service is starting.
 * SERVICE_STOP_PENDING      0x00000003 The service is stopping.
 * SERVICE_STOPPED           0x00000001 The service is not running.
 */
static BOOL reportServiceStatusE(DWORD dwLevel,
                                 DWORD dwCurrentState,
                                 DWORD dwWin32ExitCode,
                                 DWORD dwWaitHint,
                                 DWORD dwServiceSpecificExitCode)
{
   static DWORD dwCheckPoint = 1;
   BOOL fResult = TRUE;

   apxLogWrite(NULL, dwLevel, TRUE, __FILE__, __LINE__, __func__,
       "reportServiceStatusE: dwCurrentState = %d (%s), dwWin32ExitCode = %d, dwWaitHint = %d milliseconds, dwServiceSpecificExitCode = %d.",
       dwCurrentState, apxServiceGetStateName(dwCurrentState), dwWin32ExitCode, dwWaitHint, dwServiceSpecificExitCode);

   if (_service_mode && _service_status_handle) {
       if (dwCurrentState == SERVICE_RUNNING)
            _service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
        else
            _service_status.dwControlsAccepted = 0;

       _service_status.dwCurrentState  = dwCurrentState;
       _service_status.dwWin32ExitCode = dwWin32ExitCode;
       _service_status.dwWaitHint      = dwWaitHint;
       _service_status.dwServiceSpecificExitCode = dwServiceSpecificExitCode;

       if ((dwCurrentState == SERVICE_RUNNING) ||
           (dwCurrentState == SERVICE_STOPPED))
           _service_status.dwCheckPoint = 0;
       else
           _service_status.dwCheckPoint = dwCheckPoint++;
       fResult = SetServiceStatus(_service_status_handle, &_service_status);
       if (!fResult) {
           /* TODO: Deal with error */
           apxLogWrite(APXLOG_MARK_ERROR "Failed to set service status.");
       }
   }
   return fResult;
}

/* Report the service status to the SCM.
 */
static BOOL reportServiceStatus(DWORD dwCurrentState,
                                DWORD dwWin32ExitCode,
                                DWORD dwWaitHint)
{
    // exit code 0
    return reportServiceStatusE(APXLOG_LEVEL_DEBUG, dwCurrentState, dwWin32ExitCode, dwWaitHint, 0);
}

/* Report SERVICE_STOPPED to the SCM.
 */
static BOOL reportServiceStatusStopped(DWORD exitCode)
{
    return reportServiceStatusE(APXLOG_LEVEL_DEBUG, SERVICE_STOPPED, exitCode ? ERROR_SERVICE_SPECIFIC_ERROR : NO_ERROR, 0, exitCode);
}

BOOL child_callback(APXHANDLE hObject, UINT uMsg,
                    WPARAM wParam, LPARAM lParam)
{
    /* TODO: Make stdout and stderr buffers
     * to prevent streams intermixing when there
     * is no separate file for each stream.
     */
    if (uMsg == WM_CHAR) {
        int ch = LOWORD(wParam);
        if (lParam)
            fputc(ch, stderr);
        else
            fputc(ch, stdout);
    }
    return TRUE;
    UNREFERENCED_PARAMETER(hObject);
}

static int onExitStop(void)
{
    if (_service_mode) {
        apxLogWrite(APXLOG_MARK_DEBUG "Stop exit hook called...");
        reportServiceStatusStopped(0);
    }
    return 0;
}

static int onExitStart(void)
{
    if (_service_mode) {
        apxLogWrite(APXLOG_MARK_DEBUG "Start exit hook called...");
        apxLogWrite(APXLOG_MARK_DEBUG "JVM exit code: %d.", apxGetVmExitCode());
        /* Reporting the service as stopped even with a non-zero exit code
         * will not cause recovery actions to be initiated, so don't report at all.
         * "A service is considered failed when it terminates without reporting a
         * status of SERVICE_STOPPED to the service controller"
         * http://msdn.microsoft.com/en-us/library/ms685939(VS.85).aspx
         */
        if (apxGetVmExitCode() == 0) {
            reportServiceStatusStopped(0);
        }
    }
    return 0;
}

/* Executed when the service receives stop event. */
static DWORD WINAPI serviceStop(LPVOID lpParameter)
{
    APXHANDLE hWorker = NULL;
    DWORD  rv = 0;
    BOOL   wait_to_die = FALSE;
    DWORD  timeout     = SO_STOPTIMEOUT * 1000;
    DWORD  dwCtrlType  = (DWORD)((BYTE *)lpParameter - (BYTE *)0);

    apxLogWrite(APXLOG_MARK_INFO "Stopping service...");

    if (IS_INVALID_HANDLE(gWorker)) {
        apxLogWrite(APXLOG_MARK_INFO "Worker is not defined.");
        return TRUE;    /* Nothing to do */
    }
    if (timeout > 0x7FFFFFFF)
        timeout = INFINITE;     /* If the timeout was '-1' wait forewer */
    if (_jni_shutdown) {
        if (!IS_VALID_STRING(SO_STARTPATH) && IS_VALID_STRING(SO_STOPPATH)) {
            /* If the Working path is specified change the current directory
             * but only if the start path wasn't specified already.
             */
            SetCurrentDirectoryW(SO_STOPPATH);
        }
        hWorker = apxCreateJava(gPool, _jni_jvmpath, SO_JAVAHOME);
        if (IS_INVALID_HANDLE(hWorker)) {
            apxLogWrite(APXLOG_MARK_ERROR "Failed creating Java '%S'.", _jni_jvmpath);
            return 1;
        }
        gSargs.hJava            = hWorker;
        gSargs.szClassPath      = _jni_classpath;
        gSargs.lpOptions        = _jni_jvmoptions;
        gSargs.lpOptions9       = _jni_jvmoptions9;
        gSargs.dwMs             = SO_JVMMS;
        gSargs.dwMx             = SO_JVMMX;
        gSargs.dwSs             = SO_JVMSS;
        gSargs.bJniVfprintf     = SO_JNIVFPRINTF;
        gSargs.szClassName      = _jni_sclass;
        gSargs.szMethodName     = _jni_smethod;
        gSargs.lpArguments      = _jni_sparam;
        gSargs.szStdErrFilename = NULL;
        gSargs.szStdOutFilename = NULL;
        gSargs.szLibraryPath    = SO_LIBPATH;
        /* Register onexit hook
         */
        _onexit(onExitStop);
        /* Create shutdown event */
        gShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!apxJavaStart(&gSargs)) {
            apxLogWrite(APXLOG_MARK_ERROR "Failed starting Java.");
            rv = 3;
        }
        else {
            if (lstrcmpA(_jni_sclass, "java/lang/System") == 0) {
                reportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 20 * 1000);
                apxLogWrite(APXLOG_MARK_DEBUG "Forcing Java JNI System.exit() worker to finish...");
                return 0;
            }
            else {
                apxLogWrite(APXLOG_MARK_DEBUG "Waiting for Java JNI stop worker to finish for %s:%s...", _jni_sclass, _jni_smethod);
                if (!timeout)
                    apxJavaWait(hWorker, INFINITE, FALSE);
                else
                    apxJavaWait(hWorker, timeout, FALSE);
                apxLogWrite(APXLOG_MARK_DEBUG "Java JNI stop worker finished.");
            }
        }
        wait_to_die = TRUE;
    }
    else if (IS_VALID_STRING(SO_STOPMODE)) { /* Only in case we have a stop mode */
        DWORD nArgs;
        LPWSTR *pArgs;

        if (!IS_VALID_STRING(SO_STOPIMAGE)) {
            apxLogWrite(APXLOG_MARK_ERROR "Missing service ImageFile.");
            if (!_service_mode)
                apxDisplayError(FALSE, NULL, 0, "Service '%S' is missing the ImageFile.",
                                _service_name ? _service_name : L"unknown");
            return 1;
        }
        /* Redirect process */
        hWorker = apxCreateProcessW(gPool,
                                    0,
                                    child_callback,
                                    SO_USER,
                                    SO_PASSWORD,
                                    FALSE);
        if (IS_INVALID_HANDLE(hWorker)) {
            apxLogWrite(APXLOG_MARK_ERROR "Failed creating process.");
            return 1;
        }
        /* If the service process completes before the stop process does the
         * cleanup code below will free structures required by the stop process
         * which will, in all probability, trigger a crash. Wait for the stop
         * process to complete before cleaning up.
         */
        gShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!apxProcessSetExecutableW(hWorker, SO_STOPIMAGE)) {
            apxLogWrite(APXLOG_MARK_ERROR "Failed setting process executable '%S'.",
                        SO_STOPIMAGE);
            rv = 2;
            goto cleanup;
        }
        /* Assemble the command line */
        if (_java_shutdown) {
            nArgs = apxJavaCmdInitialize(gPool, SO_CLASSPATH, SO_STOPCLASS,
                                         SO_JVMOPTIONS, SO_JVMMS, SO_JVMMX,
                                         SO_JVMSS, SO_STOPPARAMS, &pArgs);
        }
        else {
            nArgs = apxMultiSzToArrayW(gPool, SO_STOPPARAMS, &pArgs);
        }

        /* Pass the argv to child process */
        if (!apxProcessSetCommandArgsW(hWorker, SO_STOPIMAGE,
                                       nArgs, pArgs)) {
            rv = 3;
            apxLogWrite(APXLOG_MARK_ERROR "Failed setting process arguments (argc=%d).",
                        nArgs);
            goto cleanup;
        }
        /* Set the working path */
        if (!apxProcessSetWorkingPathW(hWorker, SO_STOPPATH)) {
            rv = 4;
            apxLogWrite(APXLOG_MARK_ERROR "Failed setting process working path to '%S'.",
                        SO_STOPPATH);
            goto cleanup;
        }
        /* Finally execute the child process
         */
        if (!apxProcessExecute(hWorker)) {
            rv = 5;
            apxLogWrite(APXLOG_MARK_ERROR "Failed executing process.");
            goto cleanup;
        } else {
            apxLogWrite(APXLOG_MARK_DEBUG "Waiting for stop worker to finish...");
            if (!timeout)
                apxHandleWait(hWorker, INFINITE, FALSE);
            else
                apxHandleWait(hWorker, timeout, FALSE);
            apxLogWrite(APXLOG_MARK_DEBUG "Stop worker finished.");
        }
        wait_to_die = TRUE;
    }
cleanup:
    /* Close Java JNI handle or stop worker
     * If this is the single JVM instance it will unload
     * the JVM dll too.
     * The worker will be closed on service exit.
     */
    if (!IS_INVALID_HANDLE(hWorker))
        apxCloseHandle(hWorker);
    if (gSignalEvent) {
        gSignalValid = FALSE;
        SetEvent(gSignalEvent);
        WaitForSingleObject(gSignalThread, 1000);
        CloseHandle(gSignalEvent);
        CloseHandle(gSignalThread);
        gSignalEvent = NULL;
    }
    if (wait_to_die && !timeout)
        timeout = 300 * 1000;   /* Use the 5 minute default shutdown */

    if (dwCtrlType == SERVICE_CONTROL_SHUTDOWN)
        timeout = MIN(timeout, apxGetMaxServiceTimeout(gPool));
    reportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, timeout);

    if (timeout) {
        FILETIME fts, fte;
        ULARGE_INTEGER s, e;
        DWORD    nms;
        /* Wait to give it a chance to die naturally, then kill it. */
        apxLogWrite(APXLOG_MARK_DEBUG "Waiting for worker to die naturally...");
        GetSystemTimeAsFileTime(&fts);
        rv = apxHandleWait(gWorker, timeout, TRUE);
        GetSystemTimeAsFileTime(&fte);
        s.LowPart  = fts.dwLowDateTime;
        s.HighPart = fts.dwHighDateTime;
        e.LowPart  = fte.dwLowDateTime;
        e.HighPart = fte.dwHighDateTime;
        nms = (DWORD)((e.QuadPart - s.QuadPart) / 10000);
        if (rv == WAIT_OBJECT_0) {
            rv = 0;
            apxLogWrite(APXLOG_MARK_DEBUG "Worker finished gracefully in %d milliseconds.", nms);
        }
        else
            apxLogWrite(APXLOG_MARK_DEBUG "Worker was killed in %d milliseconds.", nms);
    }
    else {
        apxLogWrite(APXLOG_MARK_DEBUG "Sending WM_CLOSE to worker.");
        apxHandleSendMessage(gWorker, WM_CLOSE, 0, 0);
    }

    apxLogWrite(APXLOG_MARK_INFO "Service stop thread completed.");
    if (gShutdownEvent) {
        SetEvent(gShutdownEvent);
    }
    return rv;
}

/* Executed when the service receives start event. */
static DWORD serviceStart()
{
    DWORD  rv = 0;
    DWORD  nArgs;
    LPWSTR *pArgs;
    FILETIME fts;

    apxLogWrite(APXLOG_MARK_INFO "Starting service...");

    if (!IS_INVALID_HANDLE(gWorker)) {
        apxLogWrite(APXLOG_MARK_INFO "Worker is not defined.");
        return TRUE;    /* Nothing to do */
    }
    if (IS_VALID_STRING(SO_PIDFILE)) {
        gPidfileName = apxLogFile(gPool, SO_LOGPATH, SO_PIDFILE, NULL, FALSE, 0);
        if (GetFileAttributesW(gPidfileName) !=  INVALID_FILE_ATTRIBUTES) {
            /* Pid file exists */
            if (!DeleteFileW(gPidfileName)) {
                /* Delete failed. Either no access or opened */
                apxLogWrite(APXLOG_MARK_ERROR "Pid file '%S' exists.",
                            gPidfileName);
                return 1;
            }
        }
    }
    GetSystemTimeAsFileTime(&fts);
    if (_jni_startup) {
        if (IS_EMPTY_STRING(SO_STARTPATH))
            SO_STARTPATH = gStartPath;
        if (IS_VALID_STRING(SO_STARTPATH)) {
            /* If the Working path is specified change the current directory */
            SetCurrentDirectoryW(SO_STARTPATH);
        }
        if (IS_VALID_STRING(SO_LIBPATH)) {
            /* Add LibraryPath to the PATH */
           apxAddToPathW(gPool, SO_LIBPATH);
        }
        /* Some options require additional environment settings to be in place
         * before Java is started
         */
        if (IS_VALID_STRING(SO_JVMOPTIONS)) {
        	setInprocEnvironmentOptions(SO_JVMOPTIONS);
        }
        /* Create the JVM global worker */
        gWorker = apxCreateJava(gPool, _jni_jvmpath, SO_JAVAHOME);
        if (IS_INVALID_HANDLE(gWorker)) {
            apxLogWrite(APXLOG_MARK_ERROR "Failed creating Java '%S'.", _jni_jvmpath);
            return 1;
        }
        gRargs.hJava            = gWorker;
        gRargs.szClassPath      = _jni_classpath;
        gRargs.lpOptions        = _jni_jvmoptions;
        gRargs.lpOptions9       = _jni_jvmoptions9;
        gRargs.dwMs             = SO_JVMMS;
        gRargs.dwMx             = SO_JVMMX;
        gRargs.dwSs             = SO_JVMSS;
        gRargs.bJniVfprintf     = SO_JNIVFPRINTF;
        gRargs.szClassName      = _jni_rclass;
        gRargs.szMethodName     = _jni_rmethod;
        gRargs.lpArguments      = _jni_rparam;
        gRargs.szStdErrFilename = gStdwrap.szStdErrFilename;
        gRargs.szStdOutFilename = gStdwrap.szStdOutFilename;
        gRargs.szLibraryPath    = SO_LIBPATH;
        /* Register onexit hook
         */
        _onexit(onExitStart);
        if (!apxJavaStart(&gRargs)) {
            rv = 4;
            apxLogWrite(APXLOG_MARK_ERROR "Failed to start Java");
            goto cleanup;
        }
        apxLogWrite(APXLOG_MARK_DEBUG "Java started '%s'.", _jni_rclass);
    }
    else {
        if (!IS_VALID_STRING(SO_STARTIMAGE)) {
            apxLogWrite(APXLOG_MARK_ERROR "Missing service ImageFile.");
            if (!_service_mode)
                apxDisplayError(FALSE, NULL, 0, "Service '%S' is missing the ImageFile.",
                                _service_name ? _service_name : L"unknown");
            return 1;
        }
        if (IS_VALID_STRING(SO_LIBPATH)) {
            /* Add LibraryPath to the PATH */
           apxAddToPathW(gPool, SO_LIBPATH);
        }
        /* Set the environment using putenv, so JVM can use it */
        apxSetInprocEnvironment();
        /* Java 9 specific options need to be set via an environment variable */
        setInprocEnvironment9(SO_JVMOPTIONS9);
        /* Redirect process */
        gWorker = apxCreateProcessW(gPool,
                                    0,
                                    child_callback,
                                    SO_USER,
                                    SO_PASSWORD,
                                    FALSE);
        if (IS_INVALID_HANDLE(gWorker)) {
            apxLogWrite(APXLOG_MARK_ERROR "Failed to create process.");
            return 1;
        }
        if (!apxProcessSetExecutableW(gWorker, SO_STARTIMAGE)) {
            apxLogWrite(APXLOG_MARK_ERROR "Failed setting process executable '%S'.",
                        SO_STARTIMAGE);
            rv = 2;
            goto cleanup;
        }
        /* Assemble the command line */
        if (_java_startup) {
            nArgs = apxJavaCmdInitialize(gPool, SO_CLASSPATH, SO_STARTCLASS,
                                         SO_JVMOPTIONS, SO_JVMMS, SO_JVMMX,
                                         SO_JVMSS, SO_STARTPARAMS, &pArgs);
        }
        else {
            nArgs = apxMultiSzToArrayW(gPool, SO_STARTPARAMS, &pArgs);
        }

        /* Pass the argv to child process */
        if (!apxProcessSetCommandArgsW(gWorker, SO_STARTIMAGE,
                                       nArgs, pArgs)) {
            rv = 3;
            apxLogWrite(APXLOG_MARK_ERROR "Failed setting process arguments (argc=%d).",
                        nArgs);
            goto cleanup;
        }
        /* Set the working path */
        if (!apxProcessSetWorkingPathW(gWorker, SO_STARTPATH)) {
            rv = 4;
            apxLogWrite(APXLOG_MARK_ERROR "Failed setting process working path to '%S'.",
                        SO_STARTPATH);
            goto cleanup;
        }
        /* Finally execute the child process
         */
        if (!apxProcessExecute(gWorker)) {
            rv = 5;
            apxLogWrite(APXLOG_MARK_ERROR "Failed to execute process.");
            goto cleanup;
        }
    }
    if (rv == 0) {
        FILETIME fte;
        ULARGE_INTEGER s, e;
        DWORD    nms;
        /* Create pidfile */
        if (gPidfileName) {
            char pids[32];
            gPidfileHandle = CreateFileW(gPidfileName,
                                         GENERIC_READ | GENERIC_WRITE,
                                         FILE_SHARE_READ,
                                         NULL,
                                         CREATE_NEW,
                                         FILE_ATTRIBUTE_NORMAL,
                                         NULL);

            if (gPidfileHandle != INVALID_HANDLE_VALUE) {
                DWORD wr = 0;
                if (_jni_startup)
                    _snprintf_s(pids, _countof(pids), 32, "%d\r\n", GetCurrentProcessId());
                else
                    _snprintf_s(pids, _countof(pids), 32, "%d\r\n", apxProcessGetPid(gWorker));
                WriteFile(gPidfileHandle, pids, (DWORD)strlen(pids), &wr, NULL);
                FlushFileBuffers(gPidfileName);
            }
        }
        GetSystemTimeAsFileTime(&fte);
        s.LowPart  = fts.dwLowDateTime;
        s.HighPart = fts.dwHighDateTime;
        e.LowPart  = fte.dwLowDateTime;
        e.HighPart = fte.dwHighDateTime;
        nms = (DWORD)((e.QuadPart - s.QuadPart) / 10000);
        apxLogWrite(APXLOG_MARK_INFO "Service started in %d milliseconds.", nms);
    }
    return rv;
cleanup:
    if (!IS_INVALID_HANDLE(gWorker))
        apxCloseHandle(gWorker);    /* Close the worker handle */
    gWorker = NULL;
    return rv;
}

/* Service control handler.
 */
void WINAPI service_ctrl_handler(DWORD dwCtrlCode)
{
    DWORD  threadId;
    HANDLE stopThread;

    switch (dwCtrlCode) {
        case SERVICE_CONTROL_SHUTDOWN:
            apxLogWrite(APXLOG_MARK_INFO "Service SHUTDOWN signalled.");
        case SERVICE_CONTROL_STOP:
            apxLogWrite(APXLOG_MARK_INFO "Service SERVICE_CONTROL_STOP signalled.");
            _exe_shutdown = TRUE;
            if (SO_STOPTIMEOUT > 0) {
                reportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, SO_STOPTIMEOUT * 1000);
            }
            else {
                reportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 3 * 1000);
            }
            /* Stop the service asynchronously */
            stopThread = CreateThread(NULL, 0,
                                      serviceStop,
                                      (LPVOID)SERVICE_CONTROL_STOP,
                                      0, &threadId);
            CloseHandle(stopThread);
            return;
        case SERVICE_CONTROL_INTERROGATE:
            reportServiceStatusE(APXLOG_LEVEL_TRACE,
                                _service_status.dwCurrentState,
                                _service_status.dwWin32ExitCode,
                                _service_status.dwWaitHint,
                                0);
            return;
        default:
            break;
   }
}

/* Console control handler.
 */
BOOL WINAPI console_handler(DWORD dwCtrlType)
{
    switch (dwCtrlType) {
        case CTRL_BREAK_EVENT:
            apxLogWrite(APXLOG_MARK_INFO "Console CTRL+BREAK event signaled.");
            return FALSE;
        case CTRL_C_EVENT:
            apxLogWrite(APXLOG_MARK_INFO "Console CTRL+C event signaled.");
            serviceStop((LPVOID)SERVICE_CONTROL_STOP);
            return TRUE;
        case CTRL_CLOSE_EVENT:
            apxLogWrite(APXLOG_MARK_INFO "Console CTRL+CLOSE event signaled.");
            serviceStop((LPVOID)SERVICE_CONTROL_STOP);
            return TRUE;
        case CTRL_SHUTDOWN_EVENT:
            apxLogWrite(APXLOG_MARK_INFO "Console SHUTDOWN event signaled.");
            serviceStop((LPVOID)SERVICE_CONTROL_SHUTDOWN);
            return TRUE;
        case CTRL_LOGOFF_EVENT:
            apxLogWrite(APXLOG_MARK_INFO "Console LOGOFF event signaled.");
            if (!_service_mode) {
                serviceStop((LPVOID)SERVICE_CONTROL_STOP);
            }
            return TRUE;
        break;

   }
   return FALSE;
}

/* Main service execution loop. */
void WINAPI serviceMain(DWORD argc, LPTSTR *argv)
{
    DWORD rc = 0;
    _service_status.dwServiceType      = SERVICE_WIN32_OWN_PROCESS;
    _service_status.dwCurrentState     = SERVICE_START_PENDING;
    _service_status.dwControlsAccepted = SERVICE_CONTROL_INTERROGATE;
    _service_status.dwWin32ExitCode    = 0;
    _service_status.dwCheckPoint       = 0;
    _service_status.dwWaitHint         = 0;
    _service_status.dwServiceSpecificExitCode = 0;

    apxLogWrite(APXLOG_MARK_DEBUG "Inside serviceMain()...");

    if (IS_VALID_STRING(_service_name)) {
        WCHAR en[SIZ_HUGLEN];
        int i;
        PSECURITY_ATTRIBUTES sa = GetNullACL();
        lstrlcpyW(en, SIZ_DESLEN, L"Global\\");
        lstrlcatW(en, SIZ_DESLEN, _service_name);
        lstrlcatW(en, SIZ_DESLEN, PRSRV_SIGNAL);
        for (i = 7; i < lstrlenW(en); i++) {
            if (en[i] == L' ')
                en[i] = L'_';
            else
                en[i] = towupper(en[i]);
        }
        gSignalEvent = CreateEventW(sa, TRUE, FALSE, en);
        CleanNullACL((void *)sa);

        if (gSignalEvent) {
            DWORD tid;
            gSignalThread = CreateThread(NULL, 0, eventThread, NULL, 0, &tid);
        }
    }
    /* Check the StartMode */
    if (IS_VALID_STRING(SO_STARTMODE)) {
        if (!lstrcmpiW(SO_STARTMODE, PRSRV_JVM)) {
            _jni_startup = TRUE;
            if (IS_VALID_STRING(SO_STARTCLASS)) {
                _jni_rclass  = WideToANSI(SO_STARTCLASS);
                /* Exchange all dots with slashes */
                apxStrCharReplaceA(_jni_rclass, '.', '/');
            }
            else {
                /* Presume its main */
                _jni_rclass = WideToANSI(L"Main");
            }
            _jni_rparam = SO_STARTPARAMS;
        }
        else if (!lstrcmpiW(SO_STARTMODE, PRSRV_JAVA)) {
            LPWSTR jx = NULL, szJH = SO_JAVAHOME;
            if (!szJH)
                szJH = apxGetJavaSoftHome(gPool, FALSE);
            else if (!lstrcmpiW(szJH, PRSRV_JDK)) {
                /* Figure out the JDK JavaHome */
                szJH = apxGetJavaSoftHome(gPool, FALSE);
            }
            else if (!lstrcmpiW(szJH, PRSRV_JRE)) {
                /* Figure out the JRE JavaHome */
                szJH = apxGetJavaSoftHome(gPool, TRUE);
            }
            if (szJH) {
                jx = apxPoolAlloc(gPool, (lstrlenW(szJH) + 16) * sizeof(WCHAR));
                lstrcpyW(jx, szJH);
                lstrcatW(jx, PRSRV_JBIN);
                if (!SO_STARTPATH) {
                    /* Use JAVA_HOME/bin as start path */
                    LPWSTR szJP = apxPoolAlloc(gPool, (lstrlenW(szJH) + 8) * sizeof(WCHAR));
                    lstrcpyW(szJP, szJH);
                    lstrcatW(szJP, PRSRV_PBIN);
                    SO_STARTPATH = szJP;
                }
            }
            else {
                apxLogWrite(APXLOG_MARK_ERROR "Unable to find Java Runtime Environment.");
                goto cleanup;
            }
            _java_startup = TRUE;
            /* StartImage now contains the full path to the java.exe */
            SO_STARTIMAGE = jx;
        }
    }
    /* Check the StopMode */
    if (IS_VALID_STRING(SO_STOPMODE)) {
        if (!lstrcmpiW(SO_STOPMODE, PRSRV_JVM)) {
            _jni_shutdown = TRUE;
            if (IS_VALID_STRING(SO_STOPCLASS)) {
                _jni_sclass = WideToANSI(SO_STOPCLASS);
                apxStrCharReplaceA(_jni_sclass, '.', '/');
            }
            else {
                /* Defaults to Main */
                _jni_sclass = WideToANSI(L"Main");
            }
            _jni_sparam = SO_STOPPARAMS;
        }
        else if (!lstrcmpiW(SO_STOPMODE, PRSRV_JAVA)) {
            LPWSTR jx = NULL, szJH = SO_JAVAHOME;
            if (!szJH)
                szJH = apxGetJavaSoftHome(gPool, FALSE);
            else if (!lstrcmpiW(szJH, PRSRV_JDK)) {
                /* Figure out the JDK JavaHome */
                szJH = apxGetJavaSoftHome(gPool, FALSE);
            }
            else if (!lstrcmpiW(szJH, PRSRV_JRE)) {
                /* Figure out the JRE JavaHome */
                szJH = apxGetJavaSoftHome(gPool, TRUE);
            }
            if (szJH) {
                jx = apxPoolAlloc(gPool, (lstrlenW(szJH) + 16) * sizeof(WCHAR));
                lstrcpyW(jx, szJH);
                lstrcatW(jx, PRSRV_JBIN);
                if (!SO_STOPPATH) {
                    LPWSTR szJP = apxPoolAlloc(gPool, (lstrlenW(szJH) + 8) * sizeof(WCHAR));
                    lstrcpyW(szJP, szJH);
                    lstrcatW(szJP, PRSRV_PBIN);
                    /* Use JAVA_HOME/bin as stop path */
                    SO_STOPPATH = szJP;
                }
            }
            else {
                apxLogWrite(APXLOG_MARK_ERROR "Unable to find Java Runtime Environment.");
                goto cleanup;
            }
            _java_shutdown = TRUE;
            /* StopImage now contains the full path to the java.exe */
            SO_STOPIMAGE = jx;
        }
    }
    /* Find the classpath */
    if (_jni_shutdown || _jni_startup) {
        if (IS_VALID_STRING(SO_JVM)) {
            if (lstrcmpW(SO_JVM, PRSRV_AUTO))
                _jni_jvmpath = SO_JVM;
        }
        if (IS_VALID_STRING(SO_CLASSPATH))
            _jni_classpath = WideToANSI(SO_CLASSPATH);
        if (IS_VALID_STRING(SO_STARTMETHOD))
            _jni_rmethod   = WideToANSI(SO_STARTMETHOD);
        if (IS_VALID_STRING(SO_STOPMETHOD))
            _jni_smethod   = WideToANSI(SO_STOPMETHOD);
        _jni_jvmoptions    = MzWideToANSI(SO_JVMOPTIONS);
        _jni_jvmoptions9   = MzWideToANSI(SO_JVMOPTIONS9);
    }
    if (_service_mode) {
        /* Register Service Control handler */
        _service_status_handle = RegisterServiceCtrlHandlerW(_service_name,
                                                              service_ctrl_handler);
        if (IS_INVALID_HANDLE(_service_status_handle)) {
            apxLogWrite(APXLOG_MARK_ERROR "Failed to register Service Control for '%S'.",
                        _service_name);
            goto cleanup;
        }
        /* Allocate console so that events gets processed */
        if (!AttachConsole(ATTACH_PARENT_PROCESS) &&
             GetLastError() == ERROR_INVALID_HANDLE) {
            HWND hc;
            AllocConsole();
            if ((hc = GetConsoleWindow()) != NULL)
                ShowWindow(hc, SW_HIDE);
        }
    }
    reportServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
    if ((rc = serviceStart()) == 0) {
        /* Service is started */
        reportServiceStatus(SERVICE_RUNNING, NO_ERROR, 0);
        apxLogWrite(APXLOG_MARK_DEBUG "Waiting for worker to finish...");
        /* Set console handler to capture CTRL events */
        SetConsoleCtrlHandler((PHANDLER_ROUTINE)console_handler, TRUE);

        if (SO_STOPTIMEOUT) {
            /* we have a stop timeout */
            BOOL bLoopWarningIssued = FALSE;
            do {
                /* wait 2 seconds */
                DWORD rv = apxHandleWait(gWorker, 2000, FALSE);
                if (rv == WAIT_OBJECT_0 && !_exe_shutdown) {
                    if (!bLoopWarningIssued) {
                        apxLogWrite(APXLOG_MARK_WARN "Start method returned before stop method was called. This should not happen. Using loop with a fixed sleep of 2 seconds waiting for stop method to be called.");
                        bLoopWarningIssued = TRUE;
                    }
                    Sleep(2000);
                }
            } while (!_exe_shutdown);
            apxLogWrite(APXLOG_MARK_DEBUG "waiting %d sec... shutdown: %d", SO_STOPTIMEOUT, _exe_shutdown);
            apxHandleWait(gWorker, SO_STOPTIMEOUT*1000, FALSE);
        } else {
            apxHandleWait(gWorker, INFINITE, FALSE);
        }
        apxLogWrite(APXLOG_MARK_DEBUG "Worker finished.");
    }
    else {
        apxLogWrite(APXLOG_MARK_ERROR "ServiceStart returned %d.", rc);
        goto cleanup;
    }
    if (gShutdownEvent) {

        /* Ensure that shutdown thread exits before us */
        apxLogWrite(APXLOG_MARK_DEBUG "Waiting for ShutdownEvent.");
        reportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, ONE_MINUTE);
        WaitForSingleObject(gShutdownEvent, ONE_MINUTE);
        apxLogWrite(APXLOG_MARK_DEBUG "ShutdownEvent signaled.");
        CloseHandle(gShutdownEvent);
        gShutdownEvent = NULL;

        /* This will cause to wait for all threads to exit
         */
        apxLogWrite(APXLOG_MARK_DEBUG "Waiting 1 minute for all threads to exit.");
        reportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, ONE_MINUTE);
        apxDestroyJvm(ONE_MINUTE);
        /* if we are not using JAVA apxDestroyJvm does nothing, check the chid processes in case they hang */
        apxProcessTerminateChild( GetCurrentProcessId(), FALSE); /* FALSE kills! */
    }
    else {
        /* We came here without shutdown event
         * Probably because main() returned without ensuring all threads
         * have finished
         */
        apxLogWrite(APXLOG_MARK_DEBUG "Waiting for all threads to exit.");
        apxDestroyJvm(INFINITE);
        reportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
    }
    apxLogWrite(APXLOG_MARK_DEBUG "JVM destroyed.");
    reportServiceStatusStopped(apxGetVmExitCode());

    return;
cleanup:
    /* Cleanup */
    reportServiceStatusStopped(rc);
    gExitval = rc;
    return;
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
}


/* Run the service in the debug mode. */
BOOL docmdDebugService(LPAPXCMDLINE lpCmdline)
{
    _service_mode = FALSE;
    _service_name = lpCmdline->szApplication;
    apxLogWrite(APXLOG_MARK_INFO "Debugging '%S' service...", _service_name);
    serviceMain(0, NULL);
    apxLogWrite(APXLOG_MARK_INFO "Debug service finished with exit code %d.", gExitval);
    SAFE_CLOSE_HANDLE(gPidfileHandle);
    if (gPidfileName) {
        DeleteFileW(gPidfileName);
    }
    return gExitval == 0 ? TRUE : FALSE;
}

BOOL docmdRunService(LPAPXCMDLINE lpCmdline)
{
    BOOL rv;
    SERVICE_TABLE_ENTRYW dispatch_table[] = {
        { lpCmdline->szApplication, (LPSERVICE_MAIN_FUNCTIONW)serviceMain },
        { NULL, NULL }
    };
    _service_mode = TRUE;
    _service_name = lpCmdline->szApplication;
    apxLogWrite(APXLOG_MARK_INFO "Running Service '%S'...", _service_name);
    if (StartServiceCtrlDispatcherW(dispatch_table)) {
        apxLogWrite(APXLOG_MARK_INFO "Run service finished.");
        rv = TRUE;
    }
    else {
        apxLogWrite(APXLOG_MARK_ERROR "StartServiceCtrlDispatcher for '%S' failed.",
                    lpCmdline->szApplication);
        rv = FALSE;
    }
    SAFE_CLOSE_HANDLE(gPidfileHandle);
    if (gPidfileName) {
        DeleteFileW(gPidfileName);
    }
    return rv;
}

BOOL isRunningAsAdministrator(BOOL *bElevated) {
    BOOL rv = FALSE;
    HANDLE hToken;
    TOKEN_ELEVATION tokenInformation;
    DWORD dwSize;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        goto cleanup;
    }

    if (!GetTokenInformation(hToken, TokenElevation, &tokenInformation, sizeof(tokenInformation), &dwSize)) {
        goto cleanup;
    }

    *bElevated = tokenInformation.TokenIsElevated;
    rv = TRUE;

cleanup:
    if (hToken) {
        CloseHandle(hToken);
    }
    return rv;
}

BOOL restartCurrentProcessWithElevation(DWORD *dwExitCode) {
    BOOL rv = FALSE;
    TCHAR szPath[MAX_PATH];
    SHELLEXECUTEINFO shellExecuteInfo;

    SetLastError(0);
    GetModuleFileName(NULL, szPath, MAX_PATH);
    if (GetLastError()) {
        goto cleanup;
    }

    shellExecuteInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    shellExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    shellExecuteInfo.hwnd = NULL;
    shellExecuteInfo.lpVerb = L"runas";
    shellExecuteInfo.lpFile = szPath;
    shellExecuteInfo.lpParameters = PathGetArgs(GetCommandLine());
    shellExecuteInfo.lpDirectory = NULL;
    shellExecuteInfo.nShow = SW_SHOWNORMAL;
    shellExecuteInfo.hInstApp = NULL;

    if (!ShellExecuteEx(&shellExecuteInfo)) {
        goto cleanup;
    }
    WaitForSingleObject(shellExecuteInfo.hProcess, INFINITE);
    GetExitCodeProcess(shellExecuteInfo.hProcess, dwExitCode);

    rv = TRUE;

cleanup:
    return rv;
}


static const char *gSzProc[] = {
    "",
    "parse command line arguments",
    "load configuration",
    "run service as console application",
    "run service",
    "start service",
    "stop service",
    "update service parameters",
    "install service",
    "delete service",
    NULL
};

void __cdecl main(int argc, char **argv)
{
    UINT rv = 0;

    LPAPXCMDLINE lpCmdline;

    if (argc > 1) {
        DWORD ss = 0;
        if (strncmp(argv[1], "//PP", 4) == 0) {
            /* Handy sleep routine defaulting to 1 minute */
            if (argv[1][4] && argv[1][5] && argv[1][6]) {
                int us = atoi(argv[1] + 6);
                if (us > 0)
                    ss = (DWORD)us;
            }
            Sleep(ss * 1000);
            ExitProcess(0);
            return;
        }
        else if (strcmp(argv[1], "pause") == 0) {
            /* Handy sleep routine defaulting to 1 minute */
            if (argc > 2) {
                int us = atoi(argv[2]);
                if (us > 0)
                    ss = (DWORD)us;
            }
        }
        if (ss) {
            Sleep(ss * 1000);
            ExitProcess(0);
            return;
        }
    }
    apxHandleManagerInitialize();
    /* Create the main Pool */
    gPool = apxPoolCreate(NULL, 0);

    /* Parse the command line */
    if ((lpCmdline = apxCmdlineParse(gPool, _options, _commands, _altcmds)) == NULL) {
        apxLogWrite(APXLOG_MARK_ERROR "Invalid command line arguments.");
        rv = 1;
        goto cleanup;
    }
    apxCmdlineLoadEnvVars(lpCmdline);
    if (lpCmdline->dwCmdIndex < 6) {
        if (!loadConfiguration(lpCmdline) &&
            lpCmdline->dwCmdIndex < 5) {
            apxLogWrite(APXLOG_MARK_ERROR "Load configuration failed.");
            rv = 2;
            goto cleanup;
        }
    }

    /* Only configure logging to a file when running as a service */
    if (lpCmdline->dwCmdIndex == 2) {
        apxLogOpen(gPool, SO_LOGPATH, SO_LOGPREFIX, SO_LOGROTATE);
        apxLogLevelSetW(NULL, SO_LOGLEVEL);
        apxLogWrite(APXLOG_MARK_DEBUG "Apache Commons Daemon procrun log initialized.");
        if (SO_LOGROTATE) {
            apxLogWrite(APXLOG_MARK_DEBUG "Log will rotate each %d seconds.", SO_LOGROTATE);
        }
    } else {
        /* Not running as a service, just set the log level */
        apxLogLevelSetW(NULL, SO_LOGLEVEL);
    }
    apxLogWrite(APXLOG_MARK_INFO "Apache Commons Daemon procrun (%s %d-bit) started.",
                PRG_VERSION, PRG_BITS);

    AplZeroMemory(&gStdwrap, sizeof(APX_STDWRAP));
    gStartPath = lpCmdline->szExePath;
    gStdwrap.szLogPath = SO_LOGPATH;
    /* Only redirect when running as a service */
    if (lpCmdline->dwCmdIndex == 2) {
        gStdwrap.szStdOutFilename = SO_STDOUTPUT;
        gStdwrap.szStdErrFilename = SO_STDERROR;
    }
    redirectStdStreams(&gStdwrap, lpCmdline);
    if (lpCmdline->dwCmdIndex == 2) {
        SYSTEMTIME t;
        GetLocalTime(&t);
        fprintf(stdout, "\n%d-%02d-%02d %02d:%02d:%02d "
                        "Apache Commons Daemon procrun stdout initialized.\n",
                        t.wYear, t.wMonth, t.wDay,
                        t.wHour, t.wMinute, t.wSecond);
        fprintf(stderr, "\n%d-%02d-%02d %02d:%02d:%02d "
                        "Apache Commons Daemon procrun stderr initialized.\n",
                        t.wYear, t.wMonth, t.wDay,
                        t.wHour, t.wMinute, t.wSecond);
    }

    if (lpCmdline->dwCmdIndex > 2 && lpCmdline->dwCmdIndex < 8) {
        /* Command requires elevation */
        BOOL bElevated;
        if (!isRunningAsAdministrator(&bElevated)) {
            apxDisplayError(FALSE, NULL, 0, "Unable to determine if process has administrator privileges. Continuing as if it has.\n");
        } else {
            if (!bElevated) {
                DWORD dwExitCode;
                if (!restartCurrentProcessWithElevation(&dwExitCode)) {
                    apxDisplayError(FALSE, NULL, 0, "Failed to elevate current process.\n");
                    rv = lpCmdline->dwCmdIndex + 2;
                } else {
                    if (dwExitCode) {
                        apxDisplayError(FALSE, NULL, 0, "Running from a command prompt with administrative privileges may show further error details.\n");
                    }
                    rv = dwExitCode;
                }
                goto cleanup;
            }
        }
    }
    switch (lpCmdline->dwCmdIndex) {
        case 1: /* Run Service as console application */
            if (!docmdDebugService(lpCmdline))
                rv = 3;
        break;
        case 2: /* Run Service */
            if (!docmdRunService(lpCmdline))
                rv = 4;
        break;
        case 3: /* Start service - requires elevation */
            if (!docmdStartService(lpCmdline))
                rv = 5;
        break;
        case 4: /* Stop Service - requires elevation */
            if (!docmdStopService(lpCmdline))
                rv = 6;
        break;
        case 5: /* Update Service parameters  - requires elevation */
            if (!docmdUpdateService(lpCmdline))
                rv = 7;
        break;
        case 6: /* Install Service - requires elevation */
            if (!docmdInstallService(lpCmdline))
                rv = 8;
        break;
        case 7: /* Delete Service  - requires elevation */
            if (!docmdDeleteService(lpCmdline))
                rv = 9;
        break;
        case 8: /* Print Configuration and exit */
            printConfig(lpCmdline);
        break;
        case 9: /* Print Usage and exit */
            printUsage(lpCmdline, TRUE);
        break;
        case 10: /* Print version and exit */
            printVersion();
        break;
        default:
            /* Unknown command option */
            apxLogWrite(APXLOG_MARK_ERROR "Unknown command line option.");
            printUsage(lpCmdline, FALSE);
            rv = 99;
        break;
    }

cleanup:
    if (rv) {
        int ix = 0;
        if (rv > 0 && rv < 10)
            ix = rv;
        apxLogWrite(APXLOG_MARK_ERROR "Apache Commons Daemon procrun failed "
                                      "with exit value: %d (failed to %s).",
                                      rv, gSzProc[ix]);
        if (ix > 2 && !_service_mode) {
            /* Print something to the user console */
            apxDisplayError(FALSE, NULL, 0, "Failed to %s.\n", gSzProc[ix]);
        }
    }
    else
        apxLogWrite(APXLOG_MARK_INFO "Apache Commons Daemon procrun finished.");
    if (lpCmdline)
        apxCmdlineFree(lpCmdline);
    _service_status_handle = NULL;
    _service_mode = FALSE;
    _flushall();
    apxLogClose(NULL);
    apxHandleManagerDestroy();
    ExitProcess(rv);
}
