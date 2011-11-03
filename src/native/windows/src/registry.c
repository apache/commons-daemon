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

static LPCWSTR REGSERVICE_ROOT  = L"SYSTEM\\CurrentControlSet\\Services\\";
static LPCWSTR REGSOFTWARE_ROOT = L"SOFTWARE\\";
static LPCWSTR REGSERVICE_START = L"Start";
static LPCWSTR REGSERVICE_USER  = L"ObjectName";
static LPCWSTR REGPARAMS        = L"Parameters";
static LPCWSTR REGDESCRIPTION   = L"Description";
static LPCWSTR REGSEPARATOR     = L"\\";
static LPCWSTR REGAPACHE_ROOT   = L"Apache Software Foundation";
/* predefined java keys */
static LPCWSTR JRE_REGKEY       = L"SOFTWARE\\JavaSoft\\Java Runtime Environment\\";
static LPCWSTR JDK_REGKEY       = L"SOFTWARE\\JavaSoft\\Java Development Kit\\";
static LPCWSTR JAVA_CURRENT     = L"CurrentVersion";
static LPCWSTR JAVA_RUNTIME     = L"RuntimeLib";
static LPCWSTR JAVA_HOME        = L"JAVA_HOME";
static LPCWSTR JAVAHOME         = L"JavaHome";
static LPCWSTR CONTROL_REGKEY   = L"SYSTEM\\CurrentControlSet\\Control";
static LPCWSTR REGTIMEOUT       = L"WaitToKillServiceTimeout";

#define REG_CAN_CREATE(r)   \
    ((r)->samOptions & KEY_CREATE_SUB_KEY)

#define REG_CAN_WRITE(r)   \
    ((r)->samOptions & KEY_SET_VALUE)


#define REG_GET_KEY(r, w, k)    \
    APXMACRO_BEGIN              \
    switch(w) {                                                     \
        case APXREG_SOFTWARE:       k = (r)->hRootKey;      break;  \
        case APXREG_PARAMSOFTWARE:  k = (r)->hRparamKey;    break;  \
        case APXREG_SERVICE:        k = (r)->hServKey;      break;  \
        case APXREG_PARAMSERVICE:   k = (r)->hSparamKey;    break;  \
        case APXREG_USER:           k = (r)->hUserKey;      break;  \
        case APXREG_PARAMUSER:      k = (r)->hUparamKey;    break;  \
        default: k = NULL; break;                                   \
    } APXMACRO_END


typedef struct APXREGISTRY  APXREGISTRY;
typedef APXREGISTRY*        LPAPXREGISTRY;
typedef struct APXREGSUBKEY APXREGSUBKEY;

struct APXREGSUBKEY {
    APXHANDLE   hRegistry;
    HKEY        hKey;
    LPCTSTR     syKeyName;
    TAILQ_ENTRY(APXREGSUBKEY);
};

struct APXREGISTRY {
    HKEY    hRootKey;   /* root key */
    HKEY    hServKey;   /* service key */
    HKEY    hUserKey;   /* user key */
    HKEY    hCurrKey;   /* Current opened key */
    LPVOID  pCurrVal;   /* Current value, overwitten on a next call */
    HKEY    hRparamKey; /* root\\Parameters */
    HKEY    hSparamKey; /* service\\Parameters */
    HKEY    hUparamKey; /* service\\Parameters */
    REGSAM  samOptions;
    /** list enty for opened subkeys  */
    TAILQ_HEAD(_lSubkeys, APXREGSUBKEY) lSubkeys;

};

#define SAFE_CLOSE_KEY(k) \
    if ((k) != NULL && (k) != INVALID_HANDLE_VALUE) {   \
        RegCloseKey((k));                               \
        (k) = NULL;                                     \
    }

static BOOL __apxRegistryCallback(APXHANDLE hObject, UINT uMsg,
                                 WPARAM wParam, LPARAM lParam)
{
    LPAPXREGISTRY lpReg;

    lpReg = APXHANDLE_DATA(hObject);
    switch (uMsg) {
        case WM_CLOSE:
        SAFE_CLOSE_KEY(lpReg->hCurrKey);
        SAFE_CLOSE_KEY(lpReg->hRparamKey);
        SAFE_CLOSE_KEY(lpReg->hSparamKey);
        SAFE_CLOSE_KEY(lpReg->hUparamKey);
        SAFE_CLOSE_KEY(lpReg->hRootKey);
        SAFE_CLOSE_KEY(lpReg->hServKey);
        SAFE_CLOSE_KEY(lpReg->hUserKey);
        break;
        default:
        break;
    }
    return TRUE;
}

LPSTR __apxGetRegistrySzA(APXHANDLE hPool, HKEY hKey, LPCSTR szValueName)
{
    LPSTR  szRet;
    DWORD  rc;
    DWORD  dwType;
    DWORD  dwSize;

    rc = RegQueryValueExA(hKey, szValueName, NULL, &dwType, NULL, &dwSize);
    if (rc != ERROR_SUCCESS || dwType != REG_SZ) {
        return NULL;
    }
    if (!(szRet = apxPoolAlloc(hPool, dwSize)))
        return NULL;
    RegQueryValueExA(hKey, szValueName, NULL, &dwType, (LPBYTE)szRet, &dwSize);

    return szRet;
}

