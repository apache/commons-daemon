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
 
/* ====================================================================
 * prunsrv -- Service Runner.
 * Contributed by Mladen Turk <mturk@apache.org>
 * 05 Aug 2003
 * ==================================================================== 
 */

/* Force the JNI vprintf functions */
#define _DEBUG_JNI  1
#include "apxwin.h"
#include "prunsrv.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <fcntl.h>
#include <io.h>         /* _open_osfhandle */

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

typedef struct APX_STDWRAP {
    LPCWSTR szLogPath;
    LPCWSTR szStdOutFilename;
    LPCWSTR szStdErrFilename;
    HANDLE  hStdOutFile;
    HANDLE  hStdErrFile;
    FILE    *fpStdOutFile;
    FILE    *fpStdErrFile;
    FILE    fpStdOutSave;
    FILE    fpStdErrSave;
} APX_STDWRAP;

/* Use static variables instead of #defines */
static LPCWSTR      PRSRV_AUTO   = L"auto";
static LPCWSTR      PRSRV_JAVA   = L"java";
static LPCWSTR      PRSRV_JVM    = L"jvm";
static LPCWSTR      PRSRV_MANUAL = L"manual";
static LPCWSTR      PRSRV_JBIN   = L"\\bin\\java.exe";

static LPWSTR       _service_name = NULL;
/* Allowed procrun commands */
static LPCWSTR _commands[] = {
    L"TS",      /* 1 Run Service as console application (default)*/
    L"RS",      /* 2 Run Service */
    L"SS",      /* 3 Stop Service */
    L"US",      /* 4 Update Service parameters */
    L"IS",      /* 5 Install Service */
    L"DS",      /* 6 Delete Service */
    NULL
};

/* Allowed procrun parameters */
static APXCMDLINEOPT _options[] = {

/* 0  */    { L"Description",       L"Description",     NULL,           APXCMDOPT_STR | APXCMDOPT_SRV, NULL, 0},
/* 1  */    { L"DisplayName",       L"DisplayName",     NULL,           APXCMDOPT_STR | APXCMDOPT_SRV, NULL, 0},
/* 2  */    { L"Install",           L"ImagePath",       NULL,           APXCMDOPT_STE | APXCMDOPT_SRV, NULL, 0},
/* 3  */    { L"Startup",           L"Startup",         NULL,           APXCMDOPT_STR | APXCMDOPT_SRV, NULL, 0},
/* 4  */    { L"DependsOn",         L"DependsOn",       NULL,           APXCMDOPT_MSZ | APXCMDOPT_REG, NULL, 0},
/* 5  */    { L"Environment",       L"Environment",     NULL,           APXCMDOPT_MSZ | APXCMDOPT_REG, NULL, 0},
/* 6  */    { L"User",              L"User",            NULL,           APXCMDOPT_STR | APXCMDOPT_REG, NULL, 0},
/* 7  */    { L"Password",          L"Password",        NULL,           APXCMDOPT_BIN | APXCMDOPT_REG, NULL, 0},

/* 8  */    { L"JavaHome",          L"JavaHome",        L"Java",        APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},
/* 9  */    { L"Jvm",               L"Jvm",             L"Java",        APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},
/* 10 */    { L"JvmOptions",        L"Options",         L"Java",        APXCMDOPT_MSZ | APXCMDOPT_REG, NULL, 0},
/* 11 */    { L"Classpath",         L"Classpath",       L"Java",        APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},
/* 12 */    { L"JvmMs",             L"JvmMs",           L"Java",        APXCMDOPT_INT | APXCMDOPT_REG, NULL, 0},
/* 13 */    { L"JvmMx",             L"JvmMx",           L"Java",        APXCMDOPT_INT | APXCMDOPT_REG, NULL, 0},
/* 14 */    { L"JvmSs",             L"JvmSs",           L"Java",        APXCMDOPT_INT | APXCMDOPT_REG, NULL, 0},

/* 15 */    { L"StopImage",         L"Image",           L"Stop",        APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},
/* 16 */    { L"StopPath",          L"WorkingPath",     L"Stop",        APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},
/* 17 */    { L"StopClass",         L"Class",           L"Stop",        APXCMDOPT_STR | APXCMDOPT_REG, NULL, 0},
/* 18 */    { L"StopParams",        L"Params",          L"Stop",        APXCMDOPT_MSZ | APXCMDOPT_REG, NULL, 0},
/* 19 */    { L"StopMethod",        L"Method",          L"Stop",        APXCMDOPT_STR | APXCMDOPT_REG, NULL, 0},
/* 20 */    { L"StopMode",          L"Mode",            L"Stop",        APXCMDOPT_STR | APXCMDOPT_REG, NULL, 0},
/* 21 */    { L"StopTimeout",       L"Timeout",         L"Stop",        APXCMDOPT_INT | APXCMDOPT_REG, NULL, 0},

/* 22 */    { L"StartImage",        L"Image",           L"Start",       APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},
/* 23 */    { L"StartPath",         L"WorkingPath",     L"Start",       APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},
/* 24 */    { L"StartClass",        L"Class",           L"Start",       APXCMDOPT_STR | APXCMDOPT_REG, NULL, 0},
/* 25 */    { L"StartParams",       L"Params",          L"Start",       APXCMDOPT_MSZ | APXCMDOPT_REG, NULL, 0},
/* 26 */    { L"StartMethod",       L"Method",          L"Start",       APXCMDOPT_STR | APXCMDOPT_REG, NULL, 0},
/* 27 */    { L"StartMode",         L"Mode",            L"Start",       APXCMDOPT_STR | APXCMDOPT_REG, NULL, 0},

/* 28 */    { L"LogPath",           L"Path",            L"Log",         APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},
/* 29 */    { L"LogPrefix",         L"Prefix",          L"Log",         APXCMDOPT_STR | APXCMDOPT_REG, NULL, 0},
/* 30 */    { L"LogLevel",          L"Level",           L"Log",         APXCMDOPT_STR | APXCMDOPT_REG, NULL, 0},
/* 31 */    { L"StdError",          L"StdError",        L"Log",         APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},
/* 32 */    { L"StdOutput",         L"StdOutput",       L"Log",         APXCMDOPT_STE | APXCMDOPT_REG, NULL, 0},
            /* NULL terminate the array */
            { NULL }
};

