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

#define ALLOCBLOCK_INVALID      0xdeadbeef

typedef struct APXPOOL      APXPOOL;
typedef APXPOOL*            LPAPXPOOL;

typedef struct ALLOCBLOCK {
    DWORD       dwSize;
    APXHANDLE   lpPool;
    APXMEMWORD  lpAlign;
} ALLOCBLOCK, *LPALLOCBLOCK;

struct APXPOOL {
    TAILQ_HEAD(_lHandles, stAPXHANDLE) lHandles;
    TAILQ_HEAD(_lPools, stAPXHANDLE)   lPools;
};

static SYSTEM_INFO      _st_sys_info;
static APXHANDLE        _st_sys_pool  = NULL;
static int              _st_sys_init  = 0;
static LPVOID           _st_sys_page  = NULL;
LPWSTR                  *_st_sys_argvw = NULL;
int                     _st_sys_argc  = 0;

#ifdef _DEBUG
static INT  _heap_count = 0;
static INT  _heap_alloc_count = 0;
static INT  _heap_realloc_count = 0;

HANDLE HeapCREATE(DWORD flOptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize)
{
    _heap_count++;
    return HeapCreate(flOptions, dwInitialSize, dwMaximumSize);
}

BOOL HeapDESTROY(HANDLE hHeap)
{
    _heap_count--;
    return HeapDestroy(hHeap);
}


LPVOID HeapALLOC(HANDLE hHeap, DWORD dwFlags, SIZE_T nSize)
{
    _heap_alloc_count++;
    return HeapAlloc(hHeap, dwFlags, nSize);
}

BOOL HeapFREE(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem)
{
    _heap_alloc_count--;
    return HeapFree(hHeap, dwFlags, lpMem);
}

LPVOID HeapREALLOC(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes)
{

    _heap_realloc_count++;
    return HeapReAlloc(hHeap, dwFlags, lpMem, dwBytes);
}
#endif

static LPVOID __apxPoolAllocCore(APXHANDLE hPool, 
                                 DWORD dwSize, DWORD dwOptions)
{
    DWORD dwPhysicalSize;
    LPALLOCBLOCK lpBlock;
    
    if (!hPool)
        hPool = _st_sys_pool;
    dwPhysicalSize = APX_ALIGN_DEFAULT(dwSize + sizeof(ALLOCBLOCK));
    lpBlock = HeapALLOC(hPool->hHeap, dwOptions, dwPhysicalSize);
    lpBlock->dwSize = dwPhysicalSize;
    lpBlock->lpPool = hPool;

    return ((char *)lpBlock + sizeof(ALLOCBLOCK));
}

static LPVOID __apxPoolReallocCore(APXHANDLE hPool, LPVOID lpMem,
                                   DWORD dwSize, DWORD dwOptions)
{
    DWORD dwPhysicalSize;
    LPALLOCBLOCK lpBlock;
    LPALLOCBLOCK lpOrg;
    
    if (!lpMem)
        return __apxPoolAllocCore(hPool, dwSize, dwOptions);
    lpOrg = (LPALLOCBLOCK)((char *)lpMem - sizeof(ALLOCBLOCK));
    if (!hPool)
        hPool = _st_sys_pool;
    /* Trying to realloc something that isn't valid */
    if (lpOrg->lpPool == APXHANDLE_INVALID ||
        lpOrg->lpPool != hPool)
        return NULL;
    dwPhysicalSize = APX_ALIGN_DEFAULT(dwSize + sizeof(ALLOCBLOCK));
    lpBlock = HeapREALLOC(hPool->hHeap, dwOptions, lpOrg, dwPhysicalSize);
    lpBlock->dwSize = dwPhysicalSize;
    lpBlock->lpPool = hPool;

    return ((char *)lpBlock + sizeof(ALLOCBLOCK));
}

static void __apxPoolFreeCore(LPVOID lpMem)
{
    APXHANDLE hPool;
    LPALLOCBLOCK lpBlock = (LPALLOCBLOCK)((char *)lpMem - sizeof(ALLOCBLOCK));
    
    if (lpBlock->lpPool != APXHANDLE_INVALID) {
        hPool = lpBlock->lpPool;
        lpBlock->lpPool = APXHANDLE_INVALID;
    }
    else
        return;
    HeapFREE(hPool->hHeap, 0, lpBlock);
}
/*
 *
 */