LPWSTR __apxGetRegistrySzW(APXHANDLE hPool, HKEY hKey, LPCWSTR wsValueName)
{
    LPWSTR wsRet;
    DWORD  rc;
    DWORD  dwType;
    DWORD  dwSize;

    rc = RegQueryValueExW(hKey, wsValueName, NULL, &dwType, NULL, &dwSize);
    if (rc != ERROR_SUCCESS || dwType != REG_SZ) {
        return NULL;
    }
    if (!(wsRet = apxPoolAlloc(hPool, dwSize * sizeof(WCHAR))))
        return NULL;
    RegQueryValueExW(hKey, wsValueName, NULL, &dwType, (LPBYTE)wsRet, &dwSize);

    return wsRet;
}

BOOL __apxGetRegistryStrW(APXHANDLE hPool, HKEY hKey, LPCWSTR wsValueName,
                          LPWSTR lpRetval, DWORD dwMaxLen)
{
    DWORD  rc;
    DWORD  dwType;
    DWORD  dwSize = dwMaxLen;

    rc = RegQueryValueExW(hKey, wsValueName, NULL, &dwType, (LPBYTE)lpRetval, &dwSize);
    if (rc != ERROR_SUCCESS || dwType != REG_SZ) {
        lpRetval = L'\0';
        return FALSE;
    }
    else
        return TRUE;
}

LPBYTE __apxGetRegistryBinaryA(APXHANDLE hPool, HKEY hKey, LPCSTR szValueName,
                               LPDWORD lpdwLength)
{
    LPBYTE lpRet;
    DWORD  rc;
    DWORD  dwType;
    DWORD  dwSize;

    rc = RegQueryValueExA(hKey, szValueName, NULL, &dwType, NULL, &dwSize);
    if (rc != ERROR_SUCCESS || dwSize == 0) {
        return NULL;
    }
    if (!(lpRet = apxPoolAlloc(hPool, dwSize)))
        return NULL;
    RegQueryValueExA(hKey, szValueName, NULL, &dwType, lpRet, &dwSize);
    if (lpdwLength)
        *lpdwLength = dwSize;
    return lpRet;
}

LPBYTE __apxGetRegistryBinaryW(APXHANDLE hPool, HKEY hKey, LPCWSTR wsValueName,
                               LPDWORD lpdwLength)
{
    LPBYTE lpRet;
    DWORD  rc;
    DWORD  dwType;
    DWORD  dwSize;

    rc = RegQueryValueExW(hKey, wsValueName, NULL, &dwType, NULL, &dwSize);
    if (rc != ERROR_SUCCESS || dwSize == 0) {
        return NULL;
    }
    if (!(lpRet = apxPoolAlloc(hPool, dwSize)))
        return NULL;
    RegQueryValueExW(hKey, wsValueName, NULL, &dwType, lpRet, &dwSize);
    if (lpdwLength)
        *lpdwLength = dwSize;

    return lpRet;
}

DWORD __apxGetRegistryDwordW(APXHANDLE hPool, HKEY hKey, LPCWSTR wsValueName)
{
    DWORD  dwRet;
    DWORD  rc;
    DWORD  dwType;
    DWORD  dwSize = sizeof(DWORD);

    rc = RegQueryValueExW(hKey, wsValueName, NULL, &dwType, (LPBYTE)&dwRet, &dwSize);
    if (rc != ERROR_SUCCESS || dwType != REG_DWORD) {
        return 0xFFFFFFFF;
    }

    return dwRet;
}


