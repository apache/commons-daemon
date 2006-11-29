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

/*
 * jsvc.exe install program, create the service JavaService
 */

/* includes */
#include <windows.h>
#include <string.h>
#include <stdio.h>

#include "moni_inst.h"

/* Definitions for booleans */
typedef enum {
    false,
    true
} bool;
#include "arguments.h"

VOID Usage()
{
    printf( "\r\n - Java service installer\r\n\r\n");
    printf( " - Usage :\r\n");

    printf( "       To install Java service : InstSvc -install ");
    printf( " [-home JAVA_HOME] [-Dproperty=value]\r\n");
    printf( "                                 [-cp CLASSPATH] startclass\r\n");
    printf( " Like:\r\n");

    printf( " InstSvc -install -home c:\\jdk1.3.1_02");
    printf( " -Dcatalina.home=/home1/jakarta/jakarta-tomcat-4.1/build");
    printf( " -Djava.io.tmpdir=/var/tmp ");
    printf( " -cp \"c:\\jdk1.3.1_02\\lib\\tools.jar;");
    printf( "c:\\home1\\jakarta\\jakarta-tomcat-4.1\\build\\bin\\commons-daemon.jar;");
    printf( "c:\\home1\\jakarta\\jakarta-tomcat-4.1\\build\\bin\\bootstrap.jar\"");
    printf( " org.apache.catalina.startup.BootstrapService\r\n");

    printf( "       To remove Java service  : InstSvc -remove\r\n\r\n");
    printf( "   Use regedit if you want to change something\r\n\r\n");
    printf( "   Note that the service keys are stored under:\r\n");
    printf( "   HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\");
    printf( "%s",SZSERVICENAME);
    printf( "\r\n");
    printf( "   The environment keys in:\r\n");
    printf( "   ");
    printf( "%s",SZKEY_ONSERVE);
    printf( "\r\n");
    return;
}

/* from src/os/win32/service.c (httpd-1.3!) */

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


/* remove the service (first stop it!) NT version */

BOOL RemoveSvcNT (VOID)
{
    BOOL            removed;
    SC_HANDLE       hManager;
    SC_HANDLE       hService;
    SERVICE_STATUS  svcStatus;
    DWORD           dwCount;

    removed = FALSE;
    /* open service control manager with full access right */
    hManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (NULL != hManager) {
        /* open existing service */
        hService = OpenService(hManager, SZSERVICENAME, SERVICE_ALL_ACCESS);
        if (NULL != hService) {
            /* get the status of the service */
            if (QueryServiceStatus(hService, &svcStatus)) {
                /* and see if the service is stopped */
                if (SERVICE_STOPPED != svcStatus.dwCurrentState) {
                    /* if not stop the service */
                    ControlService(hService, SERVICE_CONTROL_STOP, &svcStatus);
                }
                dwCount = 0;
                do {
                    if (SERVICE_STOPPED == svcStatus.dwCurrentState) {
                        /* delete the service */
                        if (DeleteService(hService)) {
                            removed = TRUE;
                            break;
                        }
                    }
                    /* wait 10 seconds for the service to stop */
                    Sleep(10000);
                    if (!QueryServiceStatus(hService, &svcStatus)) {
                        /* something went wrong */
                        break;
                    }
                    dwCount++;
                } while (10 > dwCount);
            }
            /* close service handle */
            CloseServiceHandle(hService);
        }
        /* close service control manager */
        CloseServiceHandle(hManager);
    }
    return removed;
} /* RemoveSvc */

/* remove service (non NT) stopping it looks ugly!!! */
BOOL RemoveSvc (VOID)
{
    HKEY hkey;
    DWORD rv;

    rv = RegOpenKey(HKEY_LOCAL_MACHINE,
        "Software\\Microsoft\\Windows\\CurrentVersion\\RunServices",
        &hkey);
    if (rv != ERROR_SUCCESS) {
        printf( "Could not open the RunServices registry key.\r\n");
        return FALSE;
    }
    rv = RegDeleteValue(hkey, SZSERVICENAME);
    RegCloseKey(hkey);
    if (rv != ERROR_SUCCESS)
        printf( "Could not delete the RunServices entry.\r\n");

    rv = RegOpenKey(HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Services", &hkey);
    if (rv != ERROR_SUCCESS) {
        printf( "Could not open the Services registry key.\r\n");
        return FALSE;
    }
    rv = RegDeleteKey(hkey, SZSERVICENAME);
    RegCloseKey(hkey);
    if (rv != ERROR_SUCCESS) {
        printf( "Could not delete the Services registry key.\r\n");
        return FALSE;
    }
    return TRUE;
}


/* Install service (NT version) */

BOOL InstallSvcNT (CHAR *svcExePath)
{
    BOOL        installed;
    SC_HANDLE   hManager;
    SC_HANDLE   hService;

    installed = FALSE;
    /* open the service control manager with full access right */
    hManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (NULL != hManager) {
        /* create the service */
        hService = CreateService(hManager,
            SZSERVICENAME,             /* name of the service */
            SZSERVICEDISPLAYNAME,      /* description */
            SERVICE_ALL_ACCESS,
            SERVICE_WIN32_OWN_PROCESS,  /* type of service */
            SERVICE_DEMAND_START,       /* AUTO_START,  startmode */
            SERVICE_ERROR_NORMAL,       /* error treatment */
            svcExePath,                 /* path_name */
            NULL,                       /* no load order enty */
            NULL,                       /* no tag identifier. */
            NULL,                       /* dependencies. */
            NULL,                       /* LocalSystem account */
            NULL);                      /* dummy user password */
        if (NULL != hService) {
            /* close service handle */
            CloseServiceHandle(hService);
            installed = TRUE;
        }
    } else {
        printf( "OpenSCManager failed\r\n");
    }
    return installed;
}

/* Install service */

BOOL InstallSvc (CHAR *svcExePath)
{
    HKEY        hkey;
    DWORD rv;
    char szPath[MAX_PATH];

    printf( "InstallSvc for non-NT\r\n");

    rv = RegCreateKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows"
              "\\CurrentVersion\\RunServices", &hkey);
    if (rv != ERROR_SUCCESS) {
        printf( "Could not open the RunServices registry key\r\n");
        return FALSE;
    }
        rv = RegSetValueEx(hkey, SZSERVICENAME, 0, REG_SZ,
               (unsigned char *) svcExePath,
               strlen(svcExePath) + 1);
    RegCloseKey(hkey);
    if (rv != ERROR_SUCCESS) {
        printf( "Could not add %s:%s ",SZSERVICENAME, svcExePath);
        printf( "to RunServices Registry Key\r\n");
        return FALSE;
    }

    strcpy(szPath,
         "SYSTEM\\CurrentControlSet\\Services\\");
    strcat(szPath,SZSERVICENAME);
    rv = RegCreateKey(HKEY_LOCAL_MACHINE, szPath, &hkey);
    if (rv != ERROR_SUCCESS) {
        printf( "Could not create/open the %s registry key\r\n",
            szPath);
        return FALSE;
    }
    rv = RegSetValueEx(hkey, "ImagePath", 0, REG_SZ,
               (unsigned char *) svcExePath,
               strlen(svcExePath) + 1);
    if (rv != ERROR_SUCCESS) {
        RegCloseKey(hkey);
        printf( "Could not add ImagePath to our Registry Key\r\n");
        return FALSE;
    }
    rv = RegSetValueEx(hkey, "DisplayName", 0, REG_SZ,
               (unsigned char *) SZSERVICEDISPLAYNAME,
               strlen(SZSERVICEDISPLAYNAME) + 1);
    RegCloseKey(hkey);
    if (rv != ERROR_SUCCESS) {
        printf( "Could not add DisplayName to our Registry Key\r\n");
        return FALSE;
    }
    return TRUE;
}

/*
 * Fill the registry with the environment variables
 */
BOOL InstallEnv (char *var, char *value)
{
    BOOL        installed;
    HKEY        hKey;

    installed = FALSE;
    /* create the parameters registry tree */
        log_debug("InstallEnv: %s:%s",var,value);
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, SZKEY_ONSERVE, 0,
            NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,
            &hKey, NULL)) {
            /* key is created or opened */
            RegSetValueEx(hKey,var,0,REG_SZ,(BYTE *)value,lstrlen(value)+1);
            RegCloseKey(hKey);
            installed = TRUE;
            }
    return installed;
} /* InstallEnv */