static DWORD WINAPI __apxHandleEventThread(LPVOID lpParameter)
{
    APXHANDLE hHandle = (APXHANDLE)lpParameter;
    DWORD rv = 0;
    while (hHandle->dwType != APXHANDLE_TYPE_INVALID) {
        DWORD dwState;
        dwState = WaitForSingleObject(hHandle->hEventHandle, INFINITE);
        /* the flags can be changed to invalid meaning we are killing
         * this event.
         */
        if (dwState == WAIT_OBJECT_0 &&
            hHandle->dwType != APXHANDLE_TYPE_INVALID) {
            if (hHandle->uMsg && (hHandle->wParam || hHandle->lParam)) {
                APXCALLHOOK *lpCall;
                rv = (*hHandle->fnCallback)(hHandle, hHandle->uMsg,
                                            hHandle->wParam, hHandle->lParam);
                TAILQ_FOREACH(lpCall, &hHandle->lCallbacks, queue) {
                    (*lpCall->fnCallback)(hHandle, hHandle->uMsg,
                                          hHandle->wParam, hHandle->lParam);
                }
                hHandle->uMsg = 0;
                if (!rv)
                    break;
            }
            ResetEvent(hHandle->hEventHandle);
            SwitchToThread();
        }
        else
            break;
    }
    ResetEvent(hHandle->hEventHandle);
    /* This will rise the Thread waiting function */
    return 0;
}

static BOOL __apxPoolCallback(APXHANDLE hObject, UINT uMsg,
                              WPARAM wParam, LPARAM lParam)
{
    LPAPXPOOL  lpPool;
    APXHANDLE   hCur;
    if (hObject->dwType != APXHANDLE_TYPE_POOL)
        return FALSE;
    lpPool = APXHANDLE_DATA(hObject);

    /* recurse the subpools */
    TAILQ_FOREACH(hCur, &lpPool->lPools, queue) {
        __apxPoolCallback(hCur, uMsg, 0, 0);
    }
    /* call the handles callback */        
    for(hCur = TAILQ_FIRST(&lpPool->lHandles) ;
        hCur != NULL ;
        hCur = TAILQ_FIRST(&lpPool->lHandles)) {
        apxCloseHandle(hCur);
    }
    /* if we are closing this pool destroy the private Heap */
    if (uMsg == WM_CLOSE) {
        if (hObject->dwFlags & APXHANDLE_HAS_HEAP)
            HeapDESTROY(hObject->hHeap);
        hObject->dwSize = 0;
    }
    else if (uMsg == WM_CLEAR)
        hObject->dwSize = 0;

    return TRUE;
}

static BOOL __apxHandleCallback(APXHANDLE hObject, UINT uMsg,
                                WPARAM wParam, LPARAM lParam)
{
    BOOL rv = FALSE;
    if (hObject->dwType == APXHANDLE_TYPE_INVALID)
        return FALSE;
    /* Default handler handles only close event */
    if (uMsg != WM_CLOSE)
        return FALSE;
    if (hObject->dwType == APXHANDLE_TYPE_WINHANDLE && 
        !(IS_INVALID_HANDLE(hObject->uData.hWinHandle))) {
        rv = CloseHandle(hObject->uData.hWinHandle);
        hObject->uData.hWinHandle = NULL;
    }
    /* Invalidate the handle */
    hObject->dwType = APXHANDLE_TYPE_INVALID;

    if (hObject->dwFlags & APXHANDLE_HAS_EVENT) {
        DWORD dwState;
        /* Signal the EventThread to exit */
        SetEvent(hObject->hEventHandle);
        /* Wait for EventThread to Exit */
        dwState = WaitForSingleObject(hObject->hEventThread, 1000);
        SAFE_CLOSE_HANDLE(hObject->hEventHandle);
        if (dwState == WAIT_TIMEOUT)
            TerminateThread(hObject->hEventThread, 0);
        SAFE_CLOSE_HANDLE(hObject->hEventThread);
        /* Reset the evant flag */
        hObject->dwFlags &= ~APXHANDLE_HAS_EVENT;
    }
    return rv;
}