APXHANDLE
apxCreateRegistryW(APXHANDLE hPool, REGSAM samDesired,
                   LPCWSTR szRoot,
                   LPCWSTR szKeyName,
                   DWORD dwOptions)
{
    APXHANDLE hRegistry;
    LPAPXREGISTRY lpReg;
     /* maximum key length is 512 characters. */
    WCHAR     buff[SIZ_BUFLEN];
    LONG      rc = ERROR_SUCCESS;
    HKEY      hRootKey   = NULL;
    HKEY      hUserKey   = NULL;
    HKEY      hServKey   = NULL;
    HKEY      hRparamKey = NULL;
    HKEY      hSparamKey = NULL;
    HKEY      hUparamKey = NULL;

    if (!szKeyName || lstrlenW(szKeyName) > SIZ_RESMAX) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    if (szRoot && lstrlenW(szRoot) > SIZ_RESMAX) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* make the HKLM\\SOFTWARE key */
    lstrcpyW(buff, REGSOFTWARE_ROOT);
    if (szRoot)
        lstrcatW(buff, szRoot);
    else
        lstrcatW(buff, REGAPACHE_ROOT);
    lstrcatW(buff, REGSEPARATOR);
    lstrcatW(buff, szKeyName);
    /* Open or create the root key */
    if (dwOptions & APXREG_SOFTWARE) {
        if (samDesired & KEY_CREATE_SUB_KEY)
            rc = RegCreateKeyExW(HKEY_LOCAL_MACHINE, buff, 0, NULL, 0,
                                 samDesired, NULL,
                                 &hRootKey, NULL);
        else
            rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, buff, 0,
                               samDesired, &hRootKey);
        if (rc != ERROR_SUCCESS) {
            hRootKey = NULL;
            goto cleanup;
        }
        /* Open or create the root parameters key */
        if (samDesired & KEY_CREATE_SUB_KEY)
            rc = RegCreateKeyExW(hRootKey, REGPARAMS, 0, NULL, 0,
                                 samDesired, NULL,
                                 &hRparamKey, NULL);
        else
            rc = RegOpenKeyExW(hRootKey, REGPARAMS, 0,
                               samDesired, &hRparamKey);
        if (rc != ERROR_SUCCESS) {
            hRparamKey = NULL;
            goto cleanup;
        }
    }

    if (dwOptions & APXREG_USER) {
        /* Open or create the users root key */
        if (samDesired & KEY_CREATE_SUB_KEY)
            rc = RegCreateKeyExW(HKEY_CURRENT_USER, buff, 0, NULL, 0,
                                 samDesired, NULL,
                                 &hUserKey, NULL);
        else
            rc = RegOpenKeyExW(HKEY_CURRENT_USER, buff, 0,
                               samDesired, &hUserKey);
        if (rc != ERROR_SUCCESS) {
            hUserKey = NULL;
            goto cleanup;
        }
        /* Open or create the users parameters key */
        if (samDesired & KEY_CREATE_SUB_KEY)
            rc = RegCreateKeyExW(hUserKey, REGPARAMS, 0, NULL, 0,
                                 samDesired, NULL,
                             &hUparamKey, NULL);
        else
            rc = RegOpenKeyExW(hUserKey, REGPARAMS, 0,
                               samDesired, &hUparamKey);
        if (rc != ERROR_SUCCESS) {
            hUparamKey = NULL;
            goto cleanup;
        }
    }
    /* Check if we need a service key */
    if (dwOptions & APXREG_SERVICE) {
        lstrcpyW(buff, REGSERVICE_ROOT);
        lstrcatW(buff, szKeyName);
        /* Service has to be created allready */
        rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, buff, 0,
                           samDesired, &hServKey);
        if (rc != ERROR_SUCCESS) {
            hServKey = NULL;
            goto cleanup;
        }
        /* Open or create the root parameters key */
        if (samDesired & KEY_CREATE_SUB_KEY)
            rc = RegCreateKeyExW(hServKey, REGPARAMS, 0, NULL, 0,
                                 samDesired, NULL,
                                 &hSparamKey, NULL);
        else
            rc = RegOpenKeyExW(hServKey, REGPARAMS, 0,
                               samDesired, &hSparamKey);
        if (rc != ERROR_SUCCESS) {
            hSparamKey = NULL;
            goto cleanup;
        }
    }
    hRegistry = apxHandleCreate(hPool, 0,
                                NULL, sizeof(APXREGISTRY),
                                __apxRegistryCallback);
    if (IS_INVALID_HANDLE(hRegistry))
        return NULL;
    hRegistry->dwType = APXHANDLE_TYPE_REGISTRY;
    lpReg = APXHANDLE_DATA(hRegistry);
    lpReg->samOptions = samDesired;
    lpReg->hRootKey   = hRootKey;
    lpReg->hUserKey   = hUserKey;
    lpReg->hServKey   = hServKey;
    lpReg->hRparamKey = hRparamKey;
    lpReg->hUparamKey = hUparamKey;
    lpReg->hSparamKey = hSparamKey;
    TAILQ_INIT(&lpReg->lSubkeys);

    SetLastError(rc);
    return hRegistry;

cleanup:
    SAFE_CLOSE_KEY(hRparamKey);
    SAFE_CLOSE_KEY(hSparamKey);
    SAFE_CLOSE_KEY(hUparamKey);
    SAFE_CLOSE_KEY(hRootKey);
    SAFE_CLOSE_KEY(hServKey);
    SAFE_CLOSE_KEY(hUserKey);

    SetLastError(rc);
    return NULL;
}

APXHANDLE
apxCreateRegistryA(APXHANDLE hPool, REGSAM samDesired,
                   LPCSTR szRoot,
                   LPCSTR szKeyName,
                   DWORD dwOptions)
{
    WCHAR    wcRoot[SIZ_RESLEN];
    WCHAR    wcKey[SIZ_RESLEN];
    LPWSTR   wsRoot = NULL;
    if (szRoot) {
        AsciiToWide(szRoot, wcRoot);
        wsRoot = wcRoot;
    }
    AsciiToWide(szKeyName, wcKey);
    return apxCreateRegistryW(hPool, samDesired, wsRoot, wcKey, dwOptions);
}

LPSTR
apxRegistryGetStringA(APXHANDLE hRegistry, DWORD dwFrom,
                      LPCSTR szSubkey, LPCSTR szValueName)
{
    LPAPXREGISTRY lpReg;
    HKEY          hKey, hSub = NULL;
    if (IS_INVALID_HANDLE(hRegistry) ||
        hRegistry->dwType != APXHANDLE_TYPE_REGISTRY)
        return NULL;
    lpReg = APXHANDLE_DATA(hRegistry);

    REG_GET_KEY(lpReg, dwFrom, hKey);
    if (!hKey)
        return NULL;
    if (szSubkey) {
        SAFE_CLOSE_KEY(lpReg->hCurrKey);
        if (RegOpenKeyExA(hKey, szSubkey, 0,
                         lpReg->samOptions, &hSub) != ERROR_SUCCESS)
            return NULL;
        lpReg->hCurrKey = hSub;
        hKey = hSub;
    }
    lpReg->pCurrVal = __apxGetRegistrySzA(hRegistry->hPool, hKey, szValueName);

    return lpReg->pCurrVal;
}

LPWSTR
apxRegistryGetStringW(APXHANDLE hRegistry, DWORD dwFrom,
                      LPCWSTR szSubkey, LPCWSTR szValueName)
{
    LPAPXREGISTRY lpReg;
    HKEY          hKey, hSub = NULL;
    if (IS_INVALID_HANDLE(hRegistry) ||
        hRegistry->dwType != APXHANDLE_TYPE_REGISTRY)
        return NULL;
    lpReg = APXHANDLE_DATA(hRegistry);

    REG_GET_KEY(lpReg, dwFrom, hKey);
    if (!hKey)
        return NULL;
    if (szSubkey) {
        SAFE_CLOSE_KEY(lpReg->hCurrKey);
        if (RegOpenKeyExW(hKey, szSubkey, 0,
                         lpReg->samOptions, &hSub) != ERROR_SUCCESS)
            return NULL;
        lpReg->hCurrKey = hSub;
        hKey = hSub;
    }
    lpReg->pCurrVal = __apxGetRegistrySzW(hRegistry->hPool, hKey, szValueName);

    return lpReg->pCurrVal;
}

