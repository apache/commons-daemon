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

#ifndef _RPROCESS_H_INCLUDED_
#define _RPROCESS_H_INCLUDED_

__APXBEGIN_DECLS

BOOL        apxProcessExecute(APXHANDLE hProcess);

APXHANDLE   apxCreateProcessW(APXHANDLE hPool, DWORD dwOptions,
                              LPAPXFNCALLBACK fnCallback,
                              LPCWSTR szUsername, LPCWSTR szPassword,
                              BOOL bLogonAsService);

BOOL        apxProcessSetExecutableW(APXHANDLE hProcess, LPCWSTR szName);

BOOL        apxProcessSetCommandLineW(APXHANDLE hProcess, LPCWSTR szCmdline);
BOOL        apxProcessSetCommandArgsW(APXHANDLE hProcess, LPCWSTR szTitle,
                                      DWORD dwArgc, LPCWSTR *lpArgs);

BOOL        apxProcessSetWorkingPathW(APXHANDLE hProcess, LPCWSTR szPath);

DWORD       apxProcessWrite(APXHANDLE hProcess, LPCVOID lpData, DWORD dwLen);

VOID        apxProcessCloseInputStream(APXHANDLE hProcess);
BOOL        apxProcessFlushStdin(APXHANDLE hProcess);

DWORD       apxProcessWait(APXHANDLE hProcess, DWORD dwMilliseconds,
                           BOOL bKill);

BOOL        apxProcessRunning(APXHANDLE hProcess);
DWORD       apxProcessGetPid(APXHANDLE hProcess);


__APXEND_DECLS

#endif /* _RPROCESS_H_INCLUDED_ */