static BOOL __apxCreateSystemPool()
{
    LPAPXPOOL lpPool;
    HANDLE    hHeap;

    GetSystemInfo(&_st_sys_info);
    apxGetOsLevel();
    /* First create the shared data segment */
    _st_sys_page = VirtualAlloc(NULL, _st_sys_info.dwAllocationGranularity,
                                MEM_RESERVE, PAGE_NOACCESS);
    if (!_st_sys_page)
        return FALSE;
    _st_sys_page = VirtualAlloc(_st_sys_page, _st_sys_info.dwAllocationGranularity,
                                MEM_COMMIT, PAGE_READWRITE);     

    /* Create the main Heap */
    hHeap = HeapCREATE(0, _st_sys_info.dwAllocationGranularity, 0);
    _st_sys_pool = HeapALLOC(hHeap, HEAP_ZERO_MEMORY, 
                             APX_ALIGN_DEFAULT(APXHANDLE_SZ + sizeof(APXPOOL)));
    _st_sys_pool->hHeap = hHeap;
    _st_sys_pool->dwType = APXHANDLE_TYPE_INVALID;
    if (IS_INVALID_HANDLE(_st_sys_pool->hHeap))
        return FALSE;
    _st_sys_pool->fnCallback = __apxPoolCallback;
    lpPool = APXHANDLE_DATA(_st_sys_pool);
    /* Initialize the pool and object lists */
    TAILQ_INIT(&lpPool->lHandles);
    TAILQ_INIT(&lpPool->lPools);
    _st_sys_pool->dwType  = APXHANDLE_TYPE_POOL;
    
    /** TODO: For each unsupported function make a surrogate */
    _st_sys_argvw = CommandLineToArgvW(GetCommandLineW(), &_st_sys_argc);

    return TRUE;
}

BOOL
apxHandleManagerInitialize()
{
    BOOL rv;
    if (_st_sys_init++)
        return TRUE;
    rv = __apxCreateSystemPool();

    return rv;
}

BOOL
apxHandleManagerDestroy()
{
    HANDLE hHeap;

    if (--_st_sys_init == 0) {
        hHeap = _st_sys_pool->hHeap;
        apxCloseHandle(_st_sys_pool);
        /* Destroy the main Heap */
        HeapDESTROY(hHeap);
        _st_sys_pool = NULL;
        VirtualFree(_st_sys_page, 0, MEM_RELEASE);
        GlobalFree(_st_sys_argvw);
        _st_sys_argvw = NULL;
        _st_sys_argc  = 0;
#ifdef _DEBUG
        apxLogWrite(APXLOG_MARK_DEBUG "Alloc   Count %d", _heap_alloc_count);
        apxLogWrite(APXLOG_MARK_DEBUG "Realloc Count %d", _heap_realloc_count);
        apxLogWrite(APXLOG_MARK_DEBUG "Heap    Count %d", _heap_count);
#endif
        return TRUE;
    }
    
    return FALSE;
}

APXHANDLE
apxPoolCreate(APXHANDLE hParent, DWORD dwOptions)
{
    APXHANDLE   hHandle; 
    LPAPXPOOL   lpPool;
    HANDLE      hHeap;

    if (IS_INVALID_HANDLE(hParent))
        hParent = _st_sys_pool;
    if (hParent->dwType != APXHANDLE_TYPE_POOL) {
        apxLogWrite(APXLOG_MARK_ERROR "Parent Handle type is not POOL %d",
                    hParent->dwType);
        return INVALID_HANDLE_VALUE;
    }
    /* Allocate the handle from the parent */
    hHandle = __apxPoolAllocCore(hParent, APXHANDLE_SZ + sizeof(APXPOOL),
                                 HEAP_ZERO_MEMORY);

    if (dwOptions & APXHANDLE_HAS_HEAP) {
        /* Create the private Heap */
        hHeap = HeapCREATE(0, _st_sys_info.dwAllocationGranularity, 0);
        hHandle->dwFlags |= APXHANDLE_HAS_HEAP;
    }
    else
        hHeap = hParent->hHeap;
    hHandle->hHeap = hHeap;
    hHandle->dwType = APXHANDLE_TYPE_POOL;
    hHandle->hPool  = hParent;
    hHandle->fnCallback = __apxPoolCallback;
    lpPool = APXHANDLE_DATA(hHandle);
    TAILQ_INIT(&lpPool->lHandles);
    TAILQ_INIT(&lpPool->lPools);

    /* Insert the pool to the head of parent pool */
    lpPool = APXHANDLE_DATA(hParent);
    APXHANDLE_SPINLOCK(hParent);
    TAILQ_INSERT_HEAD(&lpPool->lPools, hHandle, queue);
    ++hParent->dwSize;
    APXHANDLE_SPINUNLOCK(hParent);

    return hHandle;
}