LPBYTE
apxRegistryGetBinaryA(APXHANDLE hRegistry, DWORD dwFrom,
                      LPCSTR szSubkey, LPCSTR szValueName,
                      LPBYTE lpData, LPDWORD lpdwLength)
{
    LPAPXREGISTRY lpReg;
    HKEY          hKey, hSub = NULL;
    if (IS_INVALID_HANDLE(hRegistry) ||
        hRegistry->dwType != APXHANDLE_TYPE_REGISTRY)
        return NULL;
    lpReg = APXHANDLE_DATA(hRegistry);
    REG_GET_KEY(lpReg, dwFrom, hKey);
    if (!hKey)
        return NULL;
    if (szSubkey) {
        SAFE_CLOSE_KEY(lpReg->hCurrKey);
        if (RegOpenKeyExA(hKey, szSubkey, 0,
                         lpReg->samOptions, &hSub) != ERROR_SUCCESS)
            return NULL;
        lpReg->hCurrKey = hSub;
        hKey = hSub;
    }
    if (lpData && lpdwLength && *lpdwLength) {
        DWORD rc, dwType = REG_BINARY;
        rc = RegQueryValueExA(hKey, szValueName, NULL, &dwType, lpData, lpdwLength);
        if (rc != ERROR_SUCCESS || dwType != REG_BINARY) {
            apxLogWrite(APXLOG_MARK_SYSERR);
            return NULL;
        }
        lpReg->pCurrVal = lpData;
    }
    else {
        lpReg->pCurrVal = __apxGetRegistryBinaryA(hRegistry->hPool, hKey, szValueName, lpdwLength);
    }

    return lpReg->pCurrVal;
}

LPBYTE
apxRegistryGetBinaryW(APXHANDLE hRegistry, DWORD dwFrom,
                      LPCWSTR szSubkey, LPCWSTR szValueName,
                      LPBYTE lpData, LPDWORD lpdwLength)
{
    LPAPXREGISTRY lpReg;
    HKEY          hKey, hSub = NULL;
    if (IS_INVALID_HANDLE(hRegistry) ||
        hRegistry->dwType != APXHANDLE_TYPE_REGISTRY)
        return NULL;
    lpReg = APXHANDLE_DATA(hRegistry);

    REG_GET_KEY(lpReg, dwFrom, hKey);
    if (!hKey)
        return NULL;
    if (szSubkey) {
        SAFE_CLOSE_KEY(lpReg->hCurrKey);
        if (RegOpenKeyExW(hKey, szSubkey, 0,
                          lpReg->samOptions, &hSub) != ERROR_SUCCESS)
            return NULL;
        lpReg->hCurrKey = hSub;
        hKey = hSub;
    }
    if (lpData && lpdwLength && *lpdwLength) {
        DWORD rc, dwType = REG_BINARY;
        rc = RegQueryValueExW(hKey, szValueName, NULL, &dwType, lpData, lpdwLength);
        if (rc != ERROR_SUCCESS || dwType != REG_BINARY) {
            return NULL;
        }
        lpReg->pCurrVal = lpData;
    }
    else {
        lpReg->pCurrVal = __apxGetRegistryBinaryW(hRegistry->hPool, hKey, szValueName, lpdwLength);
    }
    return lpReg->pCurrVal;
}

DWORD
apxRegistryGetNumberW(APXHANDLE hRegistry, DWORD dwFrom,
                      LPCWSTR szSubkey, LPCWSTR szValueName)
{
    LPAPXREGISTRY lpReg;
    HKEY          hKey, hSub = NULL;
    DWORD         dwRval, rl;
    DWORD rc, dwType = REG_DWORD;

    if (IS_INVALID_HANDLE(hRegistry) ||
        hRegistry->dwType != APXHANDLE_TYPE_REGISTRY)
        return 0;
    lpReg = APXHANDLE_DATA(hRegistry);

    REG_GET_KEY(lpReg, dwFrom, hKey);
    if (!hKey)
        return 0;
    if (szSubkey) {
        SAFE_CLOSE_KEY(lpReg->hCurrKey);
        if (RegOpenKeyExW(hKey, szSubkey, 0,
                          lpReg->samOptions, &hSub) != ERROR_SUCCESS)
            return 0;
        lpReg->hCurrKey = hSub;
        hKey = hSub;
    }
    rl = sizeof(DWORD);
    rc = RegQueryValueExW(hKey, szValueName, NULL, &dwType, (LPBYTE)&dwRval, &rl);
    if (rc != ERROR_SUCCESS || dwType != REG_DWORD)
        return 0;
    else
        return dwRval;
}