/*
 * Add the arguments to start jsvc like -Dcatalina.home=/home/jakarta/tomcat.
 */
BOOL InstallEnvParm(int i,char *value)
{
    BOOL        installed;
    HKEY        hKey;
    char var[64];

    sprintf(var,"arg%d",i);
        log_debug("InstallEnvParm: %s:%s",var,value);

    installed = FALSE;
    /* create the parameters registry tree */
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE,
            SZKEY_ONSERVEARG, 0,
            NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,
            &hKey, NULL)) {
            /* key is created or opened */
            RegSetValueEx(hKey,var,0,REG_SZ,
                (BYTE *)value,lstrlen(value)+1);
            RegCloseKey(hKey);
            installed = TRUE;
    }
    return installed;
}

/*
 * Remove the created keys
 */
BOOL RemoveEnv()
{
    HKEY hkey;
    DWORD rv;

        log_debug("RemoveEnv");

    rv = RegOpenKey(HKEY_LOCAL_MACHINE,
        NULL,
        &hkey);
    if (rv != ERROR_SUCCESS) {
        printf( "Could not open the jsvc registry key.\r\n");
        return FALSE;
    }
    rv = RegDeleteKey(hkey, SZKEY_ONSERVE);
    RegCloseKey(hkey);
    if (rv != ERROR_SUCCESS)
        printf( "Could not delete the jsvc entry.\r\n");

        /* remove the key tree if empty */

    return TRUE;
}


