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

#ifndef _JAVAJNI_H_INCLUDED_
#define _JAVAJNI_H_INCLUDED_

__APXBEGIN_DECLS

#define     APX_JVM_DESTROY 0x00000001

typedef struct stAPXJAVA_THREADARGS
{
    LPVOID      hJava;
    LPCSTR      szClassPath;
    LPCVOID     lpOptions;
    DWORD       dwMs;
    DWORD       dwMx;
    DWORD       dwSs;
    DWORD       bJniVfprintf;
    LPCSTR      szClassName;
    LPCSTR      szMethodName;
    LPCVOID     lpArguments;
    BOOL        setErrorOrOut;
    LPCWSTR     szStdErrFilename;
    LPCWSTR     szStdOutFilename;
    LPCWSTR     szLibraryPath;
} APXJAVA_THREADARGS, *LPAPXJAVA_THREADARGS;

APXHANDLE   apxCreateJava(APXHANDLE hPool, LPCWSTR szJvmDllPath);

BOOL        apxJavaInitialize(APXHANDLE hJava, LPCSTR szClassPath,
                              LPCVOID lpOptions, DWORD dwMs, DWORD dwMx,
                              DWORD dwSs, DWORD bJniVfprintf);
DWORD
apxJavaCmdInitialize(APXHANDLE hPool, LPCWSTR szClassPath, LPCWSTR szClass,
                     LPCWSTR szOptions, DWORD dwMs, DWORD dwMx,
                     DWORD dwSs, LPCWSTR szCmdArgs, LPWSTR **lppArray);

BOOL        apxJavaLoadMainClass(APXHANDLE hJava, LPCSTR szClassName,
                                 LPCSTR szMethodName,
                                 LPCVOID lpArguments);

BOOL        apxJavaStart(LPAPXJAVA_THREADARGS pArgs);

DWORD       apxJavaWait(APXHANDLE hJava, DWORD dwMilliseconds, BOOL bKill);

BOOL        apxJavaSetOut(APXHANDLE hJava, BOOL setErrorOrOut,
                          LPCWSTR szFilename);
DWORD       apxJavaSetOptions(APXHANDLE hJava, DWORD dwOptions);

BOOL        apxDestroyJvm(DWORD dwTimeout);

DWORD       apxGetVmExitCode();

void        apxSetVmExitCode(DWORD exitCode);

void        apxJavaDumpAllStacks(APXHANDLE hJava);

__APXEND_DECLS

#endif /* _JAVAJNI_H_INCLUDED_ */