LPWSTR
apxRegistryGetMzStrW(APXHANDLE hRegistry, DWORD dwFrom,
                     LPCWSTR szSubkey, LPCWSTR szValueName,
                     LPWSTR lpData, LPDWORD lpdwLength)
{
    LPAPXREGISTRY lpReg;
    HKEY          hKey, hSub = NULL;
    if (IS_INVALID_HANDLE(hRegistry) ||
        hRegistry->dwType != APXHANDLE_TYPE_REGISTRY)
        return NULL;
    lpReg = APXHANDLE_DATA(hRegistry);

    REG_GET_KEY(lpReg, dwFrom, hKey);
    if (!hKey)
        return NULL;
    if (szSubkey) {
        SAFE_CLOSE_KEY(lpReg->hCurrKey);
        if (RegOpenKeyExW(hKey, szSubkey, 0,
                          lpReg->samOptions, &hSub) != ERROR_SUCCESS)
            return NULL;
        lpReg->hCurrKey = hSub;
        hKey = hSub;
    }
    if (lpData && lpdwLength && *lpdwLength) {
        DWORD rc, dwType = REG_MULTI_SZ;
        rc = RegQueryValueExW(hKey, szValueName, NULL, &dwType, (BYTE *)lpData, lpdwLength);
        if (rc != ERROR_SUCCESS || dwType != REG_MULTI_SZ) {
            return NULL;
        }
        lpReg->pCurrVal = lpData;
    }
    else {
        lpReg->pCurrVal = __apxGetRegistryBinaryW(hRegistry->hPool, hKey, szValueName, lpdwLength);
        if (lpReg->pCurrVal && lpdwLength)
            *lpdwLength = *lpdwLength * sizeof(WCHAR);
    }
    return lpReg->pCurrVal;
}

BOOL
apxRegistrySetBinaryA(APXHANDLE hRegistry, DWORD dwFrom,
                      LPCSTR szSubkey, LPCSTR szValueName,
                      const LPBYTE lpData, DWORD dwLength)
{
    LPAPXREGISTRY lpReg;
    HKEY          hKey, hSub = NULL;
    DWORD         dwType = REG_BINARY;
    if (IS_INVALID_HANDLE(hRegistry) ||
        hRegistry->dwType != APXHANDLE_TYPE_REGISTRY)
        return FALSE;
    lpReg = APXHANDLE_DATA(hRegistry);

    REG_GET_KEY(lpReg, dwFrom, hKey);
    if (!hKey)
        return FALSE;

    if (szSubkey) {
        SAFE_CLOSE_KEY(lpReg->hCurrKey);
        if (RegCreateKeyExA(hKey, szSubkey, 0,
                            NULL, 0, lpReg->samOptions,
                            NULL, &hSub, NULL) != ERROR_SUCCESS)
            return FALSE;
        lpReg->hCurrKey = hSub;
        hKey = hSub;
    }
    if (RegSetValueExA(hKey, szValueName, 0, dwType,
                      lpData, dwLength) != ERROR_SUCCESS)
        return FALSE;

    return TRUE;
}

BOOL
apxRegistrySetBinaryW(APXHANDLE hRegistry, DWORD dwFrom,
                      LPCWSTR szSubkey, LPCWSTR szValueName,
                      const LPBYTE lpData, DWORD dwLength)
{
    LPAPXREGISTRY lpReg;
    HKEY          hKey, hSub = NULL;
    DWORD         dwType = REG_BINARY;
    if (IS_INVALID_HANDLE(hRegistry) ||
        hRegistry->dwType != APXHANDLE_TYPE_REGISTRY)
        return FALSE;
    lpReg = APXHANDLE_DATA(hRegistry);

    REG_GET_KEY(lpReg, dwFrom, hKey);
    if (!hKey)
        return FALSE;

    if (szSubkey) {
        SAFE_CLOSE_KEY(lpReg->hCurrKey);
        if (RegCreateKeyExW(hKey, szSubkey, 0,
                            NULL, 0, lpReg->samOptions,
                            NULL, &hSub, NULL) != ERROR_SUCCESS)
            return FALSE;
        lpReg->hCurrKey = hSub;
        hKey = hSub;
    }
    if (RegSetValueExW(hKey, szValueName, 0, dwType,
                      lpData, dwLength) != ERROR_SUCCESS)
        return FALSE;

    return TRUE;
}

BOOL
apxRegistrySetMzStrW(APXHANDLE hRegistry, DWORD dwFrom,
                     LPCWSTR szSubkey, LPCWSTR szValueName,
                     LPCWSTR lpData, DWORD dwLength)
{
    LPAPXREGISTRY lpReg;
    HKEY          hKey, hSub = NULL;
    DWORD         dwType = REG_MULTI_SZ;
    if (IS_INVALID_HANDLE(hRegistry) ||
        hRegistry->dwType != APXHANDLE_TYPE_REGISTRY)
        return FALSE;
    lpReg = APXHANDLE_DATA(hRegistry);

    REG_GET_KEY(lpReg, dwFrom, hKey);
    if (!hKey)
        return FALSE;

    if (szSubkey) {
        SAFE_CLOSE_KEY(lpReg->hCurrKey);
        if (RegCreateKeyExW(hKey, szSubkey, 0,
                            NULL, 0, lpReg->samOptions,
                            NULL, &hSub, NULL) != ERROR_SUCCESS)
            return FALSE;
        lpReg->hCurrKey = hSub;
        hKey = hSub;
    }
    if (RegSetValueExW(hKey, szValueName, 0, dwType,
                       (const BYTE *)lpData, dwLength) != ERROR_SUCCESS)
        return FALSE;

    return TRUE;
}