#define GET_OPT_V(x)  _options[x].szValue
#define GET_OPT_I(x)  _options[x].dwValue
#define GET_OPT_T(x)  _options[x].dwType

#define ST_DESCRIPTION      GET_OPT_T(0)
#define ST_DISPLAYNAME      GET_OPT_T(1)
#define ST_INSTALL          GET_OPT_T(2)
#define ST_STARTUP          GET_OPT_T(3)

#define SO_DESCRIPTION      GET_OPT_V(0)
#define SO_DISPLAYNAME      GET_OPT_V(1)
#define SO_INSTALL          GET_OPT_V(2)
#define SO_STARTUP          GET_OPT_V(3)
#define SO_DEPENDSON        GET_OPT_V(4)
#define SO_ENVIRONMENT      GET_OPT_V(5)

#define SO_USER             GET_OPT_V(6)
#define SO_PASSWORD         GET_OPT_V(7)

#define SO_JAVAHOME         GET_OPT_V(8)
#define SO_JVM              GET_OPT_V(9)
#define SO_JVMOPTIONS       GET_OPT_V(10)
#define SO_CLASSPATH        GET_OPT_V(11)
#define SO_JVMMS            GET_OPT_I(12)
#define SO_JVMMX            GET_OPT_I(13)
#define SO_JVMSS            GET_OPT_I(14)

#define SO_STOPIMAGE        GET_OPT_V(15)
#define SO_STOPPATH         GET_OPT_V(16)
#define SO_STOPCLASS        GET_OPT_V(17)
#define SO_STOPPARAMS       GET_OPT_V(18)
#define SO_STOPMETHOD       GET_OPT_V(19)
#define SO_STOPMODE         GET_OPT_V(20)
#define SO_STOPTIMEOUT      GET_OPT_I(21)

#define SO_STARTIMAGE       GET_OPT_V(22)
#define SO_STARTPATH        GET_OPT_V(23)
#define SO_STARTCLASS       GET_OPT_V(24)
#define SO_STARTPARAMS      GET_OPT_V(25)
#define SO_STARTMETHOD      GET_OPT_V(26)
#define SO_STARTMODE        GET_OPT_V(27)

#define SO_LOGPATH          GET_OPT_V(28)
#define SO_LOGPREFIX        GET_OPT_V(29)
#define SO_LOGLEVEL         GET_OPT_V(30)

#define SO_STDERROR         GET_OPT_V(31)
#define SO_STDOUTPUT        GET_OPT_V(32)

/* Main servic table entry
 * filled at run-time
 */
static SERVICE_TABLE_ENTRYW _service_table[] = {
        {NULL, NULL},
        {NULL, NULL}
};
 
static SERVICE_STATUS        _service_status; 
static SERVICE_STATUS_HANDLE _service_status_handle = NULL; 
/* Set if launched by SCM   */
static BOOL                  _service_mode = FALSE;
/* JVM used as worker       */
static BOOL                  _jni_startup  = FALSE;
/* JVM used for shutdown    */
static BOOL                  _jni_shutdown = FALSE;
/* Global variables and objects */
static APXHANDLE    gPool;
static APXHANDLE    gWorker;
static APX_STDWRAP  gStdwrap;           /* stdio/stderr redirection */

static LPWSTR _jni_jvmpath              = NULL;   /* Path to jvm dll */
static LPSTR  _jni_jvmoptions           = NULL;   /* Path to jvm options */

static LPSTR  _jni_classpath            = NULL;
static LPSTR  _jni_rparam               = NULL;    /* Startup  arguments */
static LPSTR  _jni_sparam               = NULL;    /* Shutdown arguments */
static LPSTR  _jni_rmethod              = NULL;    /* Startup  arguments */
static LPSTR  _jni_smethod              = NULL;    /* Shutdown arguments */
static CHAR   _jni_rclass[SIZ_RESLEN]   = {'\0'};  /* Startup  class */
static CHAR   _jni_sclass[SIZ_RESLEN]   = {'\0'};  /* Shutdown class */

static HANDLE gShutdownEvent = NULL;
/* redirect console stdout/stderr to files 
 * so that java messages can get logged
 * If stderrfile is not specified it will
 * go to stdoutfile.
 */



