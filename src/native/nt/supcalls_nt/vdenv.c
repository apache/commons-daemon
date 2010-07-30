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

/* Read the Win-NT register and set the jsvc environment variable. */
/* XXX We should use a property file instead  registry */

/* XXX Set the PATH (for dynamic linking!) what about libapr*.so? */

#include <windows.h>
#include "moni_inst.h"

#define ENVSIZE 1024

int MySetEnvironmentVariable(char *name, char *data)
{
char Variable[ENVSIZE];

  strcpy(Variable,name);
  strcat(Variable,"=");
  strcat(Variable,data);
  if (putenv(Variable)) return(-1);
  return(0);
}
/*
 *  FUNCTION: OnServeSetEnv()
 *
 *  PURPOSE: Actual code of the routine that reads the registry and
 *           set the OnServe environment variables.
 *           The PATH is needed for the dynamic linking.
 *
 *  RETURN VALUE:
 *    0 : All OK.
 *    <0: Something Failed. (Registry cannot be read or one key cannot be read).
 *
*/
int OnServeSetEnv ()
{
HKEY    hKey=NULL;
DWORD   Type;
char    jakarta_home[ENVSIZE]; /* for the path */
char    cygwin[ENVSIZE]; /* for the path */
char    Data[ENVSIZE];
DWORD   LData;
int     qreturn=0;


    /* Read the registry and set environment. */
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZKEY_ONSERVE,
                        0, KEY_READ,&hKey) !=  ERROR_SUCCESS)
      return(-1);

    /* read key and set environment. */

    /* JAKARTA_HOME */
    LData = sizeof(Data);
    if (RegQueryValueEx(hKey,"JAKARTA_HOME",NULL,&Type,Data,&LData)==ERROR_SUCCESS) {
      strcpy(jakarta_home,Data);
      MySetEnvironmentVariable("JAKARTA_HOME",Data);
      }
    else 
      qreturn = -2;

    /* CYGWIN */
    LData = sizeof(Data);
    if (RegQueryValueEx(hKey,"CYGWIN",NULL,&Type,Data,&LData)==ERROR_SUCCESS) {
      strcpy(cygwin,Data);
      MySetEnvironmentVariable("CYGWIN",Data);
      }
    else 
      qreturn = -3;

    /* JAVA_HOME */
    LData = sizeof(Data);
    if (RegQueryValueEx(hKey,"JAVA_HOME",NULL,&Type,Data,&LData)
        ==ERROR_SUCCESS) {
      MySetEnvironmentVariable("JAVA_HOME",Data);
      }
    else 
      qreturn = -4;

    RegCloseKey(hKey);
    hKey = NULL;

    /* set the PATH otherwise nothing works!!! */
    LData = sizeof(Data);                                                       
    if (!GetEnvironmentVariable("PATH",Data,LData)) {                           
      strcpy(Data,jakarta_home);
      }
    else {
      strcat(Data,";");
      strcat(Data,jakarta_home);
      }
    strcat(Data,"\\bin");

    strcat(Data,";");
    strcat(Data,cygwin);
    strcat(Data,"\\bin");

    MySetEnvironmentVariable("PATH",Data);

    return(qreturn);
}

/*
 * Build the jsvc.exe command using the registry information.
 */
int BuildCommand(char *data)
{
    int  i;
    LONG lRet;
    DWORD dwIndex;
    char name[128];
    DWORD lname;
    char value[256];
    DWORD lvalue;
    DWORD nvalue;
    DWORD   Type;
    HKEY    hKey=NULL;


    strcat(data,"\\jsvc.exe -nodetach ");

    /* Read the registry and set environment. */
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZKEY_ONSERVEARG,
                     0, KEY_READ,&hKey) !=  ERROR_SUCCESS)
        return(-1);
    if (RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL,
                        &nvalue,NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
        return(-2);

    /* Read the arguments */
    for (i=0;i<nvalue;i++) {
        lname = sizeof(name);
        lvalue = sizeof(value);

        lRet = RegEnumValue (hKey, i, name, &lname, NULL, NULL,
                             value, &lvalue);
        if (lRet != ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return(-3);
        }

        strncat(data,value,lvalue);
        strcat(data," ");
    }
    RegCloseKey(hKey);

    /* Read the start class. */
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZKEY_ONSERVE,
                        0, KEY_READ,&hKey) !=  ERROR_SUCCESS)
        return(-4);
    lvalue = sizeof(value);
    if (RegQueryValueEx(hKey,"STARTCLASS",NULL,&Type,value,&lvalue)
        !=ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return(-5);
    }
    RegCloseKey(hKey);

    strncat(data,value,lvalue);

    return(0);
}