BOOL
apxRegistrySetStrA(APXHANDLE hRegistry, DWORD dwFrom,
                   LPCSTR szSubkey, LPCSTR szValueName,
                   LPCSTR szValue)
{
    LPAPXREGISTRY lpReg;
    HKEY          hKey, hSub = NULL;
    DWORD         dwType = REG_SZ;
    if (IS_INVALID_HANDLE(hRegistry) ||
        hRegistry->dwType != APXHANDLE_TYPE_REGISTRY)
        return FALSE;
    lpReg = APXHANDLE_DATA(hRegistry);

    REG_GET_KEY(lpReg, dwFrom, hKey);
    if (!hKey)
        return FALSE;

    if (szSubkey) {
        SAFE_CLOSE_KEY(lpReg->hCurrKey);
        if (RegCreateKeyExA(hKey, szSubkey, 0,
                            NULL, 0, lpReg->samOptions,
                            NULL, &hSub, NULL) != ERROR_SUCCESS)
            return FALSE;
        lpReg->hCurrKey = hSub;
        hKey = hSub;
    }
    if (!szValue || !lstrlenA(szValue)) {
        if (RegDeleteValueA(hKey, szValueName) != ERROR_SUCCESS)
            return FALSE;
    }
    else if (RegSetValueExA(hKey, szValueName, 0, dwType,
                       (LPBYTE)szValue, lstrlenA(szValue) + 1) != ERROR_SUCCESS)
        return FALSE;

    return TRUE;
}

BOOL
apxRegistrySetStrW(APXHANDLE hRegistry, DWORD dwFrom,
                   LPCWSTR szSubkey, LPCWSTR szValueName,
                   LPCWSTR szValue)
{
    LPAPXREGISTRY lpReg;
    HKEY          hKey, hSub = NULL;
    DWORD         dwType = REG_SZ;
    if (IS_INVALID_HANDLE(hRegistry) ||
        hRegistry->dwType != APXHANDLE_TYPE_REGISTRY)
        return FALSE;
    lpReg = APXHANDLE_DATA(hRegistry);

    REG_GET_KEY(lpReg, dwFrom, hKey);
    if (!hKey)
        return FALSE;

    if (szSubkey) {
        SAFE_CLOSE_KEY(lpReg->hCurrKey);
        if (RegCreateKeyExW(hKey, szSubkey, 0,
                            NULL, 0, lpReg->samOptions,
                            NULL, &hSub, NULL) != ERROR_SUCCESS)
            return FALSE;
        lpReg->hCurrKey = hSub;
        hKey = hSub;
    }
    if (!szValue || !lstrlenW(szValue)) {
        if (RegDeleteValueW(hKey, szValueName) != ERROR_SUCCESS)
            return FALSE;
    }
    else if (RegSetValueExW(hKey, szValueName, 0, dwType,
                       (LPBYTE)szValue,
                       (lstrlenW(szValue) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS)
        return FALSE;

    return TRUE;
}

BOOL
apxRegistrySetNumW(APXHANDLE hRegistry, DWORD dwFrom,
                   LPCWSTR szSubkey, LPCWSTR szValueName,
                   DWORD dwValue)
{
    LPAPXREGISTRY lpReg;
    HKEY          hKey, hSub = NULL;
    DWORD         dwType = REG_DWORD;
    if (IS_INVALID_HANDLE(hRegistry) ||
        hRegistry->dwType != APXHANDLE_TYPE_REGISTRY)
        return FALSE;
    lpReg = APXHANDLE_DATA(hRegistry);

    REG_GET_KEY(lpReg, dwFrom, hKey);
    if (!hKey)
        return FALSE;

    if (szSubkey) {
        SAFE_CLOSE_KEY(lpReg->hCurrKey);
        if (RegCreateKeyExW(hKey, szSubkey, 0,
                            NULL, 0, lpReg->samOptions,
                            NULL, &hSub, NULL) != ERROR_SUCCESS)
            return FALSE;
        lpReg->hCurrKey = hSub;
        hKey = hSub;
    }
    if (RegSetValueExW(hKey, szValueName, 0, dwType,
                       (LPBYTE)&dwValue,
                       sizeof(DWORD)) != ERROR_SUCCESS)
        return FALSE;

    return TRUE;
}


BOOL
apxRegistryDeleteW(APXHANDLE hRegistry, DWORD dwFrom,
                   LPCWSTR szSubkey, LPCWSTR szValueName)
{
    LPAPXREGISTRY lpReg;
    HKEY          hKey, hSub = NULL;
    DWORD         dwType = REG_SZ;
    if (IS_INVALID_HANDLE(hRegistry) ||
        hRegistry->dwType != APXHANDLE_TYPE_REGISTRY)
        return FALSE;
    lpReg = APXHANDLE_DATA(hRegistry);

    REG_GET_KEY(lpReg, dwFrom, hKey);
    if (!hKey)
        return FALSE;

    if (szSubkey) {
        SAFE_CLOSE_KEY(lpReg->hCurrKey);
        if (RegOpenKeyExW(hKey, szSubkey, 0,
                          lpReg->samOptions, &hSub) != ERROR_SUCCESS)
            return FALSE;
        lpReg->hCurrKey = hSub;
        hKey = hSub;
    }
    if (RegDeleteValueW(hKey, szValueName) != ERROR_SUCCESS)
        return FALSE;

    return TRUE;
}


LONG apxDeleteRegistryRecursive(HKEY hKeyRoot, LPCWSTR szSubKey) {
    LONG rc = ERROR_SUCCESS;
    DWORD dwSize = 0;
    WCHAR szName[SIZ_BUFLEN];
    HKEY hKey = NULL;

    if (ERROR_SUCCESS == RegDeleteKeyW(hKeyRoot, szSubKey)) {
        return ERROR_SUCCESS;
    }

    rc = RegOpenKeyExW(hKeyRoot, szSubKey, 0, KEY_ENUMERATE_SUB_KEYS | DELETE, &hKey);
    if (rc != ERROR_SUCCESS) {
        if (rc == ERROR_FILE_NOT_FOUND) {
            return ERROR_SUCCESS;
        } else {
            return rc;
        }
    }
    while (rc == ERROR_SUCCESS) {
        dwSize = SIZ_BUFLEN;
        rc = RegEnumKeyExW(hKey, 0, szName, &dwSize, NULL, NULL, NULL, NULL );

        if (rc == ERROR_NO_MORE_ITEMS) {
            rc = RegDeleteKeyW(hKeyRoot, szSubKey);
            break;
        } else {
            rc = apxDeleteRegistryRecursive(hKey, szName);
            if (rc != ERROR_SUCCESS) {
                break;   // abort when we start failing
            }
        }
    }
    RegCloseKey(hKey);
    return rc;
}


BOOL
apxDeleteRegistryW(LPCWSTR szRoot,
                   LPCWSTR szKeyName,
                   REGSAM samDesired,
                   BOOL bDeleteEmptyRoot)
{
    WCHAR     buff[SIZ_BUFLEN];
    LONG      rc = ERROR_SUCCESS;
    HKEY      hKey = NULL;
    BOOL      rv = TRUE;
    HKEY      hives[] = {HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER, NULL}, *hive = NULL;

    if (!szKeyName || lstrlenW(szKeyName) > SIZ_RESMAX)
        return FALSE;
    if (szRoot && lstrlenW(szRoot) > SIZ_RESMAX)
        return FALSE;

    if (szRoot)
        lstrcpyW(buff, szRoot);
    else
        lstrcpyW(buff, REGAPACHE_ROOT);
    lstrcatW(buff, REGSEPARATOR);

    for (hive = &hives[0]; *hive; hive++) {
        HKEY hkeySoftware = NULL;

        rc = RegOpenKeyExW(*hive, REGSOFTWARE_ROOT, 0, KEY_READ | samDesired, &hkeySoftware);
        if (rc != ERROR_SUCCESS) {
            rv = FALSE;
        } else {
            rc = RegOpenKeyExW(hkeySoftware, buff, 0, samDesired | KEY_ENUMERATE_SUB_KEYS | DELETE, &hKey);
            if (rc == ERROR_SUCCESS) {
                rc = apxDeleteRegistryRecursive(hKey, szKeyName);
                RegCloseKey(hKey);
                hKey = NULL;
                rv |= (rc == ERROR_SUCCESS);
            }
            if (bDeleteEmptyRoot) {
                // will fail if there are subkeys, just like we want
                RegDeleteKeyW(hkeySoftware, buff);
            }
            RegCloseKey(hkeySoftware);
        }
    }
    return rv;
}

BOOL
apxDeleteRegistryA(LPCSTR szRoot,
                   LPCSTR szKeyName,
                   REGSAM samDesired,
                   BOOL bDeleteEmptyRoot)
{
    WCHAR    wcRoot[SIZ_RESLEN];
    WCHAR    wcKey[SIZ_RESLEN];
    LPWSTR   wsRoot = NULL;
    if (szRoot) {
        AsciiToWide(szRoot, wcRoot);
        wsRoot = wcRoot;
    }
    AsciiToWide(szKeyName, wcKey);

    return apxDeleteRegistryW(wsRoot, wcKey, samDesired, bDeleteEmptyRoot);
}


LPWSTR apxGetJavaSoftHome(APXHANDLE hPool, BOOL bPreferJre)
{
    LPWSTR  wsJhome, off;
    DWORD   err, dwLen;
    HKEY    hKey;
    WCHAR   wsBuf[SIZ_BUFLEN];
    WCHAR   wsKey[SIZ_RESLEN];
#if 1 /* XXX: Add that customizable using function call arg */
    if (!bPreferJre && (wsJhome = __apxGetEnvironmentVariableW(hPool, JAVA_HOME)))
        return wsJhome;
#endif
    lstrcpyW(wsKey, JAVA_CURRENT);
    if (bPreferJre)
        lstrcpyW(wsBuf, JRE_REGKEY);
    else
        lstrcpyW(wsBuf, JDK_REGKEY);
    dwLen = lstrlenW(wsBuf);
    off = &wsBuf[dwLen];
    dwLen = SIZ_RESMAX;
    if ((err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, wsBuf,
                             0, KEY_READ, &hKey)) != ERROR_SUCCESS) {
        return NULL;
    }
    if ((err = RegQueryValueExW(hKey, JAVA_CURRENT, NULL, NULL,
                                (LPBYTE)off,
                                &dwLen)) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return NULL;
    }
    RegCloseKey(hKey);
    if ((err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, wsBuf,
                             0, KEY_READ, &hKey)) != ERROR_SUCCESS) {
        return NULL;
    }
    wsJhome = __apxGetRegistrySzW(hPool, hKey, JAVAHOME);
    if (wsJhome)
        SetEnvironmentVariableW(JAVA_HOME, wsJhome);
    RegCloseKey(hKey);

    return wsJhome;
}

