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
 
#ifndef _PRIVATE_H_INCLUDED_
#define _PRIVATE_H_INCLUDED_

#include "mclib.h"

#ifdef _DEBUG

HANDLE  HeapCREATE(DWORD flOptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize);
BOOL    HeapDESTROY(HANDLE hHeap);

LPVOID  HeapALLOC(HANDLE hHeap, DWORD dwFlags, SIZE_T nSize);
BOOL    HeapFREE(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem);
LPVOID  HeapREALLOC(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes);

#else

#define HeapCREATE  HeapCreate
#define HeapDESTROY HeapDestroy
#define HeapALLOC   HeapAlloc
#define HeapFREE    HeapFree
#define HeapREALLOC HeapReAlloc

#endif

/*
 * Tail queue declarations.
 */
#define TAILQ_HEAD(name, type)                                          \
struct name {                                                           \
        struct type *tqh_first; /* first element */                     \
        struct type **tqh_last; /* addr of last next element */         \
}

#define TAILQ_HEAD_INITIALIZER(head)                                    \
        { NULL, &(head).tqh_first }

#define TAILQ_ENTRY(type)                                               \
struct {                                                                \
        struct type *tqe_next;  /* next element */                      \
        struct type **tqe_prev; /* address of previous next element */  \
}

/*
 * Tail queue functions.
 */
#define TAILQ_CONCAT(head1, head2, field) do {                          \
        if (!TAILQ_EMPTY(head2)) {                                      \
                *(head1)->tqh_last = (head2)->tqh_first;                \
                (head2)->tqh_first->field.tqe_prev = (head1)->tqh_last; \
                (head1)->tqh_last = (head2)->tqh_last;                  \
                TAILQ_INIT((head2));                                    \
        }                                                               \
} while (0)

#define TAILQ_EMPTY(head)       ((head)->tqh_first == NULL)

#define TAILQ_FIRST(head)       ((head)->tqh_first)

#define TAILQ_FOREACH(var, head, field)                                 \
        for ((var) = TAILQ_FIRST((head));                               \
            (var);                                                      \
            (var) = TAILQ_NEXT((var), field))

#define TAILQ_FOREACH_REVERSE(var, head, headname, field)               \
        for ((var) = TAILQ_LAST((head), headname);                      \
            (var);                                                      \
            (var) = TAILQ_PREV((var), headname, field))

#define TAILQ_INIT(head) do {                                           \
        TAILQ_FIRST((head)) = NULL;                                     \
        (head)->tqh_last = &TAILQ_FIRST((head));                        \
} while (0)

#define TAILQ_INSERT_AFTER(head, listelm, elm, field) do {              \
        if ((TAILQ_NEXT((elm), field) = TAILQ_NEXT((listelm), field)) != NULL)\
                TAILQ_NEXT((elm), field)->field.tqe_prev =              \
                    &TAILQ_NEXT((elm), field);                          \
        else {                                                          \
                (head)->tqh_last = &TAILQ_NEXT((elm), field);           \
        }                                                               \
        TAILQ_NEXT((listelm), field) = (elm);                           \
        (elm)->field.tqe_prev = &TAILQ_NEXT((listelm), field);          \
} while (0)

#define TAILQ_INSERT_BEFORE(listelm, elm, field) do {                   \
        (elm)->field.tqe_prev = (listelm)->field.tqe_prev;              \
        TAILQ_NEXT((elm), field) = (listelm);                           \
        *(listelm)->field.tqe_prev = (elm);                             \
        (listelm)->field.tqe_prev = &TAILQ_NEXT((elm), field);          \
} while (0)

#define TAILQ_INSERT_HEAD(head, elm, field) do {                        \
        if ((TAILQ_NEXT((elm), field) = TAILQ_FIRST((head))) != NULL)   \
                TAILQ_FIRST((head))->field.tqe_prev =                   \
                    &TAILQ_NEXT((elm), field);                          \
        else                                                            \
                (head)->tqh_last = &TAILQ_NEXT((elm), field);           \
        TAILQ_FIRST((head)) = (elm);                                    \
        (elm)->field.tqe_prev = &TAILQ_FIRST((head));                   \
} while (0)

#define TAILQ_INSERT_TAIL(head, elm, field) do {                        \
        TAILQ_NEXT((elm), field) = NULL;                                \
        (elm)->field.tqe_prev = (head)->tqh_last;                       \
        *(head)->tqh_last = (elm);                                      \
        (head)->tqh_last = &TAILQ_NEXT((elm), field);                   \
} while (0)

#define TAILQ_LAST(head, headname)                                      \
        (*(((struct headname *)((head)->tqh_last))->tqh_last))

#define TAILQ_NEXT(elm, field) ((elm)->field.tqe_next)

#define TAILQ_PREV(elm, headname, field)                                \
        (*(((struct headname *)((elm)->field.tqe_prev))->tqh_last))

