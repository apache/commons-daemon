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
 
#ifndef _HANDLES_H_INCLUDED_
#define _HANDLES_H_INCLUDED_

__APXBEGIN_DECLS

#define SAFE_CLOSE_HANDLE(h)                            \
    if ((h) != NULL && (h) != INVALID_HANDLE_VALUE) {   \
        CloseHandle((h));                               \
        (h) = NULL;                                     \
    }

typedef struct stAPXHANDLE  stAPXHANDLE;
typedef stAPXHANDLE*        APXHANDLE;

/**
 * Alignment macros
 */

/* APR_ALIGN() is only to be used to align on a power of 2 boundary */
#define APX_ALIGN(size, boundary) \
    (((size) + ((boundary) - 1)) & ~((boundary) - 1))

/** Default alignment */
#define APX_ALIGN_DEFAULT(size) APX_ALIGN(size, 16)

 
/** Handle callback function prototype */
typedef BOOL (*LPAPXFNCALLBACK)(APXHANDLE hObject, UINT uMsg,
                                WPARAM wParam, LPARAM lParam);

#if _MSC_VER >= 1300
#define APXHANDLE_INVALID               ((void *)0xdeadbeefLL)
#else
#define APXHANDLE_INVALID               ((void *)0xdeadbeefL)
#endif

#define APXHANDLE_HOOK_FIRST            0
#define APXHANDLE_HOOK_LAST             1

/** Flags */
/** handle has its own heap */
#define APXHANDLE_HAS_HEAP              0x00000001
/** handle has CriticalSection */
#define APXHANDLE_HAS_LOCK              0x00000002
/** handle owns the CriticalSection */
#define APXHANDLE_OWNS_LOCK             0x00000006
/** handle has EventThread */
#define APXHANDLE_HAS_EVENT             0x00000010
/** handle has UserData */
#define APXHANDLE_HAS_USERDATA          0x00000020

/** Types */
#define APXHANDLE_TYPE_INVALID          0xdeadbeef
#define APXHANDLE_TYPE_POOL             0x01000000
#define APXHANDLE_TYPE_WINHANDLE        0x02000000
#define APXHANDLE_TYPE_SERVICE          0x03000000
#define APXHANDLE_TYPE_LPTR             0x04000000
#define APXHANDLE_TYPE_CONSOLE          0x05000000
#define APXHANDLE_TYPE_PROCESS          0x06000000
#define APXHANDLE_TYPE_JVM              0x07000000
#define APXHANDLE_TYPE_REGISTRY         0x08000000

/** Initialize the Handle manager
 *  reference counted
 */
BOOL        apxHandleManagerInitialize();
/** Destroys the Handle manager
 *  reference counted
 */
BOOL        apxHandleManagerDestroy();
/** Create the memory pool
 * param: hParent   parent pool or NULL to use the system pool
 *        dwOptions OR'd flags: APXHANDLE_HAS_HEAP,
 *                              APXHANDLE_HAS_LOCK
 *                              APXHANDLE_OWNS_LOCK
 */                 
APXHANDLE   apxPoolCreate(APXHANDLE hParent, DWORD dwOptions);
/** Create the memory pool
 * param: hPpool    pool to allocate from or NULL for system pool
 *        dwOptions OR'd flags: see APXHANDLE_TYPE_ and APXHANDLE_HAS_
 *                              values
 *        lpData     user supplied Data
 *        dwDataSize extra pool user data size, combined with options
 *                   the lpData is copied to the internal storage;   
 *        fnCallback Optional handle callback function
 */                 
APXHANDLE   apxHandleCreate(APXHANDLE hPool, DWORD dwOptions,
                            LPVOID lpData, DWORD  dwDataSize,
                            LPAPXFNCALLBACK fnCallback);
/** Close the handle
 *  Calls the callback function and frees the memory
 */
BOOL        apxCloseHandle(APXHANDLE hObject);
/** Get The internal user data
 */
LPVOID      apxHandleGetUserData(APXHANDLE hObject);
/** Set The internal user data
 *  params:
 *        lpData     user supplied Data
 *        dwDataSize user data size, combined with create options
 *                   the lpData is either copied to the internal storage
 *                   or assigned.
 */
LPVOID      apxHandleSetUserData(APXHANDLE hObject, LPVOID lpData,
                                 DWORD dwDataSize);
/** Send the message to the handle 
 *  Callback function is executed with WM_COMMAND uMsg
 */
BOOL        apxHandleSendMessage(APXHANDLE hObject, UINT uMsg,
                                 WPARAM wParam, LPARAM lParam);

/** Post the message to the handle 
 *  function returns imediately.
 */
BOOL        apxHandlePostMessage(APXHANDLE hObject, UINT uMsg,
                                 WPARAM wParam, LPARAM lParam);
/** Lock or unlock the handle
 * If bLock is true lock the handle, otherwise unlock.
 */
BOOL        apxHandleLock(APXHANDLE hObject, BOOL bLock);

/** Add the callback to the handles hook chain
 * 
 */
BOOL        apxHandleAddHook(APXHANDLE hObject, DWORD dwWhere,
                             LPAPXFNCALLBACK fnCallback);

DWORD       apxHandleWait(APXHANDLE hHandle, DWORD dwMilliseconds,
                          BOOL bKill);

/** General pool memory allocation functions
 */
LPVOID      apxPoolAlloc(APXHANDLE hPool, DWORD dwSize);
LPVOID      apxPoolCalloc(APXHANDLE hPool, DWORD dwSize);
LPVOID      apxPoolRealloc(APXHANDLE hPool, LPVOID lpMem, DWORD dwNewSize);
LPTSTR      apxPoolStrdup(APXHANDLE hPool, LPCTSTR szSource);

/** General system pool memory allocation functions
 */

LPVOID      apxAlloc(DWORD dwSize);
LPVOID      apxCalloc(DWORD dwSize);
LPVOID      apxRealloc(LPVOID lpMem, DWORD dwNewSize);

LPSTR       apxStrdupA(LPCSTR szSource);
LPWSTR      apxStrdupW(LPCWSTR szSource);
LPSTR       apxPoolStrdupA(APXHANDLE hPool, LPCSTR szSource);
LPWSTR      apxPoolStrdupW(APXHANDLE hPool, LPCWSTR szSource);

LPWSTR      apxPoolWStrdupA(APXHANDLE hPool, LPCSTR szSource);

#define     apxPoolWStrdupW apxPoolStrdupW

#ifdef _UNICODE
#define apxStrdup       apxStrdupW
#define apxPoolStrdup   apxPoolStrdupW
#else
#define apxStrdup       apxStrdupA
#define apxPoolStrdup   apxPoolStrdupW
#endif

#ifndef _UNICODE
#define     apxPoolWStrdup  apxPoolWStrdupA
#define     apxWStrdup      apxWStrdupA
#else
#define     apxPoolWStrdup  apxPoolStrdupW
#define     apxWStrdup      apxStrdupW
#endif
/** Free the allocated memory
 * It will call te correct pool if the address is valid
 */
VOID        apxFree(LPVOID lpMem);

__APXEND_DECLS

#endif /* _HANDLES_H_INCLUDED_ */