LPWSTR apxGetJavaSoftRuntimeLib(APXHANDLE hPool)
{
    LPWSTR  wsRtlib, off;
    DWORD   err, dwLen = SIZ_RESLEN;
    HKEY    hKey;
    WCHAR   wsBuf[SIZ_BUFLEN];

    lstrcpyW(wsBuf, JRE_REGKEY);

    dwLen = lstrlenW(wsBuf);
    off = &wsBuf[dwLen];
    dwLen = SIZ_RESLEN;
    if ((err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, wsBuf,
                             0, KEY_READ, &hKey)) != ERROR_SUCCESS) {
        return NULL;
    }
    if ((err = RegQueryValueExW(hKey, JAVA_CURRENT, NULL, NULL,
                                (LPBYTE)off,
                                &dwLen)) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return NULL;
    }
    RegCloseKey(hKey);
    if ((err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, wsBuf,
                             0, KEY_READ, &hKey)) != ERROR_SUCCESS) {
        return NULL;
    }
    wsRtlib = __apxGetRegistrySzW(hPool, hKey, JAVA_RUNTIME);
    RegCloseKey(hKey);

    return wsRtlib;
}

/* Service Registry helper functions */

BOOL apxRegistryEnumServices(LPAPXREGENUM lpEnum, LPAPXSERVENTRY lpEntry)
{
    DWORD rc, dwLength = SIZ_RESLEN;

    if (IS_INVALID_HANDLE(lpEnum->hServicesKey)) {
        rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, REGSERVICE_ROOT, 0,
                       KEY_READ, &(lpEnum->hServicesKey));
        if (rc != ERROR_SUCCESS) {
            return FALSE;
        }
        rc = RegQueryInfoKeyW(lpEnum->hServicesKey,
                              NULL,
                              NULL,
                              NULL,
                              &lpEnum->cSubKeys,
                              &lpEnum->cbMaxSubKey,
                              &lpEnum->cchMaxClass,
                              &lpEnum->cValues,
                              &lpEnum->cchMaxValue,
                              &lpEnum->cbMaxValueData,
                              NULL,
                              NULL);
        /* TODO: add dynamic maxsubkey length */
        if (rc != ERROR_SUCCESS || lpEnum->cbMaxSubKey > SIZ_RESLEN) {
            SAFE_CLOSE_KEY(lpEnum->hServicesKey);
            return FALSE;
        }
    }
    if (lpEnum->dwIndex >= lpEnum->cSubKeys) {
        SAFE_CLOSE_KEY(lpEnum->hServicesKey);
        return FALSE;
    }
    rc = RegEnumKeyExW(lpEnum->hServicesKey,
                       lpEnum->dwIndex++,
                       lpEntry->szServiceName,
                       &dwLength,
                       NULL,
                       NULL,
                       NULL,
                       NULL);
    if (rc != ERROR_SUCCESS) {
        SAFE_CLOSE_KEY(lpEnum->hServicesKey);
        return FALSE;
    }
    else {
        HKEY hKey;
        rc = RegOpenKeyExW(lpEnum->hServicesKey, lpEntry->szServiceName,
                           0, KEY_READ, &hKey);
        if (rc != ERROR_SUCCESS) {
            SAFE_CLOSE_KEY(lpEnum->hServicesKey);
            return FALSE;
        }
        __apxGetRegistryStrW(NULL, hKey, REGDESCRIPTION,
                            lpEntry->szServiceDescription, SIZ_DESLEN);
        __apxGetRegistryStrW(NULL, hKey, REGSERVICE_USER,
                            lpEntry->szObjectName, SIZ_RESLEN);
        lpEntry->dwStart = __apxGetRegistryDwordW(NULL, hKey, REGSERVICE_START);
        RegCloseKey(hKey);

    }
    return TRUE;
}