#define TAILQ_REMOVE(head, elm, field) do {                             \
        if ((TAILQ_NEXT((elm), field)) != NULL)                         \
                TAILQ_NEXT((elm), field)->field.tqe_prev =              \
                    (elm)->field.tqe_prev;                              \
        else {                                                          \
                (head)->tqh_last = (elm)->field.tqe_prev;               \
        }                                                               \
        *(elm)->field.tqe_prev = TAILQ_NEXT((elm), field);              \
} while (0)
  
/** Some usefull macros */

#define APXHANDLE_SPINLOCK(h)               \
    APXMACRO_BEGIN                          \
    while (InterlockedCompareExchange(&((h)->lvSpin), 1, 0) != 0) { \
        Sleep(10);                          \
        SwitchToThread();                   \
    }                                       \
    APXMACRO_END

#define APXHANDLE_SPINUNLOCK(h)             \
    APXMACRO_BEGIN                          \
    InterlockedExchange(&((h)->lvSpin), 0); \
    APXMACRO_END

#define APX_SPINLOCK(lock)                  \
    APXMACRO_BEGIN                          \
    while (InterlockedCompareExchange(&(lock), 1, 0) != 0) \
        SwitchToThread();                   \
    APXMACRO_END

#define APX_SPINUNLOCK(lock)                \
    APXMACRO_BEGIN                          \
    InterlockedExchange(&(lock), 0);        \
    APXMACRO_END

/*
 * Define a union with types which are likely to have the longest
 * *relevant* CPU-specific memory word alignment restrictions...
 */ 
typedef union APXMEMWORD {
    void  *vp;
    void (*fp)(void);
    char  *cp;
    long   l;
    double d;
} APXMEMWORD;

typedef struct APXCALLHOOK APXCALLHOOK;

struct APXCALLHOOK {

    LPAPXFNCALLBACK     fnCallback;
    TAILQ_ENTRY(APXCALLHOOK)  queue;
};

struct stAPXHANDLE {
    /** The type of the handle */ 
    DWORD               dwType;         
    /** Handle Flags */ 
    DWORD               dwFlags;
    /** Handle user data size */ 
    DWORD               dwSize;
    /** parameters for event callback */ 
    WPARAM              wParam;
    LPARAM              lParam;
    UINT                uMsg;
    /** main callback function (using default if not specified) */ 
    LPAPXFNCALLBACK     fnCallback;
    /** callback functions hook list */
    TAILQ_HEAD(_lCallbacks, APXCALLHOOK) lCallbacks;
    /** allocation pool  */
    APXHANDLE           hPool;
    /** interlocking value */ 
    LONG volatile       lvSpin;

    /** message event handle  */ 
    HANDLE              hEventHandle;
    /** message event thread  */ 
    HANDLE              hEventThread;
    /** message event thread id  */ 
    DWORD               hEventThreadId;
    /** private local heap */
    HANDLE              hHeap;
    /** list enty for pool  */ 
    TAILQ_ENTRY(stAPXHANDLE)  queue;
    /** small userdata pointer  */ 
    union   {
        LPVOID          lpPtr;
        HANDLE          hWinHandle;
        double          dValue;
        void            (*fpValue)();
    } uData;

    APXMEMWORD          stAlign;
};

#define APXHANDLE_DATA(h)       ((void *)((char*)(h) + sizeof(stAPXHANDLE)))
#define APXHANDLE_SZ            sizeof(stAPXHANDLE)

extern APX_OSLEVEL  _st_apx_oslevel;

#define APX_GET_OSLEVEL()   ((_st_apx_oslevel == APX_WINVER_UNK) ? apxGetOsLevel() : _st_apx_oslevel)

/* zero separated, double zero terminated string */
struct APXMULTISZ {
    DWORD   dwAllocated;  /* length including terminators */
    DWORD   dwInsert;     /* next insert position */
};

typedef struct APXREGENUM {
    HKEY     hServicesKey;
    DWORD    dwIndex;                   /* current enum index           */
    DWORD    cSubKeys;                  /* number of subkeys            */
    DWORD    cbMaxSubKey;               /* longest subkey size          */
    DWORD    cchMaxClass;               /* longest class string         */
    DWORD    cValues;                   /* number of values for key     */
    DWORD    cchMaxValue;               /* longest value name           */
    DWORD    cbMaxValueData;            /* longest value data           */

} APXREGENUM, *LPAPXREGENUM;

BOOL    apxRegistryEnumServices(LPAPXREGENUM lpEnum, LPAPXSERVENTRY lpEntry);
BOOL    apxGetServiceDescriptionW(LPCWSTR szServiceName, LPWSTR szDescription,
                                  DWORD dwDescriptionLength);
BOOL    apxGetServiceUserW(LPCWSTR szServiceName, LPWSTR szUser,
                           DWORD dwUserLength);

DWORD   __apxGetMultiSzLengthA(LPCSTR lpStr, LPDWORD lpdwCount);
DWORD   __apxGetMultiSzLengthW(LPCWSTR lpStr, LPDWORD lpdwCount);
LPSTR   __apxGetEnvironmentVariableA(APXHANDLE hPool, LPCSTR szName);
LPWSTR  __apxGetEnvironmentVariableW(APXHANDLE hPool, LPCWSTR wsName);

#endif /* _PRIVATE_H_INCLUDED_ */