LPVOID
apxPoolAlloc(APXHANDLE hPool, DWORD dwSize)
{
    if (IS_INVALID_HANDLE(hPool) ||
        (hPool->dwType != APXHANDLE_TYPE_POOL))
        hPool = _st_sys_pool;
    return __apxPoolAllocCore(hPool, dwSize, 0);
}

LPVOID
apxPoolCalloc(APXHANDLE hPool, DWORD dwSize)
{
    if (IS_INVALID_HANDLE(hPool) ||
        (hPool->dwType != APXHANDLE_TYPE_POOL))
        hPool = _st_sys_pool;
    return __apxPoolAllocCore(hPool, dwSize, HEAP_ZERO_MEMORY);
}

LPVOID
apxPoolRealloc(APXHANDLE hPool, LPVOID lpMem, DWORD dwNewSize)
{
    if (IS_INVALID_HANDLE(hPool) ||
        (hPool->dwType != APXHANDLE_TYPE_POOL))
        hPool = _st_sys_pool;
    return __apxPoolReallocCore(hPool, lpMem, dwNewSize, HEAP_ZERO_MEMORY);
}

LPVOID
apxAlloc(DWORD dwSize)
{
    return __apxPoolAllocCore(_st_sys_pool, dwSize, 0);
}

LPVOID
apxCalloc(DWORD dwSize)
{
    return __apxPoolAllocCore(_st_sys_pool, dwSize, HEAP_ZERO_MEMORY);
}

LPVOID
apxRealloc(LPVOID lpMem, DWORD dwNewSize)
{
    return __apxPoolReallocCore(_st_sys_pool, lpMem, dwNewSize, HEAP_ZERO_MEMORY);
}

VOID
apxFree(LPVOID lpMem)
{
    if (lpMem)
        __apxPoolFreeCore(lpMem);

}

LPWSTR
apxPoolWStrdupA(APXHANDLE hPool, LPCSTR szSource)
{
    if (szSource) {
        LPWSTR szDest;
        int cch = MultiByteToWideChar(CP_UTF8, 0, szSource, -1, NULL, 0);
        szDest = (LPWSTR)apxPoolAlloc(hPool, cch * sizeof(WCHAR));
        if (!MultiByteToWideChar(CP_UTF8, 0, szSource, -1, szDest, cch)) {
            apxFree(szDest);
            return NULL;
        }
        return szDest;
    }
    else
        return NULL;
}

LPWSTR apxWStrdupA(LPCTSTR szSource)
{
    return apxPoolWStrdup(_st_sys_pool, szSource);
}

LPSTR
apxPoolStrdupA(APXHANDLE hPool, LPCSTR szSource)
{
    if (szSource) {
        LPSTR szDest;
        DWORD l = lstrlenA(szSource);
        szDest = apxPoolAlloc(hPool, l + 1);
        lstrcpyA(szDest, szSource);
        return szDest;
    }
    else
        return NULL;
}

LPWSTR
apxPoolStrdupW(APXHANDLE hPool, LPCWSTR szSource)
{
    if (szSource) {
        LPWSTR szDest;
        DWORD l = lstrlenW(szSource);
        szDest = apxPoolAlloc(hPool, (l + 1) * sizeof(WCHAR));
        lstrcpyW(szDest, szSource);
        return szDest;
    }
    else
        return NULL;
}

LPSTR
apxStrdupA(LPCSTR szSource)
{
    return apxPoolStrdupA(_st_sys_pool, szSource);
}

LPWSTR
apxStrdupW(LPCWSTR szSource)
{
    return apxPoolStrdupW(_st_sys_pool, szSource);
}

