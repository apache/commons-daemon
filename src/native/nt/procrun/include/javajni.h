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
 
#ifndef _JAVAJNI_H_INCLUDED_
#define _JAVAJNI_H_INCLUDED_

__APXBEGIN_DECLS

APXHANDLE   apxCreateJava(APXHANDLE hPool, LPCWSTR szJvmDllPath);

BOOL        apxJavaInitialize(APXHANDLE hJava, LPCSTR szClassPath,
                              LPCVOID lpOptions, DWORD dwMs, DWORD dwMx,
                              DWORD dwSs, BOOL bReduceSignals);

BOOL        apxJavaLoadMainClass(APXHANDLE hJava, LPCSTR szClassName,
                                 LPCSTR szMethodName,
                                 LPCVOID lpArguments);

BOOL        apxJavaStart(APXHANDLE hJava);

DWORD       apxJavaWait(APXHANDLE hJava, DWORD dwMilliseconds, BOOL bKill);

BOOL        apxJavaSetOut(APXHANDLE hJava, BOOL setErrorOrOut,
                          LPCWSTR szFilename);


__APXEND_DECLS

#endif /* _JAVAJNI_H_INCLUDED_ */
