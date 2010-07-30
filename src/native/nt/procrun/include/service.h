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
 
#ifndef _SERVICE_H_INCLUDED_
#define _SERVICE_H_INCLUDED_

__APXBEGIN_DECLS

typedef struct APXSERVENTRY {
    WCHAR   szServiceName[SIZ_RESLEN];
    WCHAR   szObjectName[SIZ_RESLEN];
    WCHAR   szServiceDescription[SIZ_DESLEN];
    DWORD   dwStart;
    LPQUERY_SERVICE_CONFIGW lpConfig;
    SERVICE_STATUS          stServiceStatus;
    SERVICE_STATUS_PROCESS  stStatusProcess;

} APXSERVENTRY, *LPAPXSERVENTRY;


APXHANDLE   apxCreateService(APXHANDLE hPool, DWORD dwOptions,
                             BOOL bManagerMode);

BOOL        apxServiceOpen(APXHANDLE hService, LPCWSTR szServiceName, DWORD dwOptions);


BOOL        apxServiceSetNames(APXHANDLE hService, LPCWSTR szImagePath,
                               LPCWSTR szDisplayName, LPCWSTR szDescription,
                               LPCWSTR szUsername, LPCWSTR szPassword);

BOOL        apxServiceSetOptions(APXHANDLE hService, DWORD dwServiceType,
                                 DWORD dwStartType, DWORD dwErrorControl);

BOOL        apxServiceControl(APXHANDLE hService, DWORD dwControl, UINT uMsg,
                              LPAPXFNCALLBACK fnControlCallback,
                              LPVOID lpCbData);
BOOL        apxServiceInstall(APXHANDLE hService, LPCWSTR szServiceName,
                              LPCWSTR szDisplayName, LPCWSTR szImagePath,
                              LPCWSTR lpDependencies, DWORD dwServiceType,
                              DWORD dwStartType);

LPAPXSERVENTRY  apxServiceEntry(APXHANDLE hService, BOOL bRequeryStatus);

/** Delete te service
 * Stops the service if running
 */
BOOL        apxServiceDelete(APXHANDLE hService);

DWORD       apxServiceBrowse(APXHANDLE hService,
                             LPCWSTR szIncludeNamePattern,
                             LPCWSTR szIncludeImagePattern,
                             LPCWSTR szExcludeNamePattern,
                             LPCWSTR szExcludeImagePattern,
                             UINT uMsg,
                             LPAPXFNCALLBACK fnDisplayCallback,
                             LPVOID lpCbData);

DWORD       apxGetMaxServiceTimeout(APXHANDLE hPool);

__APXEND_DECLS

#endif /* _SERVICE_H_INCLUDED_ */