APXHANDLE
apxHandleCreate(APXHANDLE hPool, DWORD dwFlags, 
                LPVOID lpData, DWORD  dwDataSize,
                LPAPXFNCALLBACK fnCallback)
{
    APXHANDLE   hHandle; 
    LPAPXPOOL   lpPool;
    
    if (IS_INVALID_HANDLE(hPool))
        hPool = _st_sys_pool;
    if (hPool->dwType != APXHANDLE_TYPE_POOL) {
        apxLogWrite(APXLOG_MARK_ERROR "Parent Handle type is not POOL %d",
                    hPool->dwType);
        return INVALID_HANDLE_VALUE;
    }
    hHandle = __apxPoolAllocCore(hPool, APXHANDLE_SZ + dwDataSize,
                                 HEAP_ZERO_MEMORY);
    
    hHandle->hPool             = hPool;
    if (fnCallback)
        hHandle->fnCallback = fnCallback;
    else
        hHandle->fnCallback = __apxHandleCallback;

    if (dwFlags & APXHANDLE_TYPE_WINHANDLE) {
        hHandle->dwFlags |= APXHANDLE_HAS_USERDATA;
        hHandle->dwFlags |= APXHANDLE_TYPE_WINHANDLE;
        hHandle->uData.hWinHandle = lpData;
    }
    else if (dwFlags & APXHANDLE_TYPE_LPTR) {
        hHandle->dwFlags |= APXHANDLE_HAS_USERDATA;
        hHandle->dwFlags |= APXHANDLE_TYPE_LPTR;
        hHandle->uData.lpPtr = lpData;
    }
    else if (dwDataSize && lpData) {
        hHandle->dwFlags |= APXHANDLE_HAS_USERDATA;
        AplCopyMemory(APXHANDLE_DATA(hHandle), lpData, dwDataSize);
        hHandle->dwSize = dwDataSize;
    }

    if (dwFlags & APXHANDLE_HAS_EVENT) {
        /* Create the message event and message wathcing thread */
        hHandle->hEventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
        hHandle->hEventThread = CreateThread(NULL, 0, __apxHandleEventThread,
                                            hHandle, 0,
                                            &(hHandle->hEventThreadId));
        if (IS_INVALID_HANDLE(hHandle->hEventThread)) {
            SAFE_CLOSE_HANDLE(hHandle->hEventHandle);
        }
        else
            hHandle->dwFlags |= APXHANDLE_HAS_EVENT;
    }
    TAILQ_INIT(&hHandle->lCallbacks);
    /* Add the handle to the pool's object list */
    lpPool = APXHANDLE_DATA(hPool);
    APXHANDLE_SPINLOCK(hPool);
    TAILQ_INSERT_HEAD(&lpPool->lHandles, hHandle, queue);
    ++hPool->dwSize;
    APXHANDLE_SPINUNLOCK(hPool);

    return hHandle;
}

BOOL
apxCloseHandle(APXHANDLE hObject)
{
    LPAPXPOOL   lpPool;
    APXCALLHOOK *lpCall;
    
    if (IS_INVALID_HANDLE(hObject) || hObject->dwType == APXHANDLE_TYPE_INVALID)
        return FALSE;
    /* Call the user callback first */
    (*hObject->fnCallback)(hObject, WM_CLOSE, 0, 0);
    /* Now go through the callback chain */
    TAILQ_FOREACH(lpCall, &hObject->lCallbacks, queue) {
        (*lpCall->fnCallback)(hObject, WM_CLOSE, 0, 0);
        TAILQ_REMOVE(&hObject->lCallbacks, lpCall, queue);
        __apxPoolFreeCore(lpCall);
    }

    hObject->dwType = APXHANDLE_TYPE_INVALID;
    if (hObject->dwFlags & APXHANDLE_HAS_EVENT) {
        DWORD dwState;

        SetEvent(hObject->hEventHandle);
        dwState = WaitForSingleObject(hObject->hEventThread, 1000);
        SAFE_CLOSE_HANDLE(hObject->hEventHandle);
        if (dwState == WAIT_TIMEOUT)
            TerminateThread(hObject->hEventThread, 0);
        SAFE_CLOSE_HANDLE(hObject->hEventThread);

        hObject->dwFlags &= ~APXHANDLE_HAS_EVENT;
    }

    /* finaly remove the object from the pool's object list */
    if (!IS_INVALID_HANDLE(hObject->hPool)) {
        lpPool = APXHANDLE_DATA(hObject->hPool);
        APXHANDLE_SPINLOCK(hObject->hPool);
        TAILQ_REMOVE(&lpPool->lHandles, hObject, queue);
        hObject->hPool->dwSize--;
        APXHANDLE_SPINUNLOCK(hObject->hPool);
        __apxPoolFreeCore(hObject);
    }
    return TRUE;
}