BOOL apxGetServiceDescriptionW(LPCWSTR szServiceName, LPWSTR szDescription,
                               DWORD dwDescriptionLength)
{
    HKEY  hKey;
    WCHAR wcName[SIZ_RESLEN];
    DWORD rc, l = dwDescriptionLength * sizeof(WCHAR);
    DWORD t = REG_SZ;
    if (lstrlenW(szServiceName) > SIZ_RESMAX)
        return FALSE;
    lstrcpyW(wcName, REGSERVICE_ROOT);
    lstrcatW(wcName, szServiceName);

    rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, wcName, 0, KEY_READ, &hKey);
    if (rc != ERROR_SUCCESS) {
        return FALSE;
    }
    rc = RegQueryValueExW(hKey, REGDESCRIPTION, NULL, &t, (BYTE *)szDescription,
                          &l);
    SAFE_CLOSE_KEY(hKey);
    if (rc == ERROR_SUCCESS && t == REG_SZ)
        return TRUE;
    else
        return FALSE;
}

BOOL apxGetServiceUserW(LPCWSTR szServiceName, LPWSTR szUser,
                        DWORD dwUserLength)
{
    HKEY  hKey;
    WCHAR wcName[SIZ_RESLEN];
    DWORD rc, l = dwUserLength * sizeof(WCHAR);
    DWORD t = REG_SZ;
    if (lstrlenW(szServiceName) > SIZ_RESMAX)
        return FALSE;
    lstrcpyW(wcName, REGSERVICE_ROOT);
    lstrcatW(wcName, szServiceName);

    rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, wcName, 0, KEY_READ, &hKey);
    if (rc != ERROR_SUCCESS) {
        return FALSE;
    }
    rc = RegQueryValueExW(hKey, REGSERVICE_USER, NULL, &t, (BYTE *)szUser,
                          &l);
    SAFE_CLOSE_KEY(hKey);
    if (rc == ERROR_SUCCESS && t == REG_SZ)
        return TRUE;
    else
        return FALSE;
}

DWORD apxGetMaxServiceTimeout(APXHANDLE hPool)
{
    DWORD   maxTimeout  = 20000;
    LPWSTR  wsMaxTimeout;
    DWORD   err;
    HKEY    hKey;
    WCHAR   wsBuf[SIZ_BUFLEN];

    lstrcpyW(wsBuf, CONTROL_REGKEY);

    if ((err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, wsBuf,
                             0, KEY_READ, &hKey)) != ERROR_SUCCESS) {
        return maxTimeout;
    }
    wsMaxTimeout = __apxGetRegistrySzW(hPool, hKey, REGTIMEOUT);
    RegCloseKey(hKey);

    if (wsMaxTimeout[0]) {
        maxTimeout = (DWORD)apxAtoulW(wsMaxTimeout);
    }
    apxFree(wsMaxTimeout);

    return maxTimeout;
}
