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

#include "apxwin.h"
#include "private.h"
#include <stdio.h>
#include <accctrl.h>
#include <aclapi.h>

DWORD
apxSecurityGrantFileAccessToUser(
    LPCWSTR szPath,
    LPCWSTR szUser)
{
    WCHAR sPath[SIZ_PATHLEN];
    WCHAR sUser[SIZ_RESLEN];
    DWORD dwResult;
    PACL pOldDACL;
    PACL pNewDACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESS ea;

    if (szPath) { 
        lstrlcpyW(sPath, SIZ_PATHLEN, szPath);
    } else {
        dwResult = GetSystemDirectoryW(sPath, MAX_PATH);
        if (dwResult) {
            goto cleanup;
        }
        lstrlcatW(sPath, MAX_PATH, LOG_PATH_DEFAULT);
    }
    if (szUser) {
        /* The API used to set file permissions doesn't always recognised the
         * same users as the API used to configured services. We do any
         * necessary conversion here. The known issues are:
         * LocalSystem is not recognised. It needs to be converted to
         * "NT Authority\System"
         * User names for the local machine that use the ".\username" form need
         * to have the leading ".\" removed.
         */
        if (!StrCmpW(STAT_SYSTEM, szUser)) {
            lstrlcpyW(sUser, SIZ_RESLEN, STAT_SYSTEM_WITH_DOMAIN);
        } else {
            if (StrStrW(szUser, L".\\") == szUser) {
                szUser +=2;
            }
            lstrlcpyW(sUser, SIZ_RESLEN, szUser);
        }
    } else {
        lstrlcpyW(sUser, SIZ_RESLEN, DEFAULT_SERVICE_USER);
    }
    
    /* Old (current) ACL. */
    dwResult = GetNamedSecurityInfoW(
            sPath,
            SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION,
            NULL,
            NULL,
            &pOldDACL,
            NULL,
            &pSD);
    if (dwResult) {
        goto cleanup;
    }

    /* Additional access. */
    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
    ea.grfAccessPermissions = GENERIC_EXECUTE + GENERIC_READ + GENERIC_WRITE;
    ea.grfAccessMode = SET_ACCESS;
    ea.grfInheritance = CONTAINER_INHERIT_ACE + OBJECT_INHERIT_ACE;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    ea.Trustee.ptstrName = sUser;

    /* Merge old and additional into new ACL. */
    dwResult = SetEntriesInAcl(1, &ea, pOldDACL, &pNewDACL);
    if (dwResult) {
        goto cleanup;
    }

    /* Set the new ACL. */
    dwResult = SetNamedSecurityInfoW(
            sPath,
            SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION,
            NULL,
            NULL,
            pNewDACL,
            NULL);
    if (dwResult) {
        goto cleanup;
    }

cleanup:
    if (pSD != NULL) {
        LocalFree((HLOCAL) pSD);
    }
    if (pNewDACL != NULL) {
        LocalFree((HLOCAL) pNewDACL);
    }

    return dwResult;
}
 