LPVOID
apxHandleGetUserData(APXHANDLE hObject)
{
    if (hObject->dwType == APXHANDLE_TYPE_INVALID)
        return NULL;
    if (hObject->dwFlags & APXHANDLE_HAS_USERDATA)
        return APXHANDLE_DATA(hObject);
    else
        return hObject->uData.lpPtr;
}

LPVOID      
apxHandleSetUserData(APXHANDLE hObject, LPVOID lpData, DWORD dwDataSize)
{
    if (hObject->dwType == APXHANDLE_TYPE_INVALID)
        return NULL;
    if (hObject->dwFlags & APXHANDLE_HAS_USERDATA && hObject->dwSize > 0) {
        AplCopyMemory(APXHANDLE_DATA(hObject), lpData,
                      MIN(hObject->dwSize, dwDataSize));
        return APXHANDLE_DATA(hObject);
    }
    else { 
        LPVOID lpOrg = hObject->uData.lpPtr;
        hObject->uData.lpPtr = lpData;
        return lpOrg;
    }
}

BOOL apxHandleSendMessage(APXHANDLE hObject, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL rv = TRUE;
    APXCALLHOOK *lpCall;
    if (hObject->dwType == APXHANDLE_TYPE_INVALID)
        return FALSE;
    /* Serialize requests to the callback */
    APXHANDLE_SPINLOCK(hObject);
    if (hObject->fnCallback)
        rv = (*hObject->fnCallback)(hObject, uMsg, wParam, lParam);
    TAILQ_FOREACH(lpCall, &hObject->lCallbacks, queue) {
        (*lpCall->fnCallback)(hObject, uMsg, wParam, lParam);
    }
    APXHANDLE_SPINUNLOCK(hObject);

    return rv;
}

BOOL apxHandlePostMessage(APXHANDLE hObject, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (hObject->dwType == APXHANDLE_TYPE_INVALID)
        return FALSE;
    if (hObject->dwFlags & APXHANDLE_HAS_EVENT) {
        /* TODO: Create a thread message queue 
         * Right now wait while the event gets nonsignaled
         */
        while (WaitForSingleObject(hObject->hEventHandle, 0) == WAIT_OBJECT_0)
            SwitchToThread();
        APXHANDLE_SPINLOCK(hObject);
        SuspendThread(hObject->hEventThread);
        hObject->uMsg   = uMsg;
        hObject->wParam = wParam;
        hObject->lParam = lParam;
        /* Signal the event thread to call the user callback */
        SetEvent(hObject->hEventHandle);
        ResumeThread(hObject->hEventThread);
        APXHANDLE_SPINUNLOCK(hObject);

        return TRUE;
    }
    return FALSE;
}

BOOL apxHandleLock(APXHANDLE hObject, BOOL bLock)
{
    if (bLock)
        APXHANDLE_SPINLOCK(hObject);
    else
        APXHANDLE_SPINUNLOCK(hObject);

    return TRUE;
}

BOOL apxHandleAddHook(APXHANDLE hObject, DWORD dwWhere,
                      LPAPXFNCALLBACK fnCallback)
{
    APXCALLHOOK *lpCall;

    if (hObject->dwType == APXHANDLE_TYPE_INVALID || !fnCallback)
        return FALSE;
    lpCall = (APXCALLHOOK *)__apxPoolAllocCore(hObject->hPool, 
                                               sizeof(APXCALLHOOK), 0);
    if (!lpCall)
        return FALSE;
    lpCall->fnCallback = fnCallback;
    APXHANDLE_SPINLOCK(hObject);
    if (dwWhere == APXHANDLE_HOOK_FIRST) {
        TAILQ_INSERT_HEAD(&hObject->lCallbacks, lpCall, queue);
    }
    else {
        TAILQ_INSERT_TAIL(&hObject->lCallbacks, lpCall, queue);
    }
    APXHANDLE_SPINUNLOCK(hObject);
    
    return TRUE;
}

DWORD apxHandleWait(APXHANDLE hHandle, DWORD dwMilliseconds, BOOL bKill)
{
    if (IS_INVALID_HANDLE(hHandle))
        return WAIT_ABANDONED;
    if (hHandle->dwType == APXHANDLE_TYPE_JVM)
        return apxJavaWait(hHandle, dwMilliseconds, bKill);
    else if (hHandle->dwType == APXHANDLE_TYPE_PROCESS)
        return apxProcessWait(hHandle, dwMilliseconds, bKill);
    else
        return WAIT_ABANDONED;
}

