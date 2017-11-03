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
 
#ifndef _REGISTRY_H_INCLUDED_
#define _REGISTRY_H_INCLUDED_

__APXBEGIN_DECLS

#define APXREG_SOFTWARE         0x0001
#define APXREG_SERVICE          0x0002
#define APXREG_USER             0x0004

#define APXREG_PARAMSOFTWARE    0x0010
#define APXREG_PARAMSERVICE     0x0020
#define APXREG_PARAMUSER        0x0040

/** Create or open the process registry keys
 */
APXHANDLE apxCreateRegistryW(APXHANDLE hPool, REGSAM samDesired,
                             LPCWSTR szRoot, LPCWSTR szKeyName,
                             DWORD dwOptions);

/** Delete the process registry keys
 *  samDesired only needs to be KREG_WOW6432 or 0
 */
BOOL      apxDeleteRegistryW(LPCWSTR szRoot, LPCWSTR szKeyName,
                            REGSAM samDesired, BOOL bDeleteEmptyRoot);

/** Get the JavaHome path from registry
 * and set the JAVA_HOME environment variable if not found
 * If bPreferJre is set use the JRE's path as JAVA_HOME
 */
LPWSTR    apxGetJavaSoftHome(APXHANDLE hPool, BOOL bPreferJre);

/** Get the Java RuntimeLib from registry (jvm.dll)
 */
LPWSTR    apxGetJavaSoftRuntimeLib(APXHANDLE hPool);

LPWSTR    apxRegistryGetStringW(APXHANDLE hRegistry, DWORD dwFrom,
                                LPCWSTR szSubkey, LPCWSTR szValueName);

BOOL    apxRegistrySetBinaryA(APXHANDLE hRegistry, DWORD dwFrom,
                              LPCSTR szSubkey, LPCSTR szValueName,
                              const LPBYTE lpData, DWORD dwLength);

BOOL    apxRegistrySetBinaryW(APXHANDLE hRegistry, DWORD dwFrom,
                              LPCWSTR szSubkey, LPCWSTR szValueName,
                              const LPBYTE lpData, DWORD dwLength);

LPWSTR  apxRegistryGetMzStrW(APXHANDLE hRegistry, DWORD dwFrom,
                             LPCWSTR szSubkey, LPCWSTR szValueName,
                             LPWSTR lpData, LPDWORD lpdwLength);

BOOL    apxRegistrySetMzStrW(APXHANDLE hRegistry, DWORD dwFrom,
                             LPCWSTR szSubkey, LPCWSTR szValueName,
                             LPCWSTR lpData, DWORD dwLength);

BOOL    apxRegistrySetStrW(APXHANDLE hRegistry, DWORD dwFrom,
                           LPCWSTR szSubkey, LPCWSTR szValueName,
                           LPCWSTR szValue);


BOOL    apxRegistrySetNumW(APXHANDLE hRegistry, DWORD dwFrom,
                           LPCWSTR szSubkey, LPCWSTR szValueName,
                           DWORD dwValue);

DWORD   apxRegistryGetNumberW(APXHANDLE hRegistry, DWORD dwFrom,
                              LPCWSTR szSubkey, LPCWSTR szValueName);


BOOL    apxRegistryDeleteW(APXHANDLE hRegistry, DWORD dwFrom,
                           LPCWSTR szSubkey, LPCWSTR szValueName);

__APXEND_DECLS

#endif /* _REGISTRY_H_INCLUDED_ */