static BOOL redirectStdStreams(APX_STDWRAP *lpWrapper)
{
    BOOL aErr = FALSE;
    BOOL aOut = FALSE;

    /* Clear up the handles */    
    lpWrapper->fpStdErrFile = NULL;
    lpWrapper->fpStdOutFile = NULL;
    
    /* Save the original streams */
    lpWrapper->fpStdOutSave = *stdout;
    lpWrapper->fpStdErrSave = *stderr;

    /* redirect to file or console */
    if (lpWrapper->szStdOutFilename) {
        if (lstrcmpiW(lpWrapper->szStdOutFilename, PRSRV_AUTO) == 0) {
            aOut = TRUE;
            lpWrapper->szStdOutFilename = apxLogFile(gPool,
                                                     lpWrapper->szLogPath,
                                                     NULL,
                                                     L"stdout_");
        }
        /* Delete the file if not in append mode
         * XXX: See if we can use the params instead of that.
         */
        if (!aOut)
            DeleteFileW(lpWrapper->szStdOutFilename);
        lpWrapper->hStdOutFile = CreateFileW(lpWrapper->szStdOutFilename,
                                             GENERIC_WRITE | GENERIC_READ,
                                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                                             NULL,
                                             OPEN_ALWAYS,
                                             FILE_ATTRIBUTE_NORMAL,
                                             NULL);
        if (IS_INVALID_HANDLE(lpWrapper->hStdOutFile))
            return FALSE;
        /* Allways move to the end of file */
        SetFilePointer(lpWrapper->hStdOutFile, 0, NULL, FILE_END);
    }
    else {
        lpWrapper->hStdOutFile = CreateFileW(L"CONOUT$",
                                             GENERIC_READ | GENERIC_WRITE, 
                                             FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                             NULL,
                                             OPEN_EXISTING,
                                             0,
                                             NULL); 
        if (IS_INVALID_HANDLE(lpWrapper->hStdOutFile))
            return FALSE;
    }
    if (lpWrapper->szStdErrFilename) {
        if (lstrcmpiW(lpWrapper->szStdErrFilename, PRSRV_AUTO) == 0) {
            aErr = TRUE;
            lpWrapper->szStdErrFilename = apxLogFile(gPool,
                                                     lpWrapper->szLogPath,
                                                     NULL,
                                                     L"stderr_");
        }
        if (!aErr)
            DeleteFileW(lpWrapper->szStdErrFilename);
        lpWrapper->hStdErrFile = CreateFileW(lpWrapper->szStdErrFilename,
                                             GENERIC_WRITE | GENERIC_READ,
                                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                                             NULL,
                                             OPEN_ALWAYS,
                                             FILE_ATTRIBUTE_NORMAL,
                                             NULL);
        if (IS_INVALID_HANDLE(lpWrapper->hStdErrFile))
            return FALSE;
        SetFilePointer(lpWrapper->hStdErrFile, 0, NULL, FILE_END);
    }
    else if (lpWrapper->szStdOutFilename) {
        /* Use the same file handle for stderr as for stdout */
        lpWrapper->szStdErrFilename = lpWrapper->szStdOutFilename;
        lpWrapper->hStdErrFile = lpWrapper->hStdOutFile;
    }
    else {
        lpWrapper->hStdErrFile = lpWrapper->hStdOutFile;
    }
    /* Open the stream buffers 
     * This will redirect all printf to go to the redirected files.
     * It is used for JNI vprintf functionality.
     */
    lpWrapper->fpStdOutFile = _fdopen(_open_osfhandle(
                                      (intptr_t)lpWrapper->hStdOutFile,
                                      _O_TEXT), "w");
    lpWrapper->fpStdErrFile = _fdopen(_open_osfhandle(
                                      (intptr_t)lpWrapper->hStdErrFile,
                                      _O_TEXT), "w");
    if (lpWrapper->fpStdOutFile) {
        *stdout = *lpWrapper->fpStdOutFile;
        setvbuf(stdout, NULL, _IONBF, 0);
    }
    if (lpWrapper->fpStdErrFile) {
        *stderr = *lpWrapper->fpStdErrFile;
        setvbuf(stderr, NULL, _IONBF, 0);
    }
    return TRUE;
}

static void cleanupStdStreams(APX_STDWRAP *lpWrapper)
{
    /* Close the redirectied streams */
    if (lpWrapper->fpStdOutFile) {
        fclose(lpWrapper->fpStdOutFile);
        *stdout = lpWrapper->fpStdOutSave;
    }
    if (lpWrapper->fpStdErrFile) {
        fclose(lpWrapper->fpStdErrFile);
        *stderr = lpWrapper->fpStdErrSave;
    }
}

/* Debuging functions */
static void printUsage(LPAPXCMDLINE lpCmdline)
{
#ifdef _DEBUG
    int i = 0;
    fwprintf(stderr, L"Usage: %s //CMD//Servce [--options]\n",
             lpCmdline->szExecutable);
    fwprintf(stderr, L"  Commands:\n");
    fwprintf(stderr, L"  //IS//ServiceName  Install Service\n");
    fwprintf(stderr, L"  //US//ServiceName  Update Service parameters\n");
    fwprintf(stderr, L"  //DS//ServiceName  Delete Service\n");
    fwprintf(stderr, L"  //RS//ServiceName  Run Service\n");
    fwprintf(stderr, L"  //SS//ServiceName  Stop Service\n");
    fwprintf(stderr,
             L"  //TS//ServiceName  Run Service as console application\n");
    fwprintf(stderr, L"  Options:\n");
    while (_options[i].szName) {
        fwprintf(stderr, L"  --%s\n", _options[i].szName);
        ++i;
    }
#endif
}
/* Display configuration parameters */
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

static void setInprocEnvironment()
{
    LPWSTR p, e;

    if (!SO_ENVIRONMENT)
        return;    /* Nothing to do */
    
    for (p = SO_ENVIRONMENT; *p; p++) {
        e = apxExpandStrW(gPool, p);
        _wputenv(e);
        apxFree(e);
        while (*p)
            p++;
    }
}

/* Load the configuration from Registry
 * loads only nonspecified items
 */
