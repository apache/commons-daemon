/* ========================================================================= *
 *                                                                           *
 *                 The Apache Software License,  Version 1.1                 *
 *                                                                           *
 *          Copyright (c) 1999-2001 The Apache Software Foundation.          *
 *                           All rights reserved.                            *
 *                                                                           *
 * ========================================================================= *
 *                                                                           *
 * Redistribution and use in source and binary forms,  with or without modi- *
 * fication, are permitted provided that the following conditions are met:   *
 *                                                                           *
 * 1. Redistributions of source code  must retain the above copyright notice *
 *    notice, this list of conditions and the following disclaimer.          *
 *                                                                           *
 * 2. Redistributions  in binary  form  must  reproduce the  above copyright *
 *    notice,  this list of conditions  and the following  disclaimer in the *
 *    documentation and/or other materials provided with the distribution.   *
 *                                                                           *
 * 3. The end-user documentation  included with the redistribution,  if any, *
 *    must include the following acknowlegement:                             *
 *                                                                           *
 *       "This product includes  software developed  by the Apache  Software *
 *        Foundation <http://www.apache.org/>."                              *
 *                                                                           *
 *    Alternately, this acknowlegement may appear in the software itself, if *
 *    and wherever such third-party acknowlegements normally appear.         *
 *                                                                           *
 * 4. The names  "The  Jakarta  Project",  "WebApp",  and  "Apache  Software *
 *    Foundation"  must not be used  to endorse or promote  products derived *
 *    from this  software without  prior  written  permission.  For  written *
 *    permission, please contact <apache@apache.org>.                        *
 *                                                                           *
 * 5. Products derived from this software may not be called "Apache" nor may *
 *    "Apache" appear in their names without prior written permission of the *
 *    Apache Software Foundation.                                            *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES *
 * INCLUDING, BUT NOT LIMITED TO,  THE IMPLIED WARRANTIES OF MERCHANTABILITY *
 * AND FITNESS FOR  A PARTICULAR PURPOSE  ARE DISCLAIMED.  IN NO EVENT SHALL *
 * THE APACHE  SOFTWARE  FOUNDATION OR  ITS CONTRIBUTORS  BE LIABLE  FOR ANY *
 * DIRECT,  INDIRECT,   INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR  CONSEQUENTIAL *
 * DAMAGES (INCLUDING,  BUT NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE GOODS *
 * OR SERVICES;  LOSS OF USE,  DATA,  OR PROFITS;  OR BUSINESS INTERRUPTION) *
 * HOWEVER CAUSED AND  ON ANY  THEORY  OF  LIABILITY,  WHETHER IN  CONTRACT, *
 * STRICT LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN *
 * ANY  WAY  OUT OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF  ADVISED  OF THE *
 * POSSIBILITY OF SUCH DAMAGE.                                               *
 *                                                                           *
 * ========================================================================= *
 *                                                                           *
 * This software  consists of voluntary  contributions made  by many indivi- *
 * duals on behalf of the  Apache Software Foundation.  For more information *
 * on the Apache Software Foundation, please see <http://www.apache.org/>.   *
 *                                                                           *
 * ========================================================================= */

/* @version $Id: vdenv.c,v 1.1 2003/09/04 23:28:20 yoavs Exp $ */

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
DWORD	Type;
char	jakarta_home[ENVSIZE]; /* for the path */
char	cygwin[ENVSIZE]; /* for the path */
char	Data[ENVSIZE];
DWORD	LData;
int	qreturn=0;


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
    int	i;
    LONG lRet;
    DWORD dwIndex;
    char name[128];
    DWORD lname;
    char value[256];
    DWORD lvalue;
    DWORD nvalue;
    DWORD	Type;
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