/*
 * Install or remove the OnServe service and Key in the registry.
 * no parameter install the OnServe.
 * -REMOVE: desinstall the OnServe service and Keys.
 */

INT main (INT argc, CHAR *argv[])
{
    BOOL done;
    arg_data *args=NULL;
    char szPath[512];
    char szExePath[512];
    int i;

    printf( "\r\n - Copyright (c) 2001 The Apache Software Foundation. \r\n");
    printf( "\r\n");
    if (GetModuleFileName(NULL, szPath, sizeof(szPath))) {
        printf( "%s\r\n",szPath);
    }

    args=arguments(argc,argv);
    if (args==NULL) {
        Usage();
        return(1);
    }


    if (args->install==true) {
        if (args->home==NULL) {
            printf( "home empty or not defined...\r\n\r\n");
            Usage();
            return(1);
        }
        if (args->clas==NULL) {
            printf( "startclass empty or not defined...\r\n\r\n");
            Usage();
            return(1);
        }
        printf( "\r\ninstalling...\r\n\r\n");

        /* get the patch from the executable name */
        for(i=strlen(szPath);i>0;i--)
            if (szPath[i]=='\\') {
                szPath[i]='\0';
                break;
            }
        strcpy(szExePath,szPath);
        strcat(szExePath,SZDEFMONISVCPATH);
        /* install jsvcservice.exe as a service */
        if (isWindowsNT())
            done = InstallSvcNT(szExePath);
        else
            done = InstallSvc(szExePath);

        if (done)
            printf( "InstallSvc done\r\n");
        else
            printf( "InstallSvc failed\r\n");

        /* install the environment variable in registry */

        /* should get it from szPath */
        InstallEnv("JAKARTA_HOME",szPath);

        InstallEnv("CYGWIN",SZCYGWINPATH); /* need APR to get ride of it */

        InstallEnv("JAVA_HOME",args->home);

        InstallEnv("STARTCLASS",args->clas);

        if (args->onum==0) return(0);

        for(i=0;i<args->onum;i++)
            InstallEnvParm(i,args->opts[i]);

        return(0);
    }

    if (args->remove==true) {
        /* remove the  service. removing the keys not yet done!!! */
        printf( "\r\n - removing Java Service...\r\n\r\n");
        if (isWindowsNT())
            done = RemoveSvcNT();
        else
            done = RemoveSvc();
        if (!done) {
            printf( "\r\n - REMOVE FAILED....\r\n\r\n");
            return(2);
        }
        RemoveEnv();
        return(0);
    }
    printf( "\r\nonly -install or -remove supported...\r\n\r\n");
    Usage();
    return(1);
}