static BOOL loadConfiguration(LPAPXCMDLINE lpCmdline)
{
    APXHANDLE hRegistry;
    int i = 0;

    SetLastError(ERROR_SUCCESS);
    hRegistry = apxCreateRegistryW(gPool, KEY_READ, PRG_REGROOT,
                                   lpCmdline->szApplication,
                                   APXREG_SOFTWARE | APXREG_SERVICE);
    if (IS_INVALID_HANDLE(hRegistry)) {
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

/* Save changed configuration to registry
 */
static BOOL saveConfiguration(LPAPXCMDLINE lpCmdline)
{
    APXHANDLE hRegistry;
    int i = 0;
    hRegistry = apxCreateRegistryW(gPool, KEY_WRITE, PRG_REGROOT,
                                   lpCmdline->szApplication,
                                   APXREG_SOFTWARE | APXREG_SERVICE);
    if (IS_INVALID_HANDLE(hRegistry))
        return FALSE;
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

/* Operations */
static BOOL docmdInstallService(LPAPXCMDLINE lpCmdline)
{
    APXHANDLE hService;
    BOOL  rv;
    DWORD dwStart = SERVICE_DEMAND_START;
    WCHAR szImage[SIZ_HUGLEN];
    
    apxLogWrite(APXLOG_MARK_DEBUG "Installing service...");
    hService = apxCreateService(gPool, SC_MANAGER_CREATE_SERVICE, FALSE);
    if (IS_INVALID_HANDLE(hService)) {
        apxLogWrite(APXLOG_MARK_ERROR "Unable to open the Service Manager");
        return FALSE;
    }
    /* Check the startup mode */
    if ((ST_STARTUP & APXCMDOPT_FOUND) &&
        lstrcmpiW(SO_STARTUP, PRSRV_AUTO) == 0)
        dwStart = SERVICE_AUTO_START;
    /* Check if --Install is provided */
    if (!SO_INSTALL) {
        lstrcpyW(szImage, lpCmdline->szExePath);
        lstrcatW(szImage, L"\\");
        lstrcatW(szImage, lpCmdline->szExecutable);
        lstrcatW(szImage, L".exe");
    }
    else
        lstrcpyW(szImage, SO_INSTALL);
    /* Replace not needed qoutes */
    apxStrQuoteInplaceW(szImage);
    /* Add run-service command line option */
    lstrcatW(szImage, L" //RS//");
    lstrcatW(szImage, lpCmdline->szApplication);
    SO_INSTALL = apxPoolStrdupW(gPool, szImage);
    /* Ensure that option gets saved in the registry */
    ST_INSTALL |= APXCMDOPT_FOUND;
#ifdef _DEBUG
    /* Display configured options */
    dumpCmdline();
#endif
    apxLogWrite(APXLOG_MARK_INFO "Service %S name %S", lpCmdline->szApplication, 
                SO_DISPLAYNAME);
    rv = apxServiceInstall(hService, 
                          lpCmdline->szApplication,
                          SO_DISPLAYNAME,    /* --DisplayName  */
                          SO_INSTALL,    
                          SO_DEPENDSON,      /* --DependendsOn */
                          SERVICE_WIN32_OWN_PROCESS,
                          dwStart);
    /* Set the --Description */
    if (rv && (ST_DESCRIPTION & APXCMDOPT_FOUND)) {
        apxLogWrite(APXLOG_MARK_DEBUG "Setting service description %S",
                    SO_DESCRIPTION);
        apxServiceSetNames(hService, NULL, NULL, SO_DESCRIPTION,
                           NULL, NULL);
    }
    apxCloseHandle(hService);
    if (rv) {
        saveConfiguration(lpCmdline);
        apxLogWrite(APXLOG_MARK_INFO "Service %S installed",
                    lpCmdline->szApplication);
    }
    else
        apxLogWrite(APXLOG_MARK_ERROR "Failed installing %S service",
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
        apxLogWrite(APXLOG_MARK_ERROR "Unable to open the Service Manager");
        return FALSE;
    }
    /* Delete service will stop the service if running */
    if (apxServiceOpen(hService, lpCmdline->szApplication, SERVICE_ALL_ACCESS)) {
        WCHAR szWndManagerClass[SIZ_RESLEN];
        HANDLE hWndManager = NULL;
        lstrcpyW(szWndManagerClass, lpCmdline->szApplication);
        lstrcatW(szWndManagerClass, L"_CLASS");
        /* Close the monitor application if running */
        if ((hWndManager = FindWindowW(szWndManagerClass, NULL)) != NULL) {
            SendMessage(hWndManager, WM_CLOSE, 0, 0);
        }
        rv = apxServiceDelete(hService);
    }
    if (rv) {
        /* Delete all service registry settings */
        apxDeleteRegistryW(PRG_REGROOT, lpCmdline->szApplication, TRUE);
        apxLogWrite(APXLOG_MARK_DEBUG "Service %S deleted",
                    lpCmdline->szApplication);
    }
    else {
        apxDisplayError(TRUE, NULL, 0, "Unable to delete %S service",
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

    apxLogWrite(APXLOG_MARK_INFO "Stopping service...");
    hService = apxCreateService(gPool, GENERIC_ALL, FALSE);
    if (IS_INVALID_HANDLE(hService)) {
        apxLogWrite(APXLOG_MARK_ERROR "Unable to open the Service Manager");
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
        if (rv)
            apxLogWrite(APXLOG_MARK_INFO "Service %S stopped",
                        lpCmdline->szApplication);
        else
            apxLogWrite(APXLOG_MARK_ERROR "Failed to stop %S service",
                        lpCmdline->szApplication);

    }
    else
        apxDisplayError(TRUE, NULL, 0, "Unable to open %S service",
                        lpCmdline->szApplication);
    apxCloseHandle(hService);
    apxLogWrite(APXLOG_MARK_INFO "Stop service finished.");
    return rv;
}

static BOOL docmdUpdateService(LPAPXCMDLINE lpCmdline)
{
    APXHANDLE hService;
    BOOL  rv = FALSE;

    apxLogWrite(APXLOG_MARK_INFO "Updating service...");

    hService = apxCreateService(gPool, SC_MANAGER_CREATE_SERVICE, FALSE);
    if (IS_INVALID_HANDLE(hService)) {
        apxLogWrite(APXLOG_MARK_ERROR "Unable to open the Service Manager");
        return FALSE;
    }
    SetLastError(0);
    /* Open the service */
    if (apxServiceOpen(hService, lpCmdline->szApplication, SERVICE_ALL_ACCESS)) {
        apxServiceSetNames(hService,
                           NULL,                /* Never update the ImagePath */
                           SO_DISPLAYNAME,
                           SO_DESCRIPTION,
                           NULL,
                           NULL);
        /* Update the --Startup mode */
        if (ST_STARTUP & APXCMDOPT_FOUND) {
            DWORD dwStart = SERVICE_NO_CHANGE;
            if (!lstrcmpiW(SO_STARTUP, PRSRV_AUTO))
                dwStart = SERVICE_AUTO_START;
            else if (!lstrcmpiW(SO_STARTUP, PRSRV_MANUAL))
                dwStart = SERVICE_DEMAND_START;
            apxServiceSetOptions(hService,
                                 SERVICE_NO_CHANGE,
                                 dwStart,
                                 SERVICE_NO_CHANGE);
    
        }
        apxLogWrite(APXLOG_MARK_INFO "Service %S updated",
                    lpCmdline->szApplication);

        saveConfiguration(lpCmdline);
    }
    else
        apxDisplayError(TRUE, NULL, 0, "Unable to open %S service",
                        lpCmdline->szApplication);
    apxCloseHandle(hService);
    apxLogWrite(APXLOG_MARK_INFO "Update service finished.");
    return rv;
}


/* Report the service status to the SCM
 */
int reportServiceStatus(DWORD dwCurrentState,
                        DWORD dwWin32ExitCode,
                        DWORD dwWaitHint)
{
   static DWORD dwCheckPoint = 1;
   BOOL fResult = TRUE;

   if (_service_mode && _service_status_handle) {      
       if (dwCurrentState == SERVICE_START_PENDING)
            _service_status.dwControlsAccepted = 0;
        else
            _service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

       _service_status.dwCurrentState  = dwCurrentState;
       _service_status.dwWin32ExitCode = dwWin32ExitCode;
       _service_status.dwWaitHint      = dwWaitHint;

       if ((dwCurrentState == SERVICE_RUNNING) ||
           (dwCurrentState == SERVICE_STOPPED))
           _service_status.dwCheckPoint = 0;
       else
           _service_status.dwCheckPoint = dwCheckPoint++;
       fResult = SetServiceStatus(_service_status_handle, &_service_status);
       if (!fResult) {
           /* TODO: Deal with error */
       }
   }
   return fResult;
}


BOOL child_callback(APXHANDLE hObject, UINT uMsg,
                    WPARAM wParam, LPARAM lParam)
{
    /* TODO: Make stdout and stderr buffers
     * to prevent streams intermixing when there
     * is no separate file for each stream
     */
    if (uMsg == WM_CHAR) {
        int ch = LOWORD(wParam);
        if (lParam)
            fputc(ch, stderr);
        else
            fputc(ch, stdout);
    }
    return TRUE;
}

/* Executed when the service receives stop event */
static DWORD serviceStop()
{
    APXHANDLE hWorker = NULL;
    DWORD  rv = 0;
    BOOL   wait_to_die = FALSE;
    DWORD  timeout     = SO_STOPTIMEOUT * 1000;

    apxLogWrite(APXLOG_MARK_INFO "Stopping service...");

    if (IS_INVALID_HANDLE(gWorker)) {
        apxLogWrite(APXLOG_MARK_INFO "Worker is not defined");
        return TRUE;    /* Nothing to do */
    }
    if (_jni_shutdown) {
        if (!SO_STARTPATH && SO_STOPPATH) {
            /* If the Working path is specified change the current directory 
             * but only if the start path wasn't specified already.
             */
            SetCurrentDirectoryW(SO_STARTPATH);
        }
        hWorker = apxCreateJava(gPool, _jni_jvmpath);
        if (IS_INVALID_HANDLE(hWorker)) {
            apxLogWrite(APXLOG_MARK_ERROR "Failed creating java %S", _jni_jvmpath);
            return 1;
        }
        if (!apxJavaInitialize(hWorker, _jni_classpath, _jni_jvmoptions,
                               SO_JVMMS, SO_JVMMX, SO_JVMSS, TRUE)) {
            rv = 2;
            apxLogWrite(APXLOG_MARK_ERROR "Failed initializing java %s", _jni_classpath);
            goto cleanup;
        }
        if (!apxJavaLoadMainClass(hWorker, _jni_sclass, _jni_smethod, _jni_sparam)) {
            rv = 2;
            apxLogWrite(APXLOG_MARK_ERROR "Failed loading main %s class %s",
                        _jni_rclass, _jni_classpath);
            goto cleanup;
        }
        /* Create sutdown event */
        gShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!apxJavaStart(hWorker)) {
            apxLogWrite(APXLOG_MARK_ERROR "Failed starting java");
            rv = 3;
        }
        else {
            apxLogWrite(APXLOG_MARK_DEBUG "Waiting for java jni stop worker to finish...");
            apxJavaWait(hWorker, INFINITE, FALSE);        
            apxLogWrite(APXLOG_MARK_DEBUG "Java jni stop worker finished.");
        }
        wait_to_die = TRUE;
    } 
    else if (SO_STOPMODE) { /* Only in case we have a stop mode */
        DWORD nArgs;
        LPWSTR *pArgs;
        /* Redirect process */
        hWorker = apxCreateProcessW(gPool, 
                                    0,
                                    child_callback,
                                    SO_USER,
                                    SO_PASSWORD,
                                    FALSE);
        if (IS_INVALID_HANDLE(hWorker)) {
            apxLogWrite(APXLOG_MARK_ERROR "Failed creating process");
            return 1;
        }
        if (!apxProcessSetExecutableW(hWorker, SO_STOPIMAGE)) {
            apxLogWrite(APXLOG_MARK_ERROR "Failed setting process executable %S",
                        SO_STARTIMAGE);
            rv = 2;
            goto cleanup;
        }
        /* Assemble the command line */
        nArgs = apxMultiSzToArrayW(gPool, SO_STOPPARAMS, &pArgs);
        /* Pass the argv to child process */
        if (!apxProcessSetCommandArgsW(hWorker, SO_STOPIMAGE,
                                       nArgs, pArgs)) {
            rv = 3;
            apxLogWrite(APXLOG_MARK_ERROR "Failed setting process arguments (argc=%d)",
                        nArgs);
            goto cleanup;
        }
        /* Set the working path */
        if (!apxProcessSetWorkingPathW(hWorker, SO_STOPPATH)) {
            rv = 4;
            apxLogWrite(APXLOG_MARK_ERROR "Failed setting process working path to %S",
                        SO_STOPPATH);
            goto cleanup;
        }
        /* Finally execute the child process 
         */
        if (!apxProcessExecute(hWorker)) {
            rv = 5;
            apxLogWrite(APXLOG_MARK_ERROR "Failed executing process");
            goto cleanup;
        } else {
            apxLogWrite(APXLOG_MARK_DEBUG "Waiting stop worker to finish...");
            apxHandleWait(hWorker, INFINITE, FALSE);        
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
    SetEvent(gShutdownEvent);
    if (timeout > 0x7FFFFFFF)
        timeout = INFINITE;     /* If the timeout was '-1' wait forewer */ 
    if (wait_to_die && !timeout)
        timeout = 300 * 1000;   /* Use the 5 minute default shutdown */

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
            apxLogWrite(APXLOG_MARK_DEBUG "Worker finished gracefully in %d ms.", nms);
        }
        else
            apxLogWrite(APXLOG_MARK_DEBUG "Worker was killed in %d ms.", nms);
    }
    else {
        apxLogWrite(APXLOG_MARK_DEBUG "Sending WM_CLOSE to worker");
        apxHandleSendMessage(gWorker, WM_CLOSE, 0, 0);
    }

    apxLogWrite(APXLOG_MARK_INFO "Service stopped.");
    return rv;
}

/* Executed when the service receives start event */
static DWORD serviceStart()
{
    DWORD  rv = 0;
    DWORD  nArgs;
    LPWSTR *pArgs;
    FILETIME fts;

    apxLogWrite(APXLOG_MARK_INFO "Starting service...");

    if (!IS_INVALID_HANDLE(gWorker)) {
        apxLogWrite(APXLOG_MARK_INFO "Worker is not defined");
        return TRUE;    /* Nothing to do */
    }
    GetSystemTimeAsFileTime(&fts);
    if (_jni_startup) {
        if (SO_STARTPATH) {
            /* If the Working path is specified change the current directory */
            SetCurrentDirectoryW(SO_STARTPATH);
        }
        /* Set the environment using putenv, so JVM can use it */
        setInprocEnvironment();
        /* Create the JVM glbal worker */
        gWorker = apxCreateJava(gPool, _jni_jvmpath);
        if (IS_INVALID_HANDLE(gWorker)) {
            apxLogWrite(APXLOG_MARK_ERROR "Failed creating java %S", _jni_jvmpath);
            return 1;
        }
        if (!apxJavaInitialize(gWorker, _jni_classpath, _jni_jvmoptions,
                               SO_JVMMS, SO_JVMMX, SO_JVMSS, _service_mode)) {
            rv = 2;
            apxLogWrite(APXLOG_MARK_ERROR "Failed initializing java %s", _jni_classpath);
            goto cleanup;
        }
        if (!apxJavaLoadMainClass(gWorker, _jni_rclass, _jni_rmethod, _jni_rparam)) {
            rv = 3;
            apxLogWrite(APXLOG_MARK_ERROR "Failed loading main %s class %s", _jni_rclass, _jni_classpath);
            goto cleanup;
        }
        apxJavaSetOut(gWorker, TRUE,  gStdwrap.szStdErrFilename);
        apxJavaSetOut(gWorker, FALSE, gStdwrap.szStdOutFilename);
        if (!apxJavaStart(gWorker)) {
            rv = 4;
            apxLogWrite(APXLOG_MARK_ERROR "Failed starting Java");
            goto cleanup;
        }
        apxLogWrite(APXLOG_MARK_DEBUG "Java started %s", _jni_rclass);
    }
    else {
        /* Redirect process */
        gWorker = apxCreateProcessW(gPool, 
                                    0,
                                    child_callback,
                                    SO_USER,
                                    SO_PASSWORD,
                                    FALSE);
        if (IS_INVALID_HANDLE(gWorker)) {
            apxLogWrite(APXLOG_MARK_ERROR "Failed creating process");
            return 1;
        }
        if (!apxProcessSetExecutableW(gWorker, SO_STARTIMAGE)) {
            apxLogWrite(APXLOG_MARK_ERROR "Failed setting process executable %S",
                        SO_STARTIMAGE);
            rv = 2;
            goto cleanup;
        }
        /* Assemble the command line */
        nArgs = apxMultiSzToArrayW(gPool, SO_STARTPARAMS, &pArgs);
        /* Pass the argv to child process */
        if (!apxProcessSetCommandArgsW(gWorker, SO_STARTIMAGE,
                                       nArgs, pArgs)) {
            rv = 3;
            apxLogWrite(APXLOG_MARK_ERROR "Failed setting process arguments (argc=%d)",
                        nArgs);
            goto cleanup;
        }
        /* Set the working path */
        if (!apxProcessSetWorkingPathW(gWorker, SO_STARTPATH)) {
            rv = 4;
            apxLogWrite(APXLOG_MARK_ERROR "Failed setting process working path to %S",
                        SO_STARTPATH);
            goto cleanup;
        }
        /* Finally execute the child process 
         */
        if (!apxProcessExecute(gWorker)) {
            rv = 5;
            apxLogWrite(APXLOG_MARK_ERROR "Failed executing process");
            goto cleanup;
        }
    }
    if (rv == 0) {
        FILETIME fte;
        ULARGE_INTEGER s, e;
        DWORD    nms;
        GetSystemTimeAsFileTime(&fte);
        s.LowPart  = fts.dwLowDateTime;
        s.HighPart = fts.dwHighDateTime;
        e.LowPart  = fte.dwLowDateTime;
        e.HighPart = fte.dwHighDateTime;
        nms = (DWORD)((e.QuadPart - s.QuadPart) / 10000);
        apxLogWrite(APXLOG_MARK_INFO "Service started in %d ms.", nms);
    }
    return rv;
cleanup:
    if (!IS_INVALID_HANDLE(gWorker))
        apxCloseHandle(gWorker);    /* Close the worker handle */
    gWorker = NULL;
    return rv;    
}

/* Service controll handler
 */
void WINAPI service_ctrl_handler(DWORD dwCtrlCode)
{
    switch (dwCtrlCode) {
        case SERVICE_CONTROL_STOP:
            apxLogWrite(APXLOG_MARK_INFO "Service STOP signaled");
            reportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
            /* Call the stop handler that will actualy stop the service */
            serviceStop();
            return;
        case SERVICE_CONTROL_INTERROGATE:
        break;
        default:
        break;
   }
   reportServiceStatus(_service_status.dwCurrentState, NO_ERROR, 0);
}

/* Console control handler
 * 
 */
BOOL WINAPI console_handler(DWORD dwCtrlType)
{
    switch (dwCtrlType) {
        case CTRL_BREAK_EVENT:
            apxLogWrite(APXLOG_MARK_INFO "Console CTRL+BREAK event signaled");
            if (_service_mode) {
                serviceStop();
                return TRUE;
            }
            else
                return FALSE;
        case CTRL_C_EVENT:
            apxLogWrite(APXLOG_MARK_INFO "Console CTRL+C event signaled");
            serviceStop();
            return TRUE;
        case CTRL_CLOSE_EVENT:
            apxLogWrite(APXLOG_MARK_INFO "Console CTRL+CLOSE event signaled");
            serviceStop();
            return TRUE;
        case CTRL_SHUTDOWN_EVENT:
            apxLogWrite(APXLOG_MARK_INFO "Console SHUTDOWN event signaled");
            serviceStop();
            return TRUE;
        break;

   }
   return FALSE;
}

/* Main service execution loop */
void WINAPI serviceMain(DWORD argc, LPTSTR *argv)
{
    DWORD rc;
    _service_status.dwServiceType      = SERVICE_WIN32_OWN_PROCESS;
    _service_status.dwCurrentState     = SERVICE_START_PENDING; 
    _service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                         SERVICE_ACCEPT_PAUSE_CONTINUE; 
    _service_status.dwWin32ExitCode    = 0; 
    _service_status.dwCheckPoint       = 0; 
    _service_status.dwWaitHint         = 0; 
    _service_status.dwServiceSpecificExitCode = 0; 

    apxLogWrite(APXLOG_MARK_DEBUG "Inside ServiceMain...");
    
    /* Check the StartMode */
    if (SO_STARTMODE) {
        if (!lstrcmpiW(SO_STARTMODE, PRSRV_JVM)) {
            _jni_startup = TRUE;            
            WideToAscii(SO_STARTCLASS, _jni_rclass);
            /* Exchange all dots with slashes */
            apxStrCharReplaceA(_jni_rclass, '.', '/');
            _jni_rparam = MzWideToAscii(SO_STARTPARAMS, (LPSTR)SO_STARTPARAMS);
        }
        else if (!lstrcmpiW(SO_STARTMODE, PRSRV_JAVA)) {
            LPWSTR jx = NULL, szJH = apxGetJavaSoftHome(gPool, FALSE);
            if (szJH) {
                jx = apxPoolAlloc(gPool, (lstrlenW(szJH) + 16) * sizeof(WCHAR));
                lstrcpyW(jx, szJH);
                lstrcatW(jx, PRSRV_JBIN);
                SO_STARTPATH = szJH;
            }
            /* StartImage now contains the full path to the java.exe */
            SO_STARTIMAGE = jx;
        }
    }
    /* Check the StopMode */
    if (SO_STOPMODE) {
        if (!lstrcmpiW(SO_STOPMODE, PRSRV_JVM)) {
            _jni_shutdown = TRUE;            
            WideToAscii(SO_STOPCLASS, _jni_sclass);
            apxStrCharReplaceA(_jni_sclass, '.', '/');
            _jni_sparam = MzWideToAscii(SO_STOPPARAMS, (LPSTR)SO_STOPPARAMS);
        }
        else if (!lstrcmpiW(SO_STOPMODE, PRSRV_JAVA)) {
            LPWSTR jx = NULL, szJH = apxGetJavaSoftHome(gPool, FALSE);
            if (szJH) {
                jx = apxPoolAlloc(gPool, (lstrlenW(szJH) + 16) * sizeof(WCHAR));
                lstrcpyW(jx, szJH);
                lstrcatW(jx, PRSRV_JBIN);
                SO_STOPPATH = szJH;
            }
            /* StopImage now contains the full path to the java.exe */
            SO_STOPIMAGE = jx;
        }
    }
    /* Find the classpath */
    if (_jni_shutdown || _jni_startup) {
        if (SO_JVM) {
            if (lstrcmpW(SO_JVM, PRSRV_AUTO))
                _jni_jvmpath = SO_JVM;
        }
        if (SO_CLASSPATH)
            _jni_classpath = WideToAscii(SO_CLASSPATH, (LPSTR)SO_CLASSPATH);
        if (SO_STARTMETHOD)
            _jni_rmethod = WideToAscii(SO_STARTMETHOD, (LPSTR)SO_STARTMETHOD);
        if (SO_STOPMETHOD)
            _jni_smethod = WideToAscii(SO_STOPMETHOD, (LPSTR)SO_STOPMETHOD);
        if (SO_JVMOPTIONS) {
            _jni_jvmoptions = MzWideToAscii(SO_JVMOPTIONS, (LPSTR)SO_JVMOPTIONS);
        }
    }
    if (_service_mode) {
        /* Register Service Control handler */
        _service_status_handle = RegisterServiceCtrlHandlerW(_service_name,
                                                              service_ctrl_handler); 
        if (IS_INVALID_HANDLE(_service_status_handle)) {
            apxLogWrite(APXLOG_MARK_ERROR "Failed to register Service Control for %S",
                        _service_name);
            goto cleanup;
        }
    }
    reportServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
    if ((rc = serviceStart()) == 0) {
        /* Service is started */
        DWORD rv;
        reportServiceStatus(SERVICE_RUNNING, NO_ERROR, 0);
        apxLogWrite(APXLOG_MARK_DEBUG "Waitning worker to finish...");
        /* Set console handler to capture CTRL events */
        SetConsoleCtrlHandler((PHANDLER_ROUTINE)console_handler, TRUE);

        rv = apxHandleWait(gWorker, INFINITE, FALSE);
        apxLogWrite(APXLOG_MARK_DEBUG "Worker finished.");
        reportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
        fflush(stdout);
    }
    else {
        apxLogWrite(APXLOG_MARK_ERROR "ServiceStart returned %d", rc);
        goto cleanup;
    }
    if (gShutdownEvent) {
        /* Ensure that shutdown thread exits before us */
        apxLogWrite(APXLOG_MARK_DEBUG "Waiting for ShutdownEvent");
        WaitForSingleObject(gShutdownEvent, 60 * 1000);
        apxLogWrite(APXLOG_MARK_DEBUG "ShutdownEvent signaled");
        CloseHandle(gShutdownEvent);
    }
    reportServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);

    return;
cleanup:
    /* Cleanup */
    reportServiceStatus(SERVICE_STOPPED, ERROR_SERVICE_SPECIFIC_ERROR, 0);
    return;
}


/* Run the service in the debug mode */
BOOL docmdDebugService(LPAPXCMDLINE lpCmdline)
{
    BOOL rv = FALSE;

    _service_mode = FALSE;
    apxLogWrite(APXLOG_MARK_INFO "Debuging Service...");
    serviceMain(0, NULL);
    apxLogWrite(APXLOG_MARK_INFO "Debug service finished.");
    
    return rv;
}

BOOL docmdRunService(LPAPXCMDLINE lpCmdline)
{
    BOOL rv = FALSE;
    _service_mode = TRUE;

    apxLogWrite(APXLOG_MARK_INFO "Running Service...");
    _service_name = lpCmdline->szApplication;
    _service_table[0].lpServiceName = lpCmdline->szApplication;
    _service_table[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTIONW)serviceMain;
    rv = (StartServiceCtrlDispatcherW(_service_table) == FALSE); 
    apxLogWrite(APXLOG_MARK_INFO "Run service finished.");
    return rv;
}

void __cdecl main(int argc, char **argv)
{
    UINT rv = 0;

    LPAPXCMDLINE lpCmdline;

    apxHandleManagerInitialize();
    /* Create the main Pool */
    gPool = apxPoolCreate(NULL, 0);

    /* Parse the command line */
    if ((lpCmdline = apxCmdlineParse(gPool, _options, _commands)) == NULL) {
        apxLogWrite(APXLOG_MARK_ERROR "Invalid command line arguments");
        rv = 1;
        goto cleanup;
    }
    apxCmdlineLoadEnvVars(lpCmdline);
    if (lpCmdline->dwCmdIndex < 5 &&
        !loadConfiguration(lpCmdline)) {
        apxLogWrite(APXLOG_MARK_ERROR "Load configuration failed");
        rv = 2;
        goto cleanup;
    }

    apxLogOpen(gPool, SO_LOGPATH, SO_LOGPREFIX);
    apxLogLevelSetW(NULL, SO_LOGLEVEL);
    apxLogWrite(APXLOG_MARK_DEBUG "Procrun log initialized");

    AplZeroMemory(&gStdwrap, sizeof(APX_STDWRAP));
    
    gStdwrap.szLogPath = SO_LOGPATH;
    /* In debug mode allways use console */
    if (lpCmdline->dwCmdIndex != 1) {
        gStdwrap.szStdOutFilename = SO_STDOUTPUT;
        gStdwrap.szStdErrFilename = SO_STDERROR;
    }
    redirectStdStreams(&gStdwrap);
    switch (lpCmdline->dwCmdIndex) {
        case 1: /* Run Service as console application */
            if (!docmdDebugService(lpCmdline))
                rv = 3;
        break;
        case 2: /* Run Service */
            if (!docmdRunService(lpCmdline))
                rv = 4;
        break;
        case 3: /* Stop Service */
            if (!docmdStopService(lpCmdline))
                rv = 5;
        break;
        case 4: /* Update Service parameters */
            if (!docmdUpdateService(lpCmdline))
                rv = 6;
        break;
        case 5: /* Install Service */
            if (!docmdInstallService(lpCmdline))
                rv = 7;
        break;
        case 6: /* Delete Service */
            if (!docmdDeleteService(lpCmdline))
                rv = 8;
        break;
        default:
            /* Unknow command option */
            apxLogWrite(APXLOG_MARK_ERROR "Unknown command line option");
            printUsage(lpCmdline);
            rv = 99;
        break;
    }

cleanup:
    apxLogWrite(APXLOG_MARK_INFO "Procrun finished.");
    if (lpCmdline)
        apxCmdlineFree(lpCmdline);
    if (_service_status_handle)
        CloseHandle(_service_status_handle);
    _service_status_handle = NULL;
    apxLogClose(NULL);
    apxHandleManagerDestroy();
    cleanupStdStreams(&gStdwrap);
    ExitProcess(rv);
}
