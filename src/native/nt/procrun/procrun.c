/* ====================================================================
     Copyright 2002-2004 The Apache Software Foundation.
   
     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at
   
         http://www.apache.org/licenses/LICENSE-2.0
   
     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/

/* ====================================================================
 * procrun
 *
 * Contributed by Mladen Turk <mturk@apache.org>
 *
 * 05 Aug 2002
 * ==================================================================== 
 */

#ifndef STRICT
#define STRICT
#endif
#ifndef OEMRESOURCE
#define OEMRESOURCE
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <objbase.h>
#include <shlobj.h>
#include <stdlib.h>
#include <stdio.h>

#include <Shlwapi.h>
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <time.h>
#include <stdarg.h>
#include <jni.h>
 
#include "procrun.h"

typedef HANDLE (__stdcall * PFNCREATERTHRD)(HANDLE, LPSECURITY_ATTRIBUTES, 
                                            DWORD, LPTHREAD_START_ROUTINE,
                                            LPVOID, DWORD, LPDWORD); 
typedef jint (JNICALL *JNI_GETDEFAULTJAVAVMINITARGS)(void *);
typedef jint (JNICALL *JNI_CREATEJAVAVM)(JavaVM **, JNIEnv **, void *);
typedef jint (JNICALL *JNI_GETCREATEDJAVAVMS)(JavaVM **, int, int *);

JNI_GETDEFAULTJAVAVMINITARGS jni_JNI_GetDefaultJavaVMInitArgs = NULL;
JNI_CREATEJAVAVM jni_JNI_CreateJavaVM = NULL;
JNI_GETCREATEDJAVAVMS jni_JNI_GetCreatedJavaVMs = NULL;

int report_service_status(DWORD, DWORD, DWORD, process_t *); 
int procrun_redirect(char *program, char **envp, procrun_t *env, int starting);

static int g_proc_stderr_file = 0;
int g_proc_mode = 0;
/* The main envronment for services */
procrun_t *g_env = NULL;
static int g_is_windows_nt = 0;

#ifdef PROCRUN_WINAPP


#endif

#ifdef _DEBUG
void log_write(char *string)
{
    FILE *fd;
    fd = fopen("c:\\jakarta-service.txt","a");
    if (fd == NULL)
    return;
    fprintf(fd,string);
    if (string[strlen(string)-1]!='\n')
        fprintf(fd,"\n");
    fclose(fd);
}
void dbprintf(char *format, ...)
{
    va_list args;
    char tid[4096 + 128];
    char buffer[4096];
    int len;

    if (!format) {
        len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | 
                            FORMAT_MESSAGE_IGNORE_INSERTS,
                            NULL,
                            GetLastError(),
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            buffer,
                            4096,
                            NULL);
    }
    else {
        va_start(args, format);
        len = _vsnprintf(buffer, 4096, format, args);
        va_end(args);
    }
    if (len > 0) {
        sprintf(tid, "[%04X:%08d] %s", GetCurrentThreadId(),time(NULL), buffer);
        if (g_proc_stderr_file > 0)
            write(g_proc_stderr_file, tid, strlen(tid));
        else
            fprintf(stderr,tid);
    log_write(tid);
#ifdef _DEBUG_TRACE
        OutputDebugString(tid);
#endif
    }
}
#define DBPRINTF0(v1)               dbprintf(v1)
#define DBPRINTF1(v1, v2)           dbprintf(v1, v2)
#define DBPRINTF2(v1, v2, v3)       dbprintf(v1, v2, v3)
#else
#define DBPRINTF0(v1)
#define DBPRINTF1(v1, v2)
#define DBPRINTF2(v1, v2, v3)
#endif

/* Create the memory pool.
 * Memory pool is fixed size (PROC_POOL_SIZE -> 128 by default)
 * It ensures that all the memory and HANDELS gets freed
 * when the procrun exits
 */
pool_t *pool_create()
{
    pool_t *pool = malloc(sizeof(pool_t));
    if (pool) {
        InitializeCriticalSection(&pool->lock);
        pool->size = 0;
        pool->mp[0].m = NULL;
        pool->mp[0].h = NULL;
    }
    return pool;
}

/* Destroy the memory pool
 * each pool slot can have allocated memory block
 * and/or associated windows HANDLE that can be
 * release with CloseHandle.
 */
int pool_destroy(pool_t *pool)
{
    int i = 0;
    for (i = 0; i < pool->size; i++) {
        if (pool->mp[i].m) {
            free(pool->mp[i].m);
        }
        if (pool->mp[i].h != INVALID_HANDLE_VALUE && pool->mp[i].h != NULL) {
            CloseHandle(pool->mp[i].h);
        }
    }
    DeleteCriticalSection(&pool->lock);
    free(pool);
    return i;
}

/*  Allocation functions
 *  They doesn't check for overflow
 */
static void *pool_alloc(pool_t *pool, size_t size)
{
    void *m = malloc(size);
    EnterCriticalSection(&pool->lock);
    pool->mp[pool->size].h = INVALID_HANDLE_VALUE;
    pool->mp[pool->size++].m = m;
    LeaveCriticalSection(&pool->lock);
    return m;
}

static void *pool_calloc(pool_t *pool, size_t size)
{
    void *m = calloc(size, 1);
    EnterCriticalSection(&pool->lock);
    pool->mp[pool->size].h = INVALID_HANDLE_VALUE;
    pool->mp[pool->size++].m = m;
    LeaveCriticalSection(&pool->lock);
    return m;
}

static char *pool_strdup(pool_t *pool, const char *src)
{
    char *s = strdup(src);
    EnterCriticalSection(&pool->lock);
    pool->mp[pool->size].h = INVALID_HANDLE_VALUE;
    pool->mp[pool->size++].m = s;
    LeaveCriticalSection(&pool->lock);
    return s;
}

/* Attach the Handle to the pool
 * The handle will be released on pool_destroy
 * using CloseHandle API call
 */
static void *pool_handle(pool_t *pool, HANDLE h)
{
    EnterCriticalSection(&pool->lock);
    pool->mp[pool->size].h = h;
    pool->mp[pool->size++].m = NULL;
    LeaveCriticalSection(&pool->lock);
    return h;
}

static BOOL pool_close_handle(pool_t *pool, HANDLE h)
{
    int i;
    EnterCriticalSection(&pool->lock);
    for(i=0; i < pool->size; i++) {
        if(pool->mp[i].h == h) 
            pool->mp[i].h = INVALID_HANDLE_VALUE;
    }
    LeaveCriticalSection(&pool->lock);
    return CloseHandle(h);
}

/* Very simple encryption for hiding password
 * they can be easily decrypted cause the key
 * is hadcoded (100) in the code.
 * It can be easily cracked if someone finds that needed.
 * XXX: The solution is to either use the CryproAPI
 * or our own account management.
 */
static void simple_encrypt(int seed, const char *str, unsigned char bytes[256])
{
    int i;
    char sc[256];

    srand(seed);
    memset(sc, 0, 256);
    strncpy(sc, str, 255);
    for (i = 0; i < 256; i ++) {
        bytes[i] = ((rand() % 256) ^ sc[i]);
    }
}

static void simple_decrypt(int seed, char *str, unsigned char bytes[256])
{
    int i;
    char sc[256];

    srand(seed);
    for (i = 0; i < 256; i ++) {
        sc[i] = ((rand() % 256) ^ bytes[i]);
    }
    strcpy(str, sc);
}

/* Injects the 'ExitProcess' to the child
 * The function tries to kill the child process
 * without using 'hard' mathods like TerminateChild.
 * At first it sends the CTRL+C and CTRL+BREAK to the
 * child process. If that fails (the child doesn't exit)
 * it creates the remote thread in the address space of
 * the child process, and calls the ExitProcess function,
 * inside the child process.
 * Finaly it calls the TerminateProcess all of the above
 * fails.
 * Well designed console clients usually exits on closing
 * stdin stream, so this function not be called in most cases.
 */
static void inject_exitprocess(PROCESS_INFORMATION *child) 
{
    PFNCREATERTHRD pfn_CreateRemoteThread;
    UINT exit_code = 2303;
    HANDLE rt = NULL, dup = NULL;
    DWORD  rtid, stat;
    BOOL isok;

    if (!child || !child->hProcess) {
        DBPRINTF0("No process\n");
        return;
    }
    if (!GetExitCodeProcess(child->hProcess, &stat) || 
        (stat != STILL_ACTIVE)) {
            DBPRINTF1("The child process %d isn't active any more\n",
                     child->dwProcessId);
            child->hProcess = NULL;
            child->dwProcessId = 0;
            return;
    } 
    GenerateConsoleCtrlEvent(CTRL_C_EVENT, child->dwProcessId);
    GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, child->dwProcessId);
#if 1
    /* Wait for a child to capture CTRL_ events. */
    WaitForSingleObject(child->hThread, 2000);
#endif
    if (!GetExitCodeProcess(child->hProcess, &stat) || 
        (stat != STILL_ACTIVE)) {
            child->hProcess = NULL;
            child->dwProcessId = 0;
            DBPRINTF0("Breaked by CTRL+C event\n");
            return;
    } 
    DBPRINTF1("Injecting ExitProcess to the child process %d\n",
             child->dwProcessId);

    pfn_CreateRemoteThread = (PFNCREATERTHRD)GetProcAddress(
                                             GetModuleHandle("KERNEL32.DLL"),
                                            "CreateRemoteThread");

    isok = DuplicateHandle(GetCurrentProcess(), 
                           child->hProcess, 
                           GetCurrentProcess(), 
                           &dup, 
                           PROCESS_ALL_ACCESS, 
                           FALSE, 0);
    isok = GetExitCodeProcess((isok) ? dup : child->hProcess, &stat);
    if (pfn_CreateRemoteThread) {        
        FARPROC pfnExitProc;
        if (isok && stat == STILL_ACTIVE) { 
           
            pfnExitProc = GetProcAddress(GetModuleHandle("KERNEL32.DLL"),
                                         "ExitProcess");
            rt = pfn_CreateRemoteThread(child->hProcess, 
                                        NULL, 
                                        0, 
                                        (LPTHREAD_START_ROUTINE)pfnExitProc,
                                        (PVOID)exit_code, 0, &rtid);
       }
    }
    else {
        if (isok && stat == STILL_ACTIVE) {
            DBPRINTF0("Could not CreateRemoteThread... Forcing TerminateProcess\n");
            TerminateProcess(child->hProcess, exit_code);
        }
    }

    if (rt) {
        if (WaitForSingleObject(child->hProcess, 2000) == WAIT_OBJECT_0) {
            CloseHandle(rt);
            DBPRINTF0("Exited cleanly\n");
        }
        else {
            DBPRINTF0("Forcing TerminateProcess\n");
            TerminateProcess(child->hProcess, exit_code);
        }
    }
    if (dup)
        CloseHandle(dup);
} 

int __cdecl compare(const void *arg1, const void *arg2)
{
    return _stricoll(*((char **)arg1),*((char **)arg2));
}

/* Merge two char arrays and make
 * zero separated, double-zero terminated
 * string
 */
static char * merge_arrays(char **one, char **two, process_t *proc)
{
    int len = 0, n, cnt = 0;
    char *envp, *p;

    for (n = 0; one[n]; n++) {
        len += (strlen(one[n]) + 1);
        cnt++;
    }
    for (n = 0; two[n]; n++) {
        len += (strlen(two[n]) + 1);        
        cnt++;
    }
    p = envp = (char *)pool_calloc(proc->pool, len + 1);

    for (n = 0; one[n]; n++) {
        strcpy(p, one[n]);
        p += strlen(one[n]) + 1;
    }
    for (n = 0; two[n]; n++) {
        strcpy(p, two[n]);
        p += strlen(two[n]) + 1;
    }
    return envp;
}

/* Make the environment string
 * for the child process.
 * The original environment of the calling process
 * is merged with the current environment.
 */
static char * make_environment(char **envarr, char **envorg, process_t *proc)
{
    int len = 0, n, cnt = 0;
    char *envp, *p, **tmp;

    for (n = 0; envarr[n]; n++) {
        len += strlen(envarr[n]) + 1;
        cnt++;
    }
    for (n = 0; envorg[n]; n++) {
        len += strlen(envorg[n]) + 1;        
        cnt++;
    }
    if (proc->java.jpath) {
        len += strlen(proc->java.jpath) + 2;
        cnt++;
    }
    p = envp = (char *)pool_calloc(proc->pool, len + 1);
    if (!p)
        return NULL;
    tmp = (char **)calloc(cnt + 1, sizeof(char *));
    cnt = 0;
    for (n = 0; envarr[n]; n++) {
            tmp[cnt++] = envarr[n];
    }
    for (n = 0; envorg[n]; n++) {
        if (STRN_COMPARE(envorg[n], PROCRUN_ENV_STDIN)) {}
        else if (STRN_COMPARE(envorg[n], PROCRUN_ENV_STDOUT)) {}
        else if (STRN_COMPARE(envorg[n], PROCRUN_ENV_STDERR)) {}
        else if (STRN_COMPARE(envorg[n], PROCRUN_ENV_PPID)) {}
        else if (STRNI_COMPARE(envorg[n], "PATH=") && 
                 proc->java.jpath) {
            tmp[cnt] = pool_calloc(proc->pool, strlen(envorg[n]) +
                                     strlen(proc->java.jpath) + 2);
            strcpy(tmp[cnt], envorg[n]);
            strcat(tmp[cnt], ";");
            strcat(tmp[cnt], proc->java.jpath);
            DBPRINTF1("New PATH %s", tmp[cnt]);
            ++cnt;
        }
        else
            tmp[cnt++] = envorg[n];
    }
    qsort((void *)tmp, (size_t)n, sizeof(char *), compare);
    for (n = 0; tmp[n]; n++) {
        strcpy(p, tmp[n]);
        p += strlen(tmp[n]) + 1;
    }
    free(tmp);
    return envp;
}

/* Make the character string array form
 * zero separated, double-zero terminated
 * strings. Those strings comes from some
 * Windows api calls, like GetEnvironmentStrings
 * This string format is also used in Registry (REG_MULTI_SZ)
 */
static int make_array(const char *str, char **arr, int size, process_t *proc)
{
    int i = 0;
    char *p;
    
    if (!str)
        return 0;
    for (p = (char *)str; p && *p; p++) {
        arr[i] = pool_strdup(proc->pool, p);
        while (*p)
            p++;
        i++;
        if (i + 1 > size)
            break;
    }
    return i;
}

/* Simple string unqouting
 * TODO: Handle multiqoutes.
 */
static char *remove_quotes(char * string) {
  char *p = string, *q = string;
  while (*p) {
    if(*p != '\"' && *p != '\'')
      *q++ = *p;
    ++p;
  }
  *q = '\0';
  return string;
}

/*  Parse command line argument.
 *  First command param starts with //name//value
 */
static int parse_args(int argc, char **argv, process_t *proc)
{
    int mode = 0;
    char *arg = argv[1];

    if ((strlen(arg) > 5) && arg[0] == '/' && arg[1] == '/') {
        if (STRN_COMPARE(arg, PROC_ARG_ENVPREFIX)) {
            proc->env_prefix = arg + STRN_SIZE(PROC_ARG_ENVPREFIX);
            mode = PROCRUN_CMD_ENVPREFIX;
        }
        else if (STRN_COMPARE(arg, PROC_ARG_RUN_JAVA)) {
            proc->java.path = pool_strdup(proc->pool, arg + 
                                          STRN_SIZE(PROC_ARG_RUN_JAVA));
            mode = PROCRUN_CMD_RUN_JAVA;
        }
        else if (STRN_COMPARE(arg, PROC_ARG_INSTALL_SERVICE)) {
            proc->service.name = pool_strdup(proc->pool, arg + 
                                          STRN_SIZE(PROC_ARG_INSTALL_SERVICE));
            mode = PROCRUN_CMD_INSTALL_SERVICE;
        }
        else if (STRN_COMPARE(arg, PROC_ARG_RUN_SERVICE)) {
            proc->service.name = pool_strdup(proc->pool, arg + 
                                             STRN_SIZE(PROC_ARG_RUN_SERVICE));
            mode = PROCRUN_CMD_RUN_SERVICE;
        }
        else if (STRN_COMPARE(arg, PROC_ARG_TEST_SERVICE)) {
            proc->service.name = pool_strdup(proc->pool, arg + 
                                             STRN_SIZE(PROC_ARG_TEST_SERVICE));
            mode = PROCRUN_CMD_TEST_SERVICE;
        }
#ifdef PROCRUN_WINAPP
        else if (STRN_COMPARE(arg, PROC_ARG_GUIT_SERVICE)) {
            proc->service.name = pool_strdup(proc->pool, arg + 
                                             STRN_SIZE(PROC_ARG_GUIT_SERVICE));
            mode = PROCRUN_CMD_GUIT_SERVICE;
            ac_use_try = 1;
            ac_use_dlg = 1;
        }
        else if (STRN_COMPARE(arg, PROC_ARG_GUID_SERVICE)) {
            proc->service.name = pool_strdup(proc->pool, arg + 
                                             STRN_SIZE(PROC_ARG_GUID_SERVICE));
            mode = PROCRUN_CMD_GUID_SERVICE;
            ac_use_dlg = 1;
            ac_use_show = 1;
        }
        else if (STRN_COMPARE(arg, PROC_ARG_GUID_PROCESS)) {
            mode = PROCRUN_CMD_GUID_PROCESS;
            ac_use_dlg = 1;
            ac_use_show = 1;
        }
        else if (STRN_COMPARE(arg, PROC_ARG_EDIT_SERVICE)) {
            proc->service.name = pool_strdup(proc->pool, arg + 
                                             STRN_SIZE(PROC_ARG_EDIT_SERVICE));

            mode = PROCRUN_CMD_EDIT_SERVICE;
            ac_use_props = 1;
        }
#endif
        else if (STRN_COMPARE(arg, PROC_ARG_STOP_SERVICE)) {
            proc->service.name = pool_strdup(proc->pool, arg +
                                             STRN_SIZE(PROC_ARG_STOP_SERVICE));
            mode = PROCRUN_CMD_STOP_SERVICE;
        }
        else if (STRN_COMPARE(arg, PROC_ARG_DELETE_SERVICE)) {
            proc->service.name = pool_strdup(proc->pool, arg +
                                           STRN_SIZE(PROC_ARG_DELETE_SERVICE));
            mode = PROCRUN_CMD_DELETE_SERVICE;
        }
        else if (STRN_COMPARE(arg, PROC_ARG_UPDATE_SERVICE)) {
            proc->service.name = pool_strdup(proc->pool, arg +
                                           STRN_SIZE(PROC_ARG_UPDATE_SERVICE));
            mode = PROCRUN_CMD_UPDATE_SERVICE;
        }
    }
    return mode;
}

/* Print some statistics about the current process
 *
 */
static void debug_process(int argc, char **argv, process_t *p)
{
    DBPRINTF1("DUMPING %s\n", argv[0]);
    DBPRINTF0("  SERVICE:\n");
    DBPRINTF1("    name        : %s\n", p->service.name);
    DBPRINTF1("    description : %s\n", p->service.description);
    DBPRINTF1("    path        : %s\n", p->service.path);
    DBPRINTF1("    image       : %s\n", p->service.image);
    DBPRINTF1("    infile      : %s\n", p->service.inname);
    DBPRINTF1("    outfile     : %s\n", p->service.outname);
    DBPRINTF1("    errfile     : %s\n", p->service.errname);
    DBPRINTF1("    argvw       : %s\n", p->argw);
    DBPRINTF0("  JAVAVM:\n");
    DBPRINTF1("    name        : %s\n", p->java.path);
    DBPRINTF1("    start       : %s\n", p->java.start_class);
    DBPRINTF1("    stop        : %s\n", p->java.stop_class);
    DBPRINTF1("    start method: %s\n", p->java.start_method);
    DBPRINTF1("    stop method : %s\n", p->java.stop_method);
    DBPRINTF1("    start param : %s\n", p->java.start_param);
    DBPRINTF1("    stop param  : %s\n", p->java.stop_param);

    DBPRINTF0("DONE...\n");
}

/* Add the environment variable 'name=value'
 * to the environment that will be passed to the child process.
 */
static int procrun_addenv(char *name, char *value, int val, process_t *proc)
{
    int i;

    for (i = 0; i < PROC_ENV_COUNT; i++)
        if (proc->env[i] == NULL)
            break;
    if (i < PROC_ENV_COUNT) {
        if (value) {
            proc->env[i] = (char *)pool_alloc(proc->pool, strlen(name) + 
                                              strlen(value) + 1);
            strcpy(proc->env[i], name);
            strupr(proc->env[i]);
            strcat(proc->env[i], value);
        }
        else {
            proc->env[i] = (char *)pool_alloc(proc->pool, strlen(name) + 34);
            strcpy(proc->env[i], name);
            strupr(proc->env[i]);
            itoa(val, proc->env[i] + strlen(name), 10);
        }
        return i;
    }
    else
        return -1;
}

/* Read the environment.
 * This function reads the procrun defined environment
 * variables passed from the calling process,
 * if the calling process is a procrun instance.
 */
static int procrun_readenv(process_t *proc, char **envp)
{
    int i, rv = 0;
    char *env;
    HANDLE h;

    for (i = 0; envp[i]; i++) {
        env = envp[i];
        if (STRN_COMPARE(env, PROCRUN_ENV_STDIN)) {
            h = (HANDLE)atoi(env + STRN_SIZE(PROCRUN_ENV_STDIN));
            if (!DuplicateHandle(proc->pinfo.hProcess, 
                                 h,
                                 proc->pinfo.hProcess,
                                 &proc->h_stdin[0], 
                                 0L, TRUE, DUPLICATE_SAME_ACCESS)) {
                DBPRINTF1("%s\tDuplicateHandle for STDIN failed\n", env[i]);
                proc->h_stdin[0] = INVALID_HANDLE_VALUE;
                return 0;
            }
            CloseHandle(h);
            ++rv;
        }
        else if (STRN_COMPARE(env, PROCRUN_ENV_STDOUT)) {
            h = (HANDLE)atoi(env + STRN_SIZE(PROCRUN_ENV_STDOUT));
            if (!DuplicateHandle(proc->pinfo.hProcess, 
                                 h,
                                 proc->pinfo.hProcess,
                                 &proc->h_stdout[1], 
                                 0L, TRUE, DUPLICATE_SAME_ACCESS)) {
                DBPRINTF1("%s\tDuplicateHandle for STDOUT failed\n", env[i]);
                proc->h_stdout[1] = INVALID_HANDLE_VALUE;
                return 0;
            }
            CloseHandle(h);
            ++rv;
        }
        else if (STRN_COMPARE(env, PROCRUN_ENV_STDERR)) {
            h = (HANDLE)atoi(env + STRN_SIZE(PROCRUN_ENV_STDERR));
            if (!DuplicateHandle(proc->pinfo.hProcess, 
                                 h,
                                 proc->pinfo.hProcess,
                                 &proc->h_stderr[1], 
                                 0L, TRUE, DUPLICATE_SAME_ACCESS)) {
                DBPRINTF1("%s\tDuplicateHandle for STDERR failed\n", env[i]);
                proc->h_stderr[1] = INVALID_HANDLE_VALUE;
                return 0;
            }
            CloseHandle(h);
            ++rv;
        }
        else if (STRN_COMPARE(env, PROCRUN_ENV_ERRFILE)) {
            h = (HANDLE)atoi(env + STRN_SIZE(PROCRUN_ENV_ERRFILE));
            g_proc_stderr_file = _open_osfhandle((long)h,
                                                  _O_APPEND | _O_TEXT);
        }
        else if (STRN_COMPARE(env, PROCRUN_ENV_PPID)) {
            proc->ppid = atoi(env + STRN_SIZE(PROCRUN_ENV_PPID));
        }

    }
    return rv;
}

/* Find the default jvm.dll
 * The function scans through registry and finds
 * default JRE jvm.dll.
 */
static char* procrun_guess_jvm(process_t *proc)
{
    HKEY hkjs;
    char jvm[MAX_PATH+1];
    char reg[MAX_PATH+1];
    char *cver;
    unsigned long err, klen = MAX_PATH;
    
    strcpy(reg, JAVASOFT_REGKEY);
    cver = &reg[sizeof(JAVASOFT_REGKEY)-1];

    if ((err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, reg,
                            0, KEY_READ, &hkjs)) != ERROR_SUCCESS) {
       DBPRINTF0("procrun_guess_jvm() failed to open Registry key\n");
       return NULL;
    }
    if ((err = RegQueryValueEx(hkjs, "CurrentVersion", NULL, NULL, 
                               (unsigned char *)cver,
                               &klen)) != ERROR_SUCCESS) {
        DBPRINTF0("procrun_guess_jvm() failed obtaining Current Java Version\n");
        RegCloseKey(hkjs);
        return NULL;
    }
    RegCloseKey(hkjs);
    
    if ((err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, reg,
                            0, KEY_READ, &hkjs) ) != ERROR_SUCCESS) {
        DBPRINTF1("procrun_guess_jvm() failed to open Registry key %s\n", reg);
        return NULL;
    }
    klen = MAX_PATH;
    if ((err = RegQueryValueEx(hkjs, "RuntimeLib", NULL, NULL, 
                               (unsigned char *)jvm, 
                               &klen)) != ERROR_SUCCESS) {
        DBPRINTF0("procrun_guess_jvm() failed obtaining Runtime Library\n");
        RegCloseKey(hkjs);
        return NULL;
    }
    RegCloseKey(hkjs);

    return pool_strdup(proc->pool, jvm);
} 

/* Find the java/javaw (depending on image)
 * The function locates the JavaHome Registry entry
 * and merges that path with the requested image
 */

static char* procrun_guess_java(process_t *proc, const char *image)
{
    HKEY hkjs;
    char jbin[MAX_PATH+1];
    char reg[MAX_PATH+1];
    char *cver;
    unsigned long err, klen = MAX_PATH;
    
    if((cver = getenv("JAVA_HOME")) != NULL) {
        strcpy(jbin,cver);
    } else {
        strcpy(reg, JAVASOFT_REGKEY);
        cver = &reg[sizeof(JAVASOFT_REGKEY)-1];

        if ((err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, reg,
                            0, KEY_READ, &hkjs)) != ERROR_SUCCESS) {
           DBPRINTF0("procrun_guess_jvm() failed to open Registry key\n");
           return NULL;
        }
        if ((err = RegQueryValueEx(hkjs, "CurrentVersion", NULL, NULL, 
                               (unsigned char *)cver,
                               &klen)) != ERROR_SUCCESS) {
            DBPRINTF0("procrun_guess_jvm() failed obtaining Current Java Version\n");
            RegCloseKey(hkjs);
            return NULL;
        }
        RegCloseKey(hkjs);
    
        if ((err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, reg,
                            0, KEY_READ, &hkjs) ) != ERROR_SUCCESS) {
            DBPRINTF1("procrun_guess_jvm() failed to open Registry key %s\n", reg);
            return NULL;
        }
        klen = MAX_PATH;
        if ((err = RegQueryValueEx(hkjs, "JavaHome", NULL, NULL, 
                               (unsigned char *)jbin, 
                               &klen)) != ERROR_SUCCESS) {
            DBPRINTF0("procrun_guess_jvm() failed obtaining Java path\n");
            RegCloseKey(hkjs);
            return NULL;
        }
        RegCloseKey(hkjs);
    }
    strcat(jbin, "\\bin\\");
    strcat(jbin, image);
    strcat(jbin, ".exe");
    return pool_strdup(proc->pool, jbin);
} 

/* Find the system JavaHome path.
 * The "JAVA_HOME" environment variable
 * gets procedance over registry settings
 */
static char* procrun_guess_java_home(process_t *proc)
{
    HKEY hkjs;
    char jbin[MAX_PATH+1];
    char reg[MAX_PATH+1];
    char *cver;
    unsigned long err, klen = MAX_PATH;
    
    if ((cver = getenv("JAVA_HOME")) != NULL) {
        strcpy(jbin, cver);
        strcat(jbin, "\\bin");
        return pool_strdup(proc->pool, jbin);
    }
    strcpy(reg, JAVAHOME_REGKEY);
    cver = &reg[sizeof(JAVAHOME_REGKEY)-1];

    if ((err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, reg,
                            0, KEY_READ, &hkjs)) != ERROR_SUCCESS) {
       DBPRINTF0("procrun_guess_jvm() failed to open Registry key\n");
       return NULL;
    }
    if ((err = RegQueryValueEx(hkjs, "CurrentVersion", NULL, NULL, 
                               (unsigned char *)cver,
                               &klen)) != ERROR_SUCCESS) {
        DBPRINTF0("procrun_guess_jvm() failed obtaining Current Java SDK Version\n");
        RegCloseKey(hkjs);
        return NULL;
    }
    RegCloseKey(hkjs);
    
    if ((err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, reg,
                            0, KEY_READ, &hkjs)) != ERROR_SUCCESS) {
        DBPRINTF1("procrun_guess_jvm() failed to open Registry key %s\n", reg);
        return NULL;
    }
    klen = MAX_PATH;
    if ((err = RegQueryValueEx(hkjs, "JavaHome", NULL, NULL, 
                               (unsigned char *)jbin, 
                               &klen)) != ERROR_SUCCESS) {
        DBPRINTF0("procrun_guess_jvm() failed obtaining Java Home\n");
        RegCloseKey(hkjs);
        return NULL;
    }
    RegCloseKey(hkjs);
    procrun_addenv("JAVA_HOME", jbin, 0, proc);
    strcat(jbin, "\\bin");
    return pool_strdup(proc->pool, jbin);
} 

/* Read the service parameters from the registry
 *
 */
static int procrun_service_params(process_t *proc)
{
    HKEY key;
    char skey[256];
    char kval[MAX_PATH];
    unsigned long klen;
    DWORD err;
    
    if (!proc->service.name)
        return 0;

    sprintf(skey, PROCRUN_REGKEY_RPARAMS, proc->service.name);
    DBPRINTF1("getting key: %s\n", skey);
    if ((err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, skey,
                            0, KEY_READ, &key)) == ERROR_SUCCESS) {

        klen = MAX_PATH;
        if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_DESCRIPTION, NULL, NULL, 
                                   (unsigned char *)kval,
                                   &klen)) == ERROR_SUCCESS) {
            proc->service.description = pool_strdup(proc->pool, kval);
        }
        klen = MAX_PATH;
        if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_DISPLAY, NULL, NULL, 
                                   (unsigned char *)kval,
                                   &klen)) == ERROR_SUCCESS) {
            proc->service.display = pool_strdup(proc->pool, kval);
        }
        klen = MAX_PATH;
        if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_WORKPATH, NULL, NULL, 
                                   (unsigned char *)kval,
                                   &klen)) == ERROR_SUCCESS) {
            proc->service.path = pool_strdup(proc->pool, kval);
        }
        if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_IMAGE, NULL, NULL, 
                                   NULL,
                                   &klen)) == ERROR_SUCCESS) {
            proc->service.image = (char *)pool_alloc(proc->pool, klen);
            if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_IMAGE, NULL, NULL, 
                                   (unsigned char *)proc->service.image,
                                   &klen)) != ERROR_SUCCESS) {
                proc->service.image = NULL;
            }
        }
        klen = MAX_PATH;
        if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_ACCOUNT, NULL, NULL, 
                                   (unsigned char *)kval,
                                   &klen)) == ERROR_SUCCESS) {
            proc->service.account = pool_strdup(proc->pool, kval);
        }
        klen = MAX_PATH;
        if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_PASSWORD, NULL, NULL, 
                                   (unsigned char *)kval,
                                   &klen)) == ERROR_SUCCESS) {
            proc->service.password = pool_calloc(proc->pool, 256);
            simple_decrypt(100, proc->service.password,  kval);
        }
        klen = MAX_PATH;
        if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_STARTUP, NULL, NULL, 
                                   (unsigned char *)kval,
                                   &klen)) == ERROR_SUCCESS) {
            if (!strcmp(kval, "auto"))
                proc->service.startup = SERVICE_AUTO_START;
            else
                proc->service.startup = SERVICE_DEMAND_START;
        }
        klen = MAX_PATH;
        if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_STARTCLASS, NULL, NULL, 
                                   (unsigned char *)kval,
                                   &klen)) == ERROR_SUCCESS) {
            char *p;
            p = strchr(kval, ';');
            if (p) *p = '\0';
            proc->java.start_class = pool_strdup(proc->pool, kval);
            if (p) {
                ++p;
                proc->java.start_method = pool_strdup(proc->pool, p);
                p = strchr(proc->java.start_method, ';');
                if (p) {
                    *p = '\0';
                    ++p;
                    proc->java.start_param = pool_strdup(proc->pool, p);
                }
            }
        }
        klen = MAX_PATH;
        if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_STOPCLASS, NULL, NULL, 
                                   (unsigned char *)kval,
                                   &klen)) == ERROR_SUCCESS) {
            char *p;
            p = strchr(kval, ';');
            if (p) *p = '\0';
            proc->java.stop_class = pool_strdup(proc->pool, kval);
            if (p) {
                ++p;
                proc->java.stop_method = pool_strdup(proc->pool, p);
                p = strchr(proc->java.stop_method, ';');
                if (p) {
                    *p = '\0';
                    ++p;
                    proc->java.stop_param = pool_strdup(proc->pool, p);
                }
            }
        }
        klen = MAX_PATH;
        if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_STDINFILE, NULL, NULL, 
                                   (unsigned char *)kval,
                                   &klen)) == ERROR_SUCCESS) {
            proc->service.infile = CreateFile(kval,
                                              GENERIC_READ,
                                              0,
                                              NULL,
                                              OPEN_EXISTING,
                                              FILE_ATTRIBUTE_NORMAL,
                                              NULL);
            if (proc->service.infile != INVALID_HANDLE_VALUE) {
                if (proc->h_stdin[1] != INVALID_HANDLE_VALUE)
                    pool_close_handle(proc->pool, proc->h_stdin[1]);
                proc->h_stdin[1] = proc->service.infile;
                proc->service.inname = pool_strdup(proc->pool, kval);
            }
            pool_handle(proc->pool, proc->service.infile);
        }
        klen = MAX_PATH;
        if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_STDOUTFILE, NULL, NULL, 
                                   (unsigned char *)kval,
                                   &klen)) == ERROR_SUCCESS) {
            proc->service.outfile = CreateFile(kval,
                                               GENERIC_WRITE,
                                               FILE_SHARE_READ,
                                               NULL,
                                               OPEN_ALWAYS,
                                               FILE_ATTRIBUTE_NORMAL,
                                               NULL);
            if (proc->service.outfile != INVALID_HANDLE_VALUE) {
                SetFilePointer(proc->service.outfile, 0L, NULL, FILE_END);
                if (proc->h_stdout[1] != INVALID_HANDLE_VALUE)
                    pool_close_handle(proc->pool, proc->h_stdout[1]);
                proc->h_stdout[1] = proc->service.outfile;
                proc->service.outname = pool_strdup(proc->pool, kval);
            }
            pool_handle(proc->pool, proc->service.outfile);
        }
        klen = MAX_PATH;
        if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_STDERRFILE, NULL, NULL, 
                                   (unsigned char *)kval,
                                   &klen)) == ERROR_SUCCESS) {
            proc->service.errfile = CreateFile(kval,
                                               GENERIC_WRITE,
                                               FILE_SHARE_READ,
                                               NULL,
                                               OPEN_ALWAYS,
                                               FILE_ATTRIBUTE_NORMAL,
                                               NULL);
            if (proc->service.errfile != INVALID_HANDLE_VALUE) {
                SetFilePointer(proc->service.errfile, 0L, NULL, FILE_END);
                if (proc->h_stderr[1] != INVALID_HANDLE_VALUE)
                    pool_close_handle(proc->pool, proc->h_stderr[1]);
                proc->h_stderr[1] = proc->service.errfile;
                proc->service.errname = pool_strdup(proc->pool, kval);
            }
            pool_handle(proc->pool, proc->service.errfile);
        }
        if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_CMDARGS, NULL, NULL, 
                                   NULL,
                                   &klen)) == ERROR_SUCCESS) {
            proc->argw = (char *)pool_alloc(proc->pool, klen);
            if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_CMDARGS, NULL, NULL, 
                                       (unsigned char *)proc->argw,
                                       &klen)) != ERROR_SUCCESS) {
                proc->argw = NULL;
            }
            
        }
        if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_JVM_OPTS, NULL, NULL, 
                                   NULL,
                                   &klen)) == ERROR_SUCCESS) {
            proc->java.opts = (char *)pool_alloc(proc->pool, klen);
            if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_JVM_OPTS, NULL, NULL, 
                                       (unsigned char *)proc->java.opts,
                                       &klen)) != ERROR_SUCCESS) {
                proc->java.opts = NULL;
            }
        }
#ifdef PROCRUN_WINAPP
        klen = MAX_PATH;
        if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_WINPOS, NULL, NULL, 
                                   (unsigned char *)kval,
                                   &klen)) == ERROR_SUCCESS) {
            sscanf(kval, "%d %d %d %d", &ac_winpos.left, &ac_winpos.right,
                                        &ac_winpos.top, &ac_winpos.bottom);
        }

        klen = MAX_PATH;
        if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_USELVIEW, NULL, NULL, 
                                   (unsigned char *)kval,
                                   &klen)) == ERROR_SUCCESS) {
            ac_use_lview = atoi(kval);
        }
#endif
        klen = MAX_PATH;
        if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_ENVIRONMENT, NULL, NULL, 
                                   NULL,
                                   &klen)) == ERROR_SUCCESS) {
            proc->service.environment = (char *)pool_alloc(proc->pool, klen);
            if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_ENVIRONMENT, NULL, NULL, 
                                   (unsigned char *)proc->service.environment,
                                   &klen)) != ERROR_SUCCESS) {
                proc->service.environment = NULL;
            }
        }

        RegCloseKey(key); 
        return 0;
    }
    else
        return -1;
}

/* Decide if we need the java
 * Check the registry and decide how the java
 * is going to be loaded.
 * Using inprocess jvm.dll or as a child process running java.exe
 */
static int procrun_load_jvm(process_t *proc, int mode)
{
    int has_java = 0;
    char jvm_path[MAX_PATH + 1];
    
    if (!proc->java.start_class) {
        return 0;
    }
    jvm_path[0] = '\0';
    switch (mode) {
        case PROCRUN_CMD_RUN_JAVA:
            has_java = 1;
            break;
        case PROCRUN_CMD_RUN_SERVICE:
        case PROCRUN_CMD_TEST_SERVICE:
        case PROCRUN_CMD_GUIT_SERVICE:
        case PROCRUN_CMD_GUID_SERVICE:
        case PROCRUN_CMD_EDIT_SERVICE:
            {
                HKEY key;
                char skey[256];
                unsigned long klen = MAX_PATH;
                DWORD err;
                sprintf(skey, PROCRUN_REGKEY_RPARAMS, proc->service.name);
                if ((err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, skey,
                                        0, KEY_READ, &key)) == ERROR_SUCCESS) {
                    
                    if ((err = RegQueryValueEx(key, PROCRUN_PARAMS_JVM,
                                               NULL, NULL, 
                                               (unsigned char *)jvm_path,
                                               &klen)) == ERROR_SUCCESS) {
                        has_java = 1;
                    }
                    RegCloseKey(key); 
                }
            }
            break;
    }
    if (has_java) {
        UINT em;
        if (strlen(jvm_path)) {
            proc->java.display = pool_strdup(proc->pool, jvm_path);
            if (strlen(jvm_path) < 6)
                proc->java.path = procrun_guess_jvm(proc);
            else
                proc->java.path = pool_strdup(proc->pool, jvm_path);
            if (strnicmp(jvm_path, "java", 4) == 0) {
                proc->java.jbin = procrun_guess_java(proc, jvm_path);
                proc->java.jpath = procrun_guess_java_home(proc);
            }
        }
        DBPRINTF1("jvm dll path %s\n", proc->java.path);
        DBPRINTF1("java    path %s\n", proc->java.jpath);
        DBPRINTF1("java    bin  %s\n", proc->java.jbin);
        if (!proc->java.path || !proc->java.start_method) {
            DBPRINTF0("java path or start method missing\n");
            return -1;
        } else if (proc->java.jbin != NULL) {
            DBPRINTF0("forking no need to load java dll\n");
        return 0; // If forking, don't bother with the load.
        }
        /* Try to load the jvm dll */ 
        em = SetErrorMode(SEM_FAILCRITICALERRORS);
        proc->java.dll = LoadLibraryEx(proc->java.path, NULL, 0); 
        if (!proc->java.dll)
            proc->java.dll = LoadLibraryEx(proc->java.path, NULL,
                                           LOAD_WITH_ALTERED_SEARCH_PATH);
        SetErrorMode(em);
        if (!proc->java.dll) {
            DBPRINTF0(NULL);
            DBPRINTF0("Cannot load java dll\n");
            return -1;
        }
        /* resolve symbols */
        jni_JNI_GetDefaultJavaVMInitArgs = (JNI_GETDEFAULTJAVAVMINITARGS)
                                            GetProcAddress(proc->java.dll,
                                            "JNI_GetDefaultJavaVMInitArgs");
        jni_JNI_CreateJavaVM = (JNI_CREATEJAVAVM)
                               GetProcAddress(proc->java.dll,
                               "JNI_CreateJavaVM");
        jni_JNI_GetCreatedJavaVMs = (JNI_GETCREATEDJAVAVMS)
                                    GetProcAddress(proc->java.dll,
                                    "JNI_GetCreatedJavaVMs");
        if (jni_JNI_GetDefaultJavaVMInitArgs == NULL ||
            jni_JNI_CreateJavaVM == NULL ||
            jni_JNI_GetCreatedJavaVMs == NULL) {
            DBPRINTF0(NULL);
            DBPRINTF0("Cannot find JNI routines in java dll\n");
            FreeLibrary(proc->java.dll);
            proc->java.dll = NULL;
            return -1;
        }
        DBPRINTF1("JVM %s Loaded\n", proc->java.path);
    }
    return 0;
}

/* JVM hooks */
static int jni_exit_signaled = 0;
static int jni_exit_code = 0;
static int jni_abort_signaled = 0;

static void jni_exit_hook(int code)
{
    jni_exit_signaled = -1;
#if 1
    jni_abort_signaled = -1;    
#endif
    jni_exit_code = code;

    DBPRINTF1("JVM exit hook called %d\n", code);
}

static void jni_abort_hook()
{
    jni_abort_signaled = -1;    

    DBPRINTF0("JVM abort hook called\n");
}  

/* 'Standard' JNI functions
 *
 */

static JNIEnv *jni_attach(process_t *proc)
{
    JNIEnv *env = NULL;
    int err;
    JavaVM *jvm = proc->java.jvm;

    if (jvm == NULL || jni_abort_signaled)
        return NULL;
    err = (*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_1_2);  
    if (err == 0)
        return env;        
    if (err != JNI_EDETACHED) {
        return NULL;
    }
    err = (*jvm)->AttachCurrentThread(jvm,
                                      (void **)&env,
                                      NULL);
    if (err != 0) {
        return NULL;
    }
    return env;
}

static int jni_detach(process_t *proc)
{
    JavaVM *jvm = proc->java.jvm;

    if (jvm == NULL || jni_abort_signaled)
        return -1;
    return (*jvm)->DetachCurrentThread(jvm);
}

/* Destroy the jvm.
 * This method stops the current jvm calling
 * configured stop methods and destroys the loaded jvm.
 */
static int procrun_destroy_jvm(process_t *proc, HANDLE jh)
{
    JavaVM *jvm = proc->java.jvm;
    int err;
    JNIEnv *env;

    DBPRINTF2("procrun_destroy_jvm dll %08x jvm %08x\n",proc->java.dll,proc->java.jvm);
    if (!proc->java.dll || !jvm) {
        if(proc->java.stop_class != NULL && proc->java.stop_method != NULL && g_env->c->pinfo.dwProcessId) {
            process_t tc = *g_env->c, tm = *g_env->m;
            procrun_t tproc;
            HANDLE threads[2];
            tproc.c = &tc;
            tproc.m = &tm;
            procrun_redirect(proc->service.image,
                              proc->envp, &tproc, 0);    
            threads[0] = tc.pinfo.hThread;
            threads[1] = g_env->c->pinfo.hThread;
            WaitForMultipleObjects(2,threads, TRUE, 60000);
        }

        return 0;
    }
    env = jni_attach(proc);
    if (!env) {
        DBPRINTF0("jni_attach failed\n");
        goto cleanup;
    }
    if (proc->java.stop_bridge && proc->java.stop_mid) {
        jclass strclass;
        jarray jargs = NULL;
        DBPRINTF1("Calling shutdown %s\n", proc->java.stop_class); 

        strclass = (*env)->FindClass(env, "java/lang/String");
        if (proc->java.stop_param) {
            jstring arg = (*env)->NewStringUTF(env, proc->java.stop_param);
            jargs = (*env)->NewObjectArray(env, 1, strclass, NULL);
            (*env)->SetObjectArrayElement(env, jargs, 0, arg);
        }
        (*env)->CallStaticVoidMethod(env,
                                     proc->java.stop_bridge,
                                     proc->java.stop_mid,
                                     jargs);
        if(jh != NULL)
            WaitForSingleObject(jh, 60000);
    }
    else if (!proc->java.jbin) {
        /* Call java.lang.System.exit(0) */
        jclass sysclass;
        jmethodID exitid;
        sysclass = (*env)->FindClass(env, "java/lang/System");
        if (!sysclass)
            goto cleanup;
        exitid   = (*env)->GetStaticMethodID(env, sysclass, "exit", "(I)V");
        if (!exitid)
            goto cleanup;
        report_service_status(SERVICE_STOPPED, 0, 0,
                              g_env->m);
        DBPRINTF0("Forcing shutdown using System.exit(0)\n");
 
        (*env)->CallStaticVoidMethod(env, sysclass, exitid, 0);
    }
cleanup:
    err = (*jvm)->DestroyJavaVM(jvm);
    FreeLibrary(proc->java.dll);
    proc->java.dll = NULL;
    return err;
}

/* Initialize loaded jvm.dll 
 * Pass the startup options to the jvm,
 * and regiter the start/top classes and methods
 */
static int procrun_init_jvm(process_t *proc)
{
    int jvm_version;
    JDK1_1InitArgs vm_args11;
    JavaVMInitArgs vm_args;
    JavaVMOption options[32];
    JNIEnv *env;
    JavaVM *jvm;
    jclass strclass;
    jarray jargs = NULL;
    char *cp;
    char *opts[32];
    int optn, i, err;

    vm_args11.version = JNI_VERSION_1_2;
     
    DBPRINTF0("Initializing JVM\n");
    if (jni_JNI_GetDefaultJavaVMInitArgs(&vm_args11) != 0) {
        DBPRINTF0("Could not find Default InitArgs\n");
        return -1;
    }
    jvm_version= vm_args11.version;

    if (jvm_version != JNI_VERSION_1_2) {
        DBPRINTF1("Found: %X expecting 1.2 Java Version\n", jvm_version);
        return -1;
    }
    
    optn = make_array(proc->java.opts, opts, 30, proc);
    for (i = 0; i < optn; i++)
        options[i].optionString = remove_quotes(opts[i]);
    cp = (char *)pool_alloc(proc->pool, strlen("-Djava.class.path=") +
                            strlen(proc->service.image) + 1);
    strcpy(cp, "-Djava.class.path=");
    strcat(cp, remove_quotes(proc->service.image));
    options[optn++].optionString = cp;
    DBPRINTF1("-Djava.class.path=%s", proc->service.image);
    /* Set the abort and exit hooks */
#if 0
    options[optn].optionString = "exit";
    options[optn++].extraInfo = jni_exit_hook;
#endif
    options[optn].optionString = "abort";
    options[optn++].extraInfo = jni_abort_hook; 

    for (i = 0; i < optn; i++)
    DBPRINTF2("OPT %d %s", i , options[i].optionString);

    vm_args.version = JNI_VERSION_1_2;
    vm_args.options = options;
    vm_args.nOptions = optn;    
    vm_args.ignoreUnrecognized = JNI_TRUE;
 
    err = jni_JNI_CreateJavaVM(&jvm, &env, &vm_args);
    if (err == JNI_EEXIST) {       
        int vmcount;
        
        jni_JNI_GetCreatedJavaVMs(&jvm, 1, &vmcount);        
        if (jvm == NULL) {
            DBPRINTF0("Error creating JVM\n");
            return -1;
        }
    }
    proc->java.jvm = jvm;
    for (i = 0; i < (int)strlen(proc->java.start_class); i++) {
        if (proc->java.start_class[i] == '.')
            proc->java.start_class[i] = '/';
    }
    proc->java.start_bridge = (*env)->FindClass(env, proc->java.start_class);
    if (!proc->java.start_bridge) {
        DBPRINTF1("Couldn't find Startup class %s\n", proc->java.start_class);
        goto cleanup;
    }
    proc->java.start_mid = (*env)->GetStaticMethodID(env, proc->java.start_bridge,
                                                     proc->java.start_method, 
                                                     "([Ljava/lang/String;)V");

    if (!proc->java.start_mid) {
        DBPRINTF1("Couldn't find Startup class method %s\n", proc->java.start_method);
        goto cleanup;
    }
    if (proc->java.stop_class && proc->java.stop_method) {
        for (i = 0; i < (int)strlen(proc->java.stop_class); i++) {
            if (proc->java.stop_class[i] == '.')
                proc->java.stop_class[i] = '/';
        }
        proc->java.stop_bridge = (*env)->FindClass(env, proc->java.stop_class);
        if (!proc->java.stop_bridge) {
            goto cleanup;
        }    
        proc->java.stop_mid = (*env)->GetStaticMethodID(env, proc->java.stop_bridge,
                                                        proc->java.stop_method, 
                                                        "([Ljava/lang/String;)V");
    }
    
    /* check if we have java.exe as worker process 
     * in that case don't call the startup class. 
     */
    if (proc->java.jbin != NULL) {
        return 0;
    } 
    strclass = (*env)->FindClass(env, "java/lang/String");
    if (proc->java.start_param) {
        jstring arg = (*env)->NewStringUTF(env, proc->java.start_param);
        jargs = (*env)->NewObjectArray(env, 1, strclass, NULL);
        (*env)->SetObjectArrayElement(env, jargs, 0, arg);
    }
    (*env)->CallStaticVoidMethod(env,
                                 proc->java.start_bridge,
                                 proc->java.start_mid,
                                 jargs);

    DBPRINTF1("JVM Main class %s finished\n", proc->java.start_class);
    return 0;
cleanup:
    if (proc->java.jvm) {
        (*(proc->java.jvm))->DestroyJavaVM(proc->java.jvm);
        FreeLibrary(proc->java.dll);
        proc->java.dll = NULL;
        proc->java.jvm = NULL;
    }
    return -1;
}

/* Thread that waits for child process to exit
 * When the child process exits, it sets the
 * event so that we can exit
 */
DWORD WINAPI wait_thread(LPVOID param)
{
    procrun_t *env = (procrun_t *)param;

    /* Wait util a process has finished its initialization. */
    WaitForInputIdle(env->c->pinfo.hProcess, INFINITE);
    WaitForSingleObject(env->c->pinfo.hThread, INFINITE);
    pool_close_handle(env->c->pool, env->c->pinfo.hThread);
    env->c->pinfo.hThread = NULL;
    env->c->pinfo.dwProcessId = 0;
    SetEvent(env->m->events[1]);

    return 0;
}  

/* Redirected stdout reader thread
 * It reads char at a time and writes
 * either to stdout handle (file or pipe)
 * or calls the gui console printer function.
 */
DWORD WINAPI stdout_thread(LPVOID param)
{
    unsigned char  ch;
    DWORD readed, written;
    procrun_t *env = (procrun_t *)param;
#ifdef PROCRUN_WINAPP
    static unsigned char buff[MAX_PATH + 1];
    int n = 0;
#endif

    while (env->c->h_stdout[3] && 
           (ReadFile(env->c->h_stdout[3], &ch, 1, &readed, NULL) == TRUE))  {
        if (readed) {
#ifdef PROCRUN_WINAPP
            if (ac_use_dlg) {
                if (ch == '\n' || n >= MAX_PATH) {
                    buff[n] = '\0';
                    DBPRINTF1("RD %s", buff);
                    ac_add_list_string(buff, n, 0);
                    n  = 0;
                } 
                else if (ch == '\t' && n < (MAX_PATH - 4)) {
                    int i; /* replace the TAB with four spaces */
                    for (i = 0; i < 4; ++i)
                        buff[n++] = ' ';
                }
                else if (ch != '\r') /* skip the CR and BELL */
                    buff[n++] = ch;
                else if (ch != '\b')
                    buff[n++] = ' ';
                SwitchToThread();
            }
#endif
            if (WriteFile(env->m->h_stdout[0], &ch, 1, &written, NULL) == TRUE) {
                SwitchToThread();
            }
            else
                break;
            readed = 0;
        }
    }
    /* The client has closed it side of a pipe
     * meaning that he has finished
     */
    SetEvent(env->m->events[2]);
    return 0;
}  

/* Redirected stderr reader thread
 * It reads char at a time and writes
 * either to stderr handle (file or pipe)
 * or calls the gui console printer function.
 */

DWORD WINAPI stderr_thread(LPVOID param)
{
    unsigned char  ch;
    DWORD readed, written;
    procrun_t *env = (procrun_t *)param;
#ifdef PROCRUN_WINAPP
    static unsigned char buff[MAX_PATH + 1];
    int n = 0;
#endif

    while (env->c->h_stderr[3] && 
          (ReadFile(env->c->h_stderr[3], &ch, 1, &readed, NULL) == TRUE))  {
        if (readed) {
#ifdef PROCRUN_WINAPP
            if (ac_use_dlg) {
                if (ch == '\n' || n >= MAX_PATH) {
                    buff[n] = '\0';
                    DBPRINTF1("RD %s", buff);
                    ac_add_list_string(buff, n, 1);
                    n  = 0;
                } 
                else if (ch == '\t' && n < (MAX_PATH - 4)) {
                    int i;
                    for (i = 0; i < 4; ++i)
                        buff[n++] = ' ';
                }
                else if (ch != '\r')
                    buff[n++] = ch;
                else if (ch != '\b')
                    buff[n++] = ' ';
                SwitchToThread();
            }
#endif
            if (WriteFile(env->m->h_stderr[0], &ch, 1, &written, NULL) == TRUE) {
                SwitchToThread();
            }
            else
                break;
            readed = 0;
        }
    }
    SetEvent(env->m->events[3]);
    return 0;
}  

/* Created redirection pipes, and close the unused sides.
 * 
 */
static int procrun_create_pipes(procrun_t *env)
{
    SECURITY_ATTRIBUTES sa;    
   
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    /* redirect stdout */
    if (env->m->h_stdout[1] == INVALID_HANDLE_VALUE)
        env->m->h_stdout[0] = GetStdHandle(STD_OUTPUT_HANDLE);
    else
        env->m->h_stdout[0] = env->m->h_stdout[1];

    if (!CreatePipe(&env->c->h_stdout[0],
                    &env->c->h_stdout[1], &sa, 0)) {
        DBPRINTF0(NULL);
        return -1;
    }
    SetStdHandle(STD_OUTPUT_HANDLE, env->c->h_stdout[1]);
    pool_handle(env->c->pool, env->c->h_stdout[1]);

    if (!DuplicateHandle(env->m->pinfo.hProcess,
                         env->c->h_stdout[0],
                         env->m->pinfo.hProcess,
                         &env->c->h_stdout[3], 
                         0, FALSE, DUPLICATE_SAME_ACCESS)) {
        DBPRINTF0(NULL);
        return -1;
    }
    pool_close_handle(env->c->pool, env->c->h_stdout[0]);
    pool_handle(env->c->pool, env->c->h_stdout[3]);
    
    /* redirect stderr */
    if (env->m->h_stderr[1] == INVALID_HANDLE_VALUE)
        env->m->h_stderr[0] = GetStdHandle(STD_ERROR_HANDLE);
    else
        env->m->h_stderr[0] = env->m->h_stderr[1];

    if (!CreatePipe(&env->c->h_stderr[0],
                    &env->c->h_stderr[1], &sa, 0)) {
        DBPRINTF0(NULL);
        return -1;
    }
    SetStdHandle(STD_ERROR_HANDLE, env->c->h_stderr[1]);
    pool_handle(env->c->pool, env->c->h_stderr[1]);

    if (!DuplicateHandle(env->m->pinfo.hProcess,
                         env->c->h_stderr[0],
                         env->m->pinfo.hProcess,
                         &env->c->h_stderr[3], 
                         0, FALSE, DUPLICATE_SAME_ACCESS)) {
        DBPRINTF0(NULL);
        return -1;
    }
    pool_close_handle(env->c->pool, env->c->h_stderr[0]);
    pool_handle(env->c->pool, env->c->h_stderr[3]);

    /* redirect stdin */
    if (env->m->h_stdin[1] == INVALID_HANDLE_VALUE)
        env->m->h_stdin[0] = GetStdHandle(STD_INPUT_HANDLE);
    else
        env->m->h_stdin[0] = env->m->h_stdin[1];

    if (!CreatePipe(&env->c->h_stdin[0],
                    &env->c->h_stdin[1], &sa, 0)) {
        DBPRINTF0(NULL);
        return -1;
    }
    SetStdHandle(STD_INPUT_HANDLE, env->c->h_stdin[0]);  
    pool_handle(env->c->pool, env->c->h_stdin[0]);
    if (!DuplicateHandle(env->m->pinfo.hProcess,
                         env->c->h_stdin[1],
                         env->m->pinfo.hProcess,
                         &env->c->h_stdin[3], 
                         0, FALSE, DUPLICATE_SAME_ACCESS)) {
        DBPRINTF0(NULL);
        return -1;
    }
    
    pool_close_handle(env->c->pool, env->c->h_stdin[1]);
    pool_handle(env->c->pool, env->c->h_stdin[3]);

    return 0;
}

/* Write the specified file to the childs stdin.
 * This function wraps 'child.exe <some_file'
 */
static int procrun_write_stdin(procrun_t *env)
{
    DWORD readed, written; 
    char buf[PROC_BUFSIZE]; 
    
    if (!env->m->service.infile)
        return -1;

    for (;;) { 
        if (!ReadFile(env->m->service.infile, buf, PROC_BUFSIZE, 
                      &readed, NULL) || readed == 0) 
             break; 
        SwitchToThread();
        if (!WriteFile(env->c->h_stdin[3], buf, readed, 
            &written, NULL))
            break;
        SwitchToThread();
    } 
 
    /* Close the pipe handle so the child process stops reading. */ 
    if (!pool_close_handle(env->c->pool, env->c->h_stdin[3])) 
      return -1; 
    env->c->h_stdin[3] = INVALID_HANDLE_VALUE;
    return 0;
}

/* Make the command line for starting or stopping
 * java process.
 */
static char * set_command_line(procrun_t *env, char *program, int starting){
        int i, j, len = strlen(env->m->argw) + 8192;
        char *opts[64], *nargw;
        char *javaClass = starting ? env->m->java.start_class : env->m->java.stop_class,
            *javaParam = starting ? env->m->java.start_param : env->m->java.stop_param;

        j = make_array(env->m->java.opts, opts, 60, env->m);

        for (i = 0; i < j; i++)
            len += strlen(opts[i]);
       
        nargw = pool_calloc(env->m->pool, len);
        if(starting)
            strcpy(nargw, env->m->argw);
        else
            strcpy(nargw, "java");
        strcat(nargw, " ");
        for (i = 0; i < j; i++) {
            strcat(nargw, opts[i]);
            strcat(nargw, " ");
        }
        strcat(nargw, "-Djava.class.path=");
        if (strchr(program, ' ')) {
            strcat(nargw, "\"");
            strcat(nargw, program);
            strcat(nargw, "\"");
        }
        else
            strcat(nargw, program);
        strcat(nargw, " ");
        strcat(nargw, javaClass);
        if (javaParam) {
            strcat(nargw, " ");
            strcat(nargw, javaParam);
        }
        env->m->argw = nargw;
        program = env->m->java.jbin;
    return program;
}

/* Main redirection function.
 * Create the redirection pipes
 * Make the child's environment
 * Make the command line
 * Logon as different user
 * Create the child process
 */
int procrun_redirect(char *program, char **envp, procrun_t *env, int starting)
{
    STARTUPINFO si;
    DWORD  id;

    if (!program) {
#ifdef PROCRUN_WINAPP
        MessageBox(NULL, "Service not found ", env->m->service.name,
                         MB_OK | MB_ICONERROR);
#else
        fprintf(stderr, "Service not found %s\n", env->m->service.name); 
#endif
        return -1;
    }
    memset(&si, 0, sizeof(STARTUPINFO));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    if(starting) {
        if (procrun_create_pipes(env)) {
            DBPRINTF0("Create pipe failed.\n");
            return -1;
        }
        si.hStdOutput = env->c->h_stdout[1];
        si.hStdError  = env->c->h_stderr[1];
        si.hStdInput  = env->c->h_stdin[0]; 
    }
    else
        si.dwFlags = STARTF_USESHOWWINDOW;
    
    env->m->envw = make_environment(env->c->env, envp, env->m);
    DBPRINTF1("Creating process %s.\n", program);
    DBPRINTF1("Creating process %s.\n", env->m->argw);
    /* for java.exe merge Arguments and Java options */
    if (env->m->java.jbin) {
      program = set_command_line(env, program, starting);
    }
    DBPRINTF2("RUN [%s] %s\n", program, env->m->argw);
    if (env->m->service.account && env->m->service.password && starting) {
        /* Run the child process under a different user account */
        HANDLE user, token;
        DBPRINTF2("RUNASUSER %s@%s\n", env->m->service.account, env->m->service.password);
        if (!LogonUser(env->m->service.account, 
                       NULL, 
                       env->m->service.password, 
                       LOGON32_LOGON_SERVICE,
                       LOGON32_PROVIDER_DEFAULT,
                       &user)) {
            DBPRINTF0(NULL);
            DBPRINTF0("LogonUser failed\n");
            return -1;
        }

        DuplicateTokenEx(user, 
                         TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY, 
                         NULL,
                         SecurityImpersonation,
                         TokenPrimary,
                         &token);
        DBPRINTF0(NULL);

        DBPRINTF2("Launching as %s:%s", env->m->service.account, env->m->service.password);
        ImpersonateLoggedOnUser(token);
        DBPRINTF0(NULL);
        si.lpDesktop = (LPSTR) "Winsta0\\Default";
        if (!CreateProcessAsUser(token,
                                 program,
                                 env->m->argw,
                                 NULL,
                                 NULL,
                                 TRUE,
                                 CREATE_SUSPENDED | CREATE_NEW_CONSOLE | 
                                 CREATE_NEW_PROCESS_GROUP,
                                 env->m->envw,
                                 env->m->service.path,
                                 &si,
                                 &env->c->pinfo)) {

            DBPRINTF1("Error redirecting '%s'\n", program);
            DBPRINTF0(NULL);
            return -1;
        }
    }
    else {
        if (!CreateProcess(program,
                           env->m->argw,
                           NULL,
                           NULL,
                           TRUE,
                           CREATE_SUSPENDED | CREATE_NEW_CONSOLE | 
                           CREATE_NEW_PROCESS_GROUP,
                           env->m->envw,
                           env->m->service.path,
                           &si,
                           &env->c->pinfo)) {
        
            DBPRINTF1("Error redirecting '%s'\n", program);
            DBPRINTF0(NULL);
            return -1;
        }
    }
    DBPRINTF1("started child thread %08x",env->c->pinfo.hThread);
    if(starting) {
        pool_handle(env->c->pool, env->c->pinfo.hThread);
        pool_handle(env->c->pool, env->c->pinfo.hProcess);

        SetStdHandle(STD_OUTPUT_HANDLE, env->m->h_stdout[0]);
        SetStdHandle(STD_ERROR_HANDLE, env->m->h_stderr[0]);
        SetStdHandle(STD_INPUT_HANDLE, env->m->h_stdin[0]);  
        pool_close_handle(env->c->pool, env->c->h_stdout[1]);
        pool_close_handle(env->c->pool, env->c->h_stderr[1]);
        pool_close_handle(env->c->pool, env->c->h_stdin[0]); 

        CloseHandle(CreateThread(NULL, 0, stdout_thread, env, 0, &id));     
        CloseHandle(CreateThread(NULL, 0, stderr_thread, env, 0, &id));     
        ResumeThread(env->c->pinfo.hThread);    
        CloseHandle(CreateThread(NULL, 0, wait_thread, env, 0, &id));     

        procrun_write_stdin(env);
    }
    else
        ResumeThread(env->c->pinfo.hThread);        
    return 0;
}

/* Delete the value from the registry
 */
static int del_service_param(process_t *proc, const char *name)
{
    HKEY key;
    char skey[256];
    DWORD err, c;

    sprintf(skey, PROCRUN_REGKEY_RPARAMS, proc->service.name);
    if ((err = RegCreateKeyEx(HKEY_LOCAL_MACHINE, skey, 0, NULL,
        0, KEY_ALL_ACCESS,
        NULL, &key, &c)) != ERROR_SUCCESS) {
        DBPRINTF0(NULL);
        DBPRINTF2("Failed Opening [%s] [%d]\n", skey, err);
        return -1;
    }

    err = RegDeleteValue(key, name);
    RegCloseKey(key); 
    return (err != ERROR_SUCCESS);

}

/* Update or create the value in the registry
 *
 */
static int set_service_param(process_t *proc, const char *name,
                             const char *value, int len, int service)
{
    HKEY key;
    char skey[256];
    DWORD err;
    DWORD c, type = REG_SZ;


    if (service == 1) {
        sprintf(skey, PROCRUN_REGKEY_SERVICES, proc->service.name);
        if ((err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, skey,
                            0, KEY_SET_VALUE, &key)) != ERROR_SUCCESS) {
        DBPRINTF2("Failed Creating [%s] name [%s]\n", skey, name);
            return -1;
        }
    }
    else {
        sprintf(skey, PROCRUN_REGKEY_RPARAMS, proc->service.name);
        if ((err = RegCreateKeyEx(HKEY_LOCAL_MACHINE, skey, 0, NULL,
                              0, KEY_SET_VALUE,
                              NULL, &key, &c)) != ERROR_SUCCESS) {
        DBPRINTF0(NULL);
        DBPRINTF2("Failed Creating [%s] [%d]\n", skey, err);
            return -1;
        }
    }
    DBPRINTF2("Creating [%s] name  [%s]\n", skey, name);
    DBPRINTF2("Creating [%s] value [%s]\n", skey, value);
    if (value) {
        if (service == 2)
            type = REG_BINARY;
        else if (len > 0)
            type = REG_MULTI_SZ;
        else
            len = strlen(value);
        err = RegSetValueEx(key, name, 0, type,
            (unsigned char *)value, len);
    }
    else 
        err = RegSetValueEx(key, name, 0, REG_DWORD,
        (unsigned char *)&len, sizeof(int));

    RegCloseKey(key); 
    return (err != ERROR_SUCCESS);
}
static const char *location_jvm_default[] = {
    "\\jre\\bin\\classic\\jvm.dll",           /* Sun JDK 1.3 */
    "\\bin\\classic\\jvm.dll",                /* Sun JRE 1.3 */
    "\\jre\\bin\\client\\jvm.dll",            /* Sun JDK 1.4 */
    "\\bin\\client\\jvm.dll",                 /* Sun JRE 1.4 */
    NULL,
};
  
/* 
 * Attempt to locate the jvm from the installation.
 */
static char *procrun_find_java(process_t *proc, char *jhome)
{
  char path[MAX_PATH+1];
  int x = 0;
  HMODULE hm;
  for(x=0; location_jvm_default[x] != NULL; x++) {
      strcpy(path,jhome);
      strcat(path,location_jvm_default[x]);
      hm = LoadLibraryEx(path, NULL, 0); 
      if(hm != NULL) {
          DBPRINTF1("Found library at %s\n",path);
          FreeLibrary(hm);
          return pool_strdup(proc->pool, path);
      }
  }
  return NULL;
}
   
/*
 * Process the arguments and fill the process_t stuct.
 */
static int process_args(process_t *proc, int argc, char **argv,
                        char **java, char *path)
{
    int arglen = 0;
    char *argp;
    int i,n, sp;

    /* parse command line */
    *java = NULL;
    if (!GetModuleFileName(NULL, path, MAX_PATH - 
                           strlen(proc->service.name) - 7)) {
        DBPRINTF0("GetModuleFileName failed\n");
        return -1;
    } 
    strcat(path, " " PROC_ARG_RUN_SERVICE);
    sp = strchr(proc->service.name, ' ') != NULL;
    if(sp) {
        strcat(path,"\"");
    }
    strcat(path, proc->service.name); 
    if(sp) {
        strcat(path,"\"");
    }
    for (i = 2; i < argc; i++) {
        DBPRINTF2("Parsing %d [%s]\n", i, argv[i]);
        if (strlen(argv[i]) > 2 && argv[i][0] == '-' && argv[i][1] == '-') {
            argp = &argv[i][2];
            if (STRNI_COMPARE(argp, PROCRUN_PARAMS_IMAGE))
                proc->service.image = pool_strdup(proc->pool, argv[++i]);
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_DESCRIPTION))
                proc->service.description = pool_strdup(proc->pool, argv[++i]);
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_DISPLAY))
                proc->service.display = argv[++i];
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_WORKPATH))
                proc->service.path = pool_strdup(proc->pool, argv[++i]);
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_JVM_OPTS)) {
                proc->java.opts = pool_calloc(proc->pool, strlen(argv[++i]) + 2);
                strcpy(proc->java.opts, argv[i]);
            }
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_JVM))
                *java = argv[++i];
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_STDINFILE))
                proc->service.inname = pool_strdup(proc->pool, argv[++i]);
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_STDOUTFILE))
                proc->service.outname = pool_strdup(proc->pool, argv[++i]);
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_STDERRFILE))
                proc->service.errname = pool_strdup(proc->pool, argv[++i]);
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_STARTCLASS))
                proc->java.start_class = pool_strdup(proc->pool, argv[++i]);
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_STOPCLASS))
                proc->java.stop_class = pool_strdup(proc->pool, argv[++i]);
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_STARTUP)) {
                ++i;
                if (stricmp(argv[i], "auto") == 0)
                    proc->service.startup = SERVICE_AUTO_START;
                else if (stricmp(argv[i], "manual") == 0)
                    proc->service.startup = SERVICE_DEMAND_START;
            }
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_ACCOUNT))
                proc->service.account = pool_strdup(proc->pool, argv[++i]);
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_PASSWORD))
                proc->service.password = pool_strdup(proc->pool, argv[++i]);
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_INSTALL)) {
                strcpy(path, argv[++i]); 
                strcat(path, " " PROC_ARG_RUN_SERVICE);
                if(sp) {
                    strcat(path,"\"");
                }
                strcat(path, proc->service.name);
                if(sp) {
                    strcat(path, "\"");
                }
            }
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_ENVIRONMENT)) {
              proc->service.environment = pool_strdup(proc->pool, argv[++i]);
            }
            else {
                DBPRINTF1("Unrecognized option %s\n", argv[i]);
                break;
            }
        }
        else {
           DBPRINTF1("Not an Command option option %s\n", argv[i]);
           break; 
        }
    }
    if (*java && !strnicmp(*java, "java", 4)) {
        arglen = strlen(*java) + 1;
    } else if(*java) {
        int jlen = strlen(*java) +1;
        if(stricmp(*java+(jlen-5),".dll")) {
            /* Assume it is the java installation */
            char *njava = procrun_find_java(proc, *java);
            DBPRINTF1("Attempting to locate jvm from %s\n",*java);
            if(njava != NULL) {
              *java = njava;
            }
        }
    } else if (proc->service.name)
        arglen = strlen(proc->service.name) + 1;
    for (n = i; n < argc; n++) {
        arglen += (strlen(argv[n]) + 1);
        if (strchr(argv[n], ' '))
            arglen += 2;
    }
    if (arglen) {
        ++arglen;
        proc->argw = (char *)pool_calloc(proc->pool, arglen);
        if (*java && !strnicmp(*java, "java", 4)) 
            strcpy(proc->argw, *java);
        else 
            strcpy(proc->argw, proc->service.name);
        for (n = i; n < argc; n++) {
            strcat(proc->argw, " ");
            if (strchr(argv[n], ' ')) {
                strcat(proc->argw, "\"");
                strcat(proc->argw, argv[n]);
                strcat(proc->argw, "\"");
            }
            else
                strcat(proc->argw, argv[n]);
            DBPRINTF1("Adding cmdline %s\n", argv[n]);
        }
    }
    if (!proc->service.startup)
        proc->service.startup = SERVICE_AUTO_START;
    return 0;
}

/*
 * Save the parameters in registry
 */
void save_service_params(process_t *proc, char *java)
{
    int i;

    if (proc->argw)
        set_service_param(proc, PROCRUN_PARAMS_CMDARGS, proc->argw, 0, 0);
    if (proc->service.description) {
        set_service_param(proc, PROCRUN_PARAMS_DESCRIPTION,
                          proc->service.description, 0, 0);
        set_service_param(proc, PROCRUN_PARAMS_DESCRIPTION,
                          proc->service.description, 0, 1);
    }
    if (proc->service.display) {
        set_service_param(proc, PROCRUN_PARAMS_DISPLAY,
                          proc->service.display, 0, 0);
        set_service_param(proc, PROCRUN_PARAMS_DISPLAY,
                          proc->service.display, 0, 1);
    }
    if (proc->service.image)
        set_service_param(proc, PROCRUN_PARAMS_IMAGE,
                          proc->service.image, 0, 0);
    if (proc->service.path)
        set_service_param(proc, PROCRUN_PARAMS_WORKPATH,
                          proc->service.path, 0, 0);
    if (proc->service.inname)
        set_service_param(proc, PROCRUN_PARAMS_STDINFILE,
                          proc->service.inname, 0, 0);
    if (proc->service.outname)
        set_service_param(proc, PROCRUN_PARAMS_STDOUTFILE,
                          proc->service.outname, 0, 0);
    if (proc->service.errname)
        set_service_param(proc, PROCRUN_PARAMS_STDERRFILE,
                          proc->service.errname, 0, 0);
    if (java)
        set_service_param(proc, PROCRUN_PARAMS_JVM,
                          java, 0, 0);
    if (proc->java.start_class)
        set_service_param(proc, PROCRUN_PARAMS_STARTCLASS,
                          proc->java.start_class, 0, 0);
    if (proc->java.stop_class)
        set_service_param(proc, PROCRUN_PARAMS_STOPCLASS,
                          proc->java.stop_class, 0, 0);
    /* Account and password allways comes as pair */
    if (proc->service.account && proc->service.password) {
        if (*proc->service.account == '-') {
            del_service_param(proc, PROCRUN_PARAMS_ACCOUNT);
            del_service_param(proc, PROCRUN_PARAMS_PASSWORD);
        }
        else {
            unsigned char b[256];
            set_service_param(proc, PROCRUN_PARAMS_ACCOUNT,
                              proc->service.account, 0, 0);
            simple_encrypt(100, proc->service.password, b);
            set_service_param(proc, PROCRUN_PARAMS_PASSWORD,
                              b, 256, 2);
        }
        
    }
    if (proc->service.environment) {
        int l = strlen(proc->service.environment);
        for(i=0; i < l; i++) {
            if(proc->service.environment[i] == '#')
                proc->service.environment[i] = '\0';
        }
        set_service_param(proc, PROCRUN_PARAMS_ENVIRONMENT,
                        proc->service.environment, l+2, 0);
    }


    if (proc->service.startup != SERVICE_NO_CHANGE)
        set_service_param(proc, PROCRUN_PARAMS_STARTUP,
            proc->service.startup == SERVICE_AUTO_START ? "auto" : "manual",
            0, 0);

    if (proc->java.opts) {
        int l = strlen(proc->java.opts);
        /* change the string to zero separated for MULTI_SZ */
        for (i = 0; i < l; i ++) {
            if (proc->java.opts[i] == '#')
                proc->java.opts[i] = '\0';
        }
        set_service_param(proc, PROCRUN_PARAMS_JVM_OPTS,
                          proc->java.opts, l + 2, 0);
    }
}

/* Install the service
 */
static int procrun_install_service(process_t *proc, int argc, char **argv)
{
    SC_HANDLE service;
    SC_HANDLE manager;
    char path[MAX_PATH+1] = {0};
    char *java = NULL;

    if (!proc->service.name) {
        return -1;
    }

    if (process_args(proc, argc, argv, &java, path)) {
         DBPRINTF0("Installing NT service: process_args failed\n");
         return -1;
    }

    DBPRINTF2("Installing NT service %s %s", path, proc->service.display);

    manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!manager) {    
        return -1;
    }

    service = CreateService(manager, 
                            proc->service.name,
                            proc->service.display,
                            SERVICE_ALL_ACCESS,
                            SERVICE_WIN32_OWN_PROCESS,
                            proc->service.startup,
                            SERVICE_ERROR_NORMAL,
                            path,
                            NULL,
                            NULL,
                            SERVICE_DEPENDENCIES,
                            NULL,
                            NULL);
    if (service) {
        CloseServiceHandle(service);
        CloseServiceHandle(manager);
    }
    else {
        DBPRINTF0("CreateService failed\n");
        CloseServiceHandle(manager);
        return -1;
    }

    /* Save parameters in registry */
    save_service_params(proc,java);

    DBPRINTF0("NT service installed succesfully\n");
    SetEvent(proc->events[0]);
    return 0;
}

/* Install service on win9x */
static int procrun_install_service9x(process_t *proc, int argc, char **argv)
{
    HKEY        hkey;
    DWORD rv;
    char szPath[MAX_PATH+1] = {0};

    char path[MAX_PATH+1];
    char *display = NULL;
    char *java = NULL;

    DBPRINTF0( "InstallSvc for non-NT\r\n");

    if (!proc->service.name) {
        return -1;
    }

    rv = RegCreateKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows"
              "\\CurrentVersion\\RunServices", &hkey);
    if (rv != ERROR_SUCCESS) {
        DBPRINTF0( "Could not open the RunServices registry key\r\n");
        return -1;
    }

    if (process_args(proc, argc, argv, &java, path)) {
         DBPRINTF0("Installing service: process_args failed\n");
         return -1;
    }

    DBPRINTF1("Installing service %s\n", path);

    rv = RegSetValueEx(hkey, proc->service.name, 0, REG_SZ,
               (unsigned char *) path,
               strlen(path) + 1);
    RegCloseKey(hkey);
    if (rv != ERROR_SUCCESS) {
        DBPRINTF2( "Could not add %s:%s to RunServices Registry Key\r\n",
                  proc->service.name, path);
        return -1;
    }

    strcpy(szPath,
         "SYSTEM\\CurrentControlSet\\Services\\");
    strcat(szPath,proc->service.name);
    rv = RegCreateKey(HKEY_LOCAL_MACHINE, szPath, &hkey);
    if (rv != ERROR_SUCCESS) {
        DBPRINTF1( "Could not create/open the %s registry key\r\n",
            szPath);
        return -1;
    }
    rv = RegSetValueEx(hkey, "ImagePath", 0, REG_SZ,
               (unsigned char *) path,
               strlen(path) + 1);
    if (rv != ERROR_SUCCESS) {
        RegCloseKey(hkey);
        DBPRINTF0( "Could not add ImagePath to our Registry Key\r\n");
        return -1;
    }
    rv = RegSetValueEx(hkey, "DisplayName", 0, REG_SZ,
               (unsigned char *) proc->service.display,
               strlen(proc->service.display) + 1);
    RegCloseKey(hkey);
    if (rv != ERROR_SUCCESS) {
        DBPRINTF0( "Could not add DisplayName to our Registry Key\r\n");
        return -1;
    }

    /* Save parameters in registry */
    save_service_params(proc,java);

    DBPRINTF0("service installed succesfully\n");
    SetEvent(proc->events[0]);
    return 0;
}

/* Update the service parameters
 * This is the main parser for //US//
 */
int procrun_update_service(process_t *proc, int argc, char **argv)
{
    SC_HANDLE service;
    SC_HANDLE manager;
    char *argp;
    char path[MAX_PATH+1];
    char *java = NULL;
    int arglen = 0, sp;

    int i, n;
    if (!proc->service.name) {
        return -1;
    }
    if (!GetModuleFileName(NULL, path, MAX_PATH - 
                           strlen(proc->service.name) - 7)) {
        DBPRINTF0(NULL);
        return -1;
    }
    sp = strchr(proc->service.name, ' ') != NULL;
    strcat(path, " " PROC_ARG_RUN_SERVICE);
    if(sp) {
        strcat(path,"\"");
    }
    strcat(path, proc->service.name);
    if(sp) {
        strcat(path, "\"");
    }
    DBPRINTF1("Updating service %s\n", path);

    manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!manager) {    
        DBPRINTF0(NULL);
        return -1;
    }
    service = OpenService(manager, proc->service.name, SERVICE_ALL_ACCESS);
    if (!service) {
        DBPRINTF0(NULL);
        CloseServiceHandle(manager);
        return -1;
    }
    proc->service.startup = SERVICE_NO_CHANGE;
    proc->service.image = NULL;
    proc->service.description = NULL;
    proc->service.path = NULL;
    proc->service.inname = NULL;
    proc->service.outname = NULL;
    proc->service.errname = NULL;
    proc->java.start_class = NULL;
    proc->java.stop_class = NULL;
    proc->java.opts = NULL;
    proc->service.account  = NULL;
    proc->service.password = NULL;
    proc->service.display = NULL;
    proc->argw = NULL;
    /* parse command line */
    for (i = 2; i < argc; i++) {
        if (strlen(argv[i]) > 2 && argv[i][0] == '-' && argv[i][1] == '-') {
            argp = &argv[i][2];
            if (STRNI_COMPARE(argp, PROCRUN_PARAMS_IMAGE))
                proc->service.image = pool_strdup(proc->pool, argv[++i]);
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_DESCRIPTION))
                proc->service.description = pool_strdup(proc->pool, argv[++i]);
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_DISPLAY))
                proc->service.display = argv[++i];
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_WORKPATH))
                proc->service.path = pool_strdup(proc->pool, argv[++i]);
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_JVM_OPTS)) {
                proc->java.opts = pool_calloc(proc->pool, strlen(argv[++i]) + 2);
                strcpy(proc->java.opts, argv[i]);
            }
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_JVM))
                java = argv[++i];
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_STDINFILE))
                proc->service.inname = pool_strdup(proc->pool, argv[++i]);
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_STDOUTFILE))
                proc->service.outname = pool_strdup(proc->pool, argv[++i]);
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_STDERRFILE))
                proc->service.errname = pool_strdup(proc->pool, argv[++i]);
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_STARTCLASS))
                proc->java.start_class = pool_strdup(proc->pool, argv[++i]);
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_STOPCLASS))
                proc->java.stop_class = pool_strdup(proc->pool, argv[++i]);
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_STARTUP)) {
                ++i;
                if (stricmp(argv[i], "auto") == 0)
                    proc->service.startup = SERVICE_AUTO_START;
                else if (stricmp(argv[i], "manual") == 0)
                    proc->service.startup = SERVICE_DEMAND_START;
            }
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_ACCOUNT))
                proc->service.account = pool_strdup(proc->pool, argv[++i]);
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_PASSWORD))
                proc->service.password = pool_strdup(proc->pool, argv[++i]);
            else if (STRNI_COMPARE(argp, PROCRUN_PARAMS_ENVIRONMENT))
                proc->service.environment = pool_strdup(proc->pool, argv[++i]);
            else
                break;
        }
        else
            break;
    }

    if (java && strnicmp(java, "java", 4)) {
        int jlen = strlen(java) + 1;
        if(stricmp(java+(jlen-5),".dll")) {
            /* Assume it is the java installation */
            char *njava = procrun_find_java(proc, java);
            DBPRINTF1("Attempting to locate jvm from %s\n",java);
            if(njava != NULL) {
              java = njava;
            }
        }
    }
 
    if (i < argc) {
        if (java && !strnicmp(java, "java", 4))
            arglen = strlen(java) + 1;
        else if (proc->service.name)
            arglen = strlen(proc->service.name) + 1;
        for (n = i; n < argc; n++) {
            arglen += strlen(argv[n]) + 1;
            if (strchr(argv[n], ' '))
                arglen += 2;
        }
        if (arglen) {
            ++arglen;
            proc->argw = (char *)pool_calloc(proc->pool, arglen);
            if (java && !strnicmp(java, "java", 4))
                strcpy(proc->argw, java);
            else
                strcpy(proc->argw, proc->service.name);
            for (n = i; n < argc; n++) {
                strcat(proc->argw, " ");
                if (strchr(argv[n], ' ')) {
                    strcat(proc->argw, "\"");
                    strcat(proc->argw, argv[n]);
                    strcat(proc->argw, "\"");
                }
                else
                    strcat(proc->argw, argv[n]);
                DBPRINTF1("Adding cmdline %s\n", argv[n]);
            }
        }
    }
    ChangeServiceConfig(service,
                        SERVICE_NO_CHANGE,
                        proc->service.startup,
                        SERVICE_NO_CHANGE,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        proc->service.display);

    CloseServiceHandle(service);
    CloseServiceHandle(manager);
    
    save_service_params(proc,java);

    return 0;
}

/* Delete the service
 * Removes the service form registry and SCM.
 * Stop the service if running.
 */
static int procrun_delete_service(process_t *proc)
{
    SC_HANDLE service;
    SC_HANDLE manager;
    SERVICE_STATUS status;
    char skey[MAX_PATH+1];
    DWORD err;

    if (!proc->service.name) {
        return -1;
    }
    DBPRINTF1("Deleting service %s", proc->service.name);

    sprintf(skey, PROCRUN_REGKEY_RSERVICES, proc->service.name);
    if ((err = SHDeleteKey(HKEY_LOCAL_MACHINE, skey)) != ERROR_SUCCESS) {
        DBPRINTF2("Failed Deleting [%s] [%d]\n", skey, err);
    }

    manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!manager) {    
        DBPRINTF0(NULL);
        return -1;
    }
    service = OpenService(manager, proc->service.name, SERVICE_ALL_ACCESS);
    
    if (service) {
        BOOL ss = FALSE;
        QueryServiceStatus(service, &status);
        if (status.dwCurrentState != SERVICE_RUNNING)
            ss = DeleteService(service);
        else {
            /* Stop the service */
            if (ControlService(service, SERVICE_CONTROL_STOP, &status)) {
                Sleep(1000);
                while (QueryServiceStatus(service, &status)) {
                    if (status.dwCurrentState == SERVICE_STOP_PENDING)
                        Sleep(1000);
                    else
                        break;
                }
            }
            QueryServiceStatus(service, &status);
            if (status.dwCurrentState != SERVICE_RUNNING)
                ss = DeleteService(service);
        }
        CloseServiceHandle(service);
        CloseServiceHandle(manager);
        if (!ss)
            return -1;
    }
    else {
        DBPRINTF0(NULL);
        CloseServiceHandle(manager);
        return -1;
    }
    DBPRINTF0("NT service deleted succesfully\n");
    SetEvent(proc->events[0]);
    return 0;
}
 
/* remove service (non NT) stopping it looks ugly!!! so we let it run. */
static int procrun_delete_service9x(process_t *proc)
{
    HKEY hkey;
    DWORD rv;
 
    if (!proc->service.name) {
        return -1;
    }
    DBPRINTF1("Deleting service %s", proc->service.name);

    rv = RegOpenKey(HKEY_LOCAL_MACHINE,
        "Software\\Microsoft\\Windows\\CurrentVersion\\RunServices",
        &hkey);
    if (rv != ERROR_SUCCESS) {
        DBPRINTF0( "Could not open the RunServices registry key.\r\n");
        return -1;
    }
    rv = RegDeleteValue(hkey, proc->service.name);
    RegCloseKey(hkey);
    if (rv != ERROR_SUCCESS)
        DBPRINTF0( "Could not delete the RunServices entry.\r\n");
 
    rv = RegOpenKey(HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Services", &hkey);
    if (rv != ERROR_SUCCESS) {
        DBPRINTF0( "Could not open the Services registry key.\r\n");
        return -1;
    }
    rv = RegDeleteKey(hkey, proc->service.name);
    RegCloseKey(hkey);
    if (rv != ERROR_SUCCESS) {
        DBPRINTF0( "Could not delete the Services registry key.\r\n");
        return -1;
    }
    DBPRINTF0("service deleted succesfull\n");
    SetEvent(proc->events[0]);
    return 0;
}

/* This is the WndProc procedure for our invisible window.
 * When our subclasssed tty window receives the WM_CLOSE, WM_ENDSESSION,
 * or WM_QUERYENDSESSION messages, the message is dispatched to our hidden
 * window (this message process), and we call the installed HandlerRoutine
 * that was registered by the app.
 */
#ifndef ENDSESSION_LOGOFF
#define ENDSESSION_LOGOFF    0x80000000
#endif
static LRESULT CALLBACK ttyConsoleCtrlWndProc(HWND hwnd, UINT msg,
                                              WPARAM wParam, LPARAM lParam)
{

    if (msg == WM_CREATE) {
        DBPRINTF0("ttyConsoleCtrlWndProc WM_CREATE\n");
        return 0;
    } else if (msg == WM_DESTROY) {
        DBPRINTF0("ttyConsoleCtrlWndProc WM_DESTROY\n");
        return 0;
    } else if (msg == WM_CLOSE) {
        /* Call StopService?. */
        DBPRINTF0("ttyConsoleCtrlWndProc WM_CLOSE\n");
        return 0; /* May return 1 if StopService failed. */
    } else if ((msg == WM_QUERYENDSESSION) || (msg == WM_ENDSESSION)) {
        if (lParam & ENDSESSION_LOGOFF) {
            /* Here we have nothing to our hidden windows should stay. */
            DBPRINTF0(TEXT("ttyConsoleCtrlWndProc LOGOFF\n"));
            return(1); /* Otherwise it cancels the logoff */
        } else {
            /* Stop Service. */
            DBPRINTF0("ttyConsoleCtrlWndProc SHUTDOWN\n");
            SetEvent(g_env->m->events[0]);

            /* Wait for the JVM to stop */
            DBPRINTF1("Stopping: main thread %08x\n",g_env->m->pinfo.hThread);
            if (g_env->m->pinfo.hThread != NULL) {
                DBPRINTF0("ttyConsoleCtrlWndProc waiting for main\n");
                WaitForSingleObject(g_env->m->pinfo.hThread, 60000); 
                DBPRINTF0("ttyConsoleCtrlWndProc main thread finished\n");
            }

            return(1); /* Otherwise it cancels the shutdown. */
        }
    }
    return (DefWindowProc(hwnd, msg, wParam, lParam));
}                                                                               
/* ttyConsoleCreateThread is the process that runs within the user app's
 * context.  It creates and pumps the messages of a hidden monitor window,
 * watching for messages from the system, or the associated subclassed tty
 * window.  Things can happen in our context that can't be done from the
 * tty's context, and visa versa, so the subclass procedure and this hidden
 * window work together to make it all happen.
 */
static DWORD WINAPI ttyConsoleCtrlThread(LPVOID param)
{
    HWND monitor_hwnd;
    WNDCLASS wc;
    MSG msg;
    wc.style         = CS_GLOBALCLASS;
    wc.lpfnWndProc   = ttyConsoleCtrlWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 8;
    wc.hInstance     = NULL;
    wc.hIcon         = NULL;
    wc.hCursor       = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = "ApacheJakartaService";
 
    if (!RegisterClass(&wc)) {
        DBPRINTF0("RegisterClass failed\n");
        return 0;
    }
 
    /* Create an invisible window */
    monitor_hwnd = CreateWindow(wc.lpszClassName, 
                                "ApacheJakartaService",
                                WS_OVERLAPPED & ~WS_VISIBLE,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                NULL, NULL,
                                GetModuleHandle(NULL), NULL);
 
    if (!monitor_hwnd) {
        DBPRINTF0("RegisterClass failed\n");
        return 0;
    }
 
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
 
    return 0;
}
/*
 * Register the process to resist logoff and start the thread that will receive
 * the shutdown via a hidden window.
 */
BOOL Windows9xServiceCtrlHandler()
{
    HANDLE hThread;
    DWORD tid;
    HINSTANCE hkernel;
    DWORD (WINAPI *register_service_process)(DWORD, DWORD) = NULL;

    /* If we have not yet done so */
    FreeConsole();

    /* Make sure the process will resist logoff */
    hkernel = LoadLibrary("KERNEL32.DLL");
    if (!hkernel) {
        DBPRINTF0("LoadLibrary KERNEL32.DLL failed\n");
        return 0;
    }
    register_service_process = (DWORD (WINAPI *)(DWORD, DWORD)) 
        GetProcAddress(hkernel, "RegisterServiceProcess");
    if (register_service_process == NULL) {
        DBPRINTF0("dlsym RegisterServiceProcess failed\n");
        return 0;
    }
    if (!register_service_process(0,TRUE)) {
        FreeLibrary(hkernel);
        DBPRINTF0("register_service_process failed\n");
        return 0;
    }
    DBPRINTF0("procrun registered as a service\n");

    /*
     * To be handle notice the shutdown, we need a thread and window.
     */
     hThread = CreateThread(NULL, 0, ttyConsoleCtrlThread,
                               (LPVOID)NULL, 0, &tid);
     if (hThread) {
        CloseHandle(hThread);
        return TRUE;
     }
    DBPRINTF0("jsvc shutdown listener start failed\n");
    return TRUE;
}

/* Report the service status to the SCM
 */
int report_service_status(DWORD dwCurrentState,
                          DWORD dwWin32ExitCode,
                          DWORD dwWaitHint,
                          process_t *proc)
{
   static DWORD dwCheckPoint = 1;
   BOOL fResult = TRUE;

   if (proc->service.mode && proc->service.h_status) {
      if (dwCurrentState == SERVICE_START_PENDING)
         proc->service.status.dwControlsAccepted = 0;
      else
         proc->service.status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

      proc->service.status.dwCurrentState = dwCurrentState;
      proc->service.status.dwWin32ExitCode = dwWin32ExitCode;
      proc->service.status.dwWaitHint = dwWaitHint;

      if ((dwCurrentState == SERVICE_RUNNING) ||
          (dwCurrentState == SERVICE_STOPPED))
         proc->service.status.dwCheckPoint = 0;
      else
        proc->service.status.dwCheckPoint = dwCheckPoint++;
      if (g_is_windows_nt)
         fResult = SetServiceStatus(proc->service.h_status, &proc->service.status);
      if (!fResult) {
         DBPRINTF0("SetServiceStatus Failed\n");
         DBPRINTF0(NULL);
      }
   }
   return fResult;
}

/* Service controll handler
 */
void WINAPI service_ctrl(DWORD dwCtrlCode)
{
    switch (dwCtrlCode) {
        case SERVICE_CONTROL_STOP:
            report_service_status(SERVICE_STOP_PENDING, NO_ERROR, 0,
                                  g_env->m);
            DBPRINTF0("ServiceCtrl STOP\n");
            SetEvent(g_env->m->events[0]);
            return;
        case SERVICE_CONTROL_INTERROGATE:
        break;
        default:
        break;
   }

   report_service_status(g_env->m->service.status.dwCurrentState,
                         NO_ERROR, 0, g_env->m);
}

/* Console controll handler
 * 
 */
BOOL WINAPI console_ctrl(DWORD dwCtrlType)
{
    switch (dwCtrlType) {
        case CTRL_BREAK_EVENT:
            DBPRINTF0("Captured CTRL+BREAK Event\n");
            SetEvent(g_env->m->events[0]);
            return TRUE;
        case CTRL_C_EVENT:
            DBPRINTF0("Captured CTRL+C Event\n");
            SetEvent(g_env->m->events[0]);
            return TRUE;
        case CTRL_CLOSE_EVENT:
            DBPRINTF0("Captured CLOSE Event\n");
            SetEvent(g_env->m->events[0]);
            return TRUE;
        case CTRL_SHUTDOWN_EVENT:
            DBPRINTF0("Captured SHUTDOWN Event\n");
            SetEvent(g_env->m->events[0]);
            return TRUE;
        break;

   }
   return FALSE;
}

/* Main JVM thread
 * Like redirected process, the redirected JVM application
 * is run in the separate thread.
 */
DWORD WINAPI java_thread(LPVOID param)
{
    DWORD rv;
    procrun_t *env = (procrun_t *)param;
    
    if (!param)
        return -1;
    rv = procrun_init_jvm(env->m);
    jni_detach(env->m);

    SetEvent(env->m->events[0]);
    return rv;
}

int service_main(int argc, char **argv)
{
    DWORD fired = 0;
    HANDLE jh;
    int   rv = -1;
    
    
    g_env->m->service.status.dwServiceType      = SERVICE_WIN32_OWN_PROCESS;
    g_env->m->service.status.dwCurrentState     = SERVICE_START_PENDING; 
    g_env->m->service.status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE; 
    g_env->m->service.status.dwWin32ExitCode    = 0; 
    g_env->m->service.status.dwServiceSpecificExitCode = 0; 
    g_env->m->service.status.dwCheckPoint       = 0; 
    g_env->m->service.status.dwWaitHint         = 0; 

    g_env->m->service.status.dwServiceSpecificExitCode = 0;
    if (g_env->m->service.mode && g_is_windows_nt) {
        g_env->m->service.h_status = RegisterServiceCtrlHandler(g_env->m->service.name,
                                                                     service_ctrl);
        if (!g_env->m->service.h_status)
            goto cleanup;
    } else
        g_env->m->service.h_status = 0;

    report_service_status(SERVICE_START_PENDING, NO_ERROR, 3000,
                          g_env->m);
    if (g_env->m->java.dll) {
        if (g_env->m->java.jbin == NULL) {
            DWORD id;
            /* Create the main JVM thread so we don't get blocked */
            jh = CreateThread(NULL, 0, java_thread, g_env, 0, &id);
            pool_handle(g_env->m->pool, jh);
            rv = 0;
        }
        else {
            /* Redirect java or javaw */
            if ((rv = procrun_init_jvm(g_env->m)) == 0) {
                rv = procrun_redirect(g_env->m->service.image,
                                      g_env->m->envp, g_env, 1);    
            }
        }
    } 
    else {
        rv = procrun_redirect(g_env->m->service.image,
                              g_env->m->envp, g_env, 1);    
    }
    if (rv == 0) {
        report_service_status(SERVICE_RUNNING, NO_ERROR, 0,
                              g_env->m);
        DBPRINTF2("Service %s Started %d\n", g_env->m->service.name, rv);

    }
    /* Wait for shutdown event or child exit */
    while (rv == 0) {
        fired = WaitForMultipleObjects(4, g_env->m->events,
                                       FALSE, INFINITE); 
        
        if (fired == WAIT_OBJECT_0 ||
            fired == (WAIT_OBJECT_0 + 1) ||
            fired == WAIT_TIMEOUT)
            break;
    }

cleanup:
    DBPRINTF1("Stoping Service %s\n", g_env->m->service.name);
    report_service_status(SERVICE_STOP_PENDING, NO_ERROR, 3000,
                          g_env->m);
    procrun_destroy_jvm(g_env->m, jh);

    inject_exitprocess(&g_env->c->pinfo);
    report_service_status(SERVICE_STOPPED, 0, 0,
                          g_env->m);

    DBPRINTF0("Setting Shutdown Event 0\n");
    SetEvent(g_env->m->events[0]);
    return rv;
}

/* Procrun main function
 */
int procrun_main(int argc, char **argv, char **envp, procrun_t *env)
{
    int i, mode = 0;
    char event[64];
    DWORD fired;
    int   rv = -1;
    OSVERSIONINFO osver;
    HANDLE handle;
    BOOL isok;

    SERVICE_TABLE_ENTRY dispatch_table[] = {
        {NULL, NULL},
        {NULL, NULL}
    };

   osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&osver)) {
        if (osver.dwPlatformId >= VER_PLATFORM_WIN32_NT)
            g_is_windows_nt = 1;
    }
    DBPRINTF1("OS Version %d\n", g_is_windows_nt);

    /* Set console handler to capture CTRL events */
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)console_ctrl, TRUE);
    env->m->pool = pool_create();
    env->c->pool = pool_create();
    for (i = 0; i < 4; i ++) {
        env->m->h_stdin[i]   = INVALID_HANDLE_VALUE;
        env->m->h_stdout[i]  = INVALID_HANDLE_VALUE;
        env->m->h_stderr[i]  = INVALID_HANDLE_VALUE;
        env->c->h_stdin[i]  = INVALID_HANDLE_VALUE;
        env->c->h_stdout[i] = INVALID_HANDLE_VALUE;
        env->c->h_stderr[i] = INVALID_HANDLE_VALUE;
    }

    env->m->pinfo.dwProcessId = GetCurrentProcessId();
    env->m->pinfo.dwThreadId = GetCurrentThreadId();

    isok = DuplicateHandle(GetCurrentProcess(),GetCurrentProcess(),
                    GetCurrentProcess(),&handle,PROCESS_ALL_ACCESS,
                    FALSE,0);
    if (isok)
        env->m->pinfo.hProcess = handle;
    else {
        DBPRINTF0("DuplicateHandle failed on Process\n");
        env->m->pinfo.hProcess = GetCurrentProcess();
    }


    isok = DuplicateHandle(GetCurrentProcess(),GetCurrentThread(),
                    GetCurrentProcess(),&handle,PROCESS_ALL_ACCESS,
                    FALSE,0);
    if (isok)
        env->m->pinfo.hThread = handle;
    else {
        DBPRINTF0("DuplicateHandle failed on Thread\n");
        env->m->pinfo.hThread = GetCurrentThread();
    }
    env->m->envp = envp;

    DBPRINTF2("handles: proc %08x thread %08x\n",env->m->pinfo.hProcess,env->m->pinfo.hThread);

    SetProcessShutdownParameters(0x300, SHUTDOWN_NORETRY);
    if (argc > 1)
        mode = parse_args(argc, argv, env->m);
    else /* nothing to do. exit... */
        return 0;
    
    procrun_readenv(env->m, envp);
    
    /* Add the pid to the client env */
    procrun_addenv(PROCRUN_ENV_PPID, NULL, (int)GetCurrentProcessId(),
                   env->m);
    
    procrun_service_params(env->m);
    /* Check if we have a JVM */
    if (procrun_load_jvm(env->m, mode) < 0)
        goto cleanup;
    if(env->m->service.environment) {
        char *nenv = env->m->service.environment;
        while(*nenv) {
            char *cenv = pool_strdup(env->c->pool, nenv);
            char *equals = strchr(cenv, '=');
            if(equals != NULL) {
                char *value = nenv + (equals-cenv)+1;
                *++equals = '\0';
                procrun_addenv(cenv, value, 0, env->c);
            }
            nenv += strlen(nenv)+1;
        }
    }
    /* Create the four events that will cause us to exit
     */
    sprintf(event, "PROC_SHUTDOWN_EVENT%d", GetCurrentProcessId());
    env->m->events[0] = CreateEvent(NULL, TRUE, FALSE, event);
    sprintf(event, "PROC_EXITWAIT_EVENT%d", GetCurrentProcessId());
    env->m->events[1] = CreateEvent(NULL, TRUE, FALSE, event);
    sprintf(event, "PROC_CLOSESTDOUT_EVENT%d", GetCurrentProcessId());
    env->m->events[2] = CreateEvent(NULL, TRUE, FALSE, event);
    sprintf(event, "PROC_CLOSESTDERR_EVENT%d", GetCurrentProcessId());
    env->m->events[3] = CreateEvent(NULL, TRUE, FALSE, event);
    pool_handle(env->m->pool, env->m->events[0]);
    pool_handle(env->m->pool, env->m->events[1]);
    pool_handle(env->m->pool, env->m->events[2]);
    pool_handle(env->m->pool, env->m->events[3]);

    DBPRINTF2("running in mode %d %p\n", mode, env->m->java.dll);
    DBPRINTF1("Events %p\n", env->m->events[0]);

    switch (mode) {
        /* Standard run modes */
        case PROCRUN_CMD_TEST_SERVICE:
#ifdef PROCRUN_WINAPP
        /*
         * GUIT: Display as Try application
         * GUOD: Immediately popup console dialog wrapper
         */
        case PROCRUN_CMD_GUIT_SERVICE:
        case PROCRUN_CMD_GUID_SERVICE:
            {
                DWORD gi;
                /* This one if for about box */
                LoadLibrary("riched32.dll");
                CreateThread(NULL, 0, gui_thread, g_env, 0, &gi);
            }
#else
            if (env->m->service.description)
                SetConsoleTitle(env->m->service.description);
            else if (env->m->service.name)
                SetConsoleTitle(env->m->service.name);
#endif
#ifdef _DEBUG
            debug_process(argc, argv, env->m);
#endif
            service_main(argc, argv);
            break;
#ifdef PROCRUN_WINAPP
        /* Display the service properties dialog
         */
        case PROCRUN_CMD_EDIT_SERVICE:
            LoadLibrary("riched32.dll");
            gui_thread(g_env);
            break;
#endif
        /* Management modes */
        case PROCRUN_CMD_INSTALL_SERVICE:
            if (g_is_windows_nt)
                rv = procrun_install_service(env->m, argc, argv);
            else
                rv = procrun_install_service9x(env->m, argc, argv);
            break;
        case PROCRUN_CMD_UPDATE_SERVICE:
            if (g_is_windows_nt) {
                rv = procrun_update_service(env->m, argc, argv);
                SetEvent(env->m->events[0]);
            }
            else {
              // rv = procrun_update_service9x(env->m, argc, argv);
                rv = -1;
                DBPRINTF0("UPDATE SERVICE is unimplemented on 9x for now\n");
            }
            break;
        case PROCRUN_CMD_DELETE_SERVICE:
            if (g_is_windows_nt)
                rv = procrun_delete_service(env->m);
            else
                rv = procrun_delete_service9x(env->m);
            break;
        case PROCRUN_CMD_STOP_SERVICE:
            rv = -1;
            DBPRINTF0("STOP SERVICE is unimplemented for now\n");
            break;
        case PROCRUN_CMD_RUN_SERVICE:
            if (g_proc_stderr_file==0) {
                g_proc_stderr_file = open("c:/jakarta-service.log",O_CREAT|O_APPEND|O_RDWR);
                if (g_proc_stderr_file < 0)
                    g_proc_stderr_file = 0;
            }
            debug_process(argc, argv, env->m);
            env->m->service.mode = 1;
            if (g_is_windows_nt) {
                dispatch_table[0].lpServiceName = env->m->service.name;
                dispatch_table[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)service_main;
                rv = (StartServiceCtrlDispatcher(dispatch_table) == FALSE);
            } else {
                Windows9xServiceCtrlHandler();
                service_main(argc,argv);
            }
            break;
#ifdef PROCRUN_WINAPP
        case PROCRUN_CMD_GUID_PROCESS:
            /* run as WIN32 application */
            {
                DWORD gi;
                ac_use_try = 1;
                LoadLibrary("riched32.dll");
                CreateThread(NULL, 0, gui_thread, g_env, 0, &gi);
            }
            env->m->service.name = argv[0];
            env->m->service.description = argv[2];
            rv = procrun_redirect(argv[2], envp, env, 1);
            break;

#endif
        default:
            /* Run the process 
             * We can be master or called by env.
             */
            rv = procrun_redirect(argv[1], envp, env, 1);
            break;
    }
    
    /* Wait for shutdown event or child exit */
    while (rv == 0) {
        fired = WaitForMultipleObjects(4, env->m->events,
                                       FALSE, INFINITE); 
        
        if (fired == WAIT_OBJECT_0 ||
            fired == (WAIT_OBJECT_0 + 1) ||
            fired == WAIT_TIMEOUT)
            break;
    }
    DBPRINTF1("Exiting from mode %d\n", mode);
    /* clean up... */
cleanup:

#ifdef PROCRUN_WINAPP
    if (ac_main_hwnd)
        ac_show_try_icon(ac_main_hwnd, NIM_DELETE, NULL, 0);
#endif
    procrun_destroy_jvm(env->m, NULL);
    inject_exitprocess(&env->c->pinfo);
    i = pool_destroy(env->m->pool);
    i = pool_destroy(env->c->pool);
    if (g_proc_stderr_file)
        close(g_proc_stderr_file);
    return 0;
}

static procrun_t *alloc_environment()
{

    procrun_t *env = (procrun_t *)malloc(sizeof(procrun_t));
    if (env) {
        env->m = (process_t *)calloc(1, sizeof(process_t));
        env->c = (process_t *)calloc(1, sizeof(process_t));
    }
    return env;
}

static void free_environment(procrun_t *env)
{
    if (env) {
        free(env->m);
        free(env->c);
        free(env);
        env = NULL;
    }
}

#if defined(PROCRUN_WINAPP)
#pragma message("Compiling WIN32 Application mode")

int WINAPI WinMain(HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine,
                    int nCmdShow)
{
    int rv;
    procrun_t *env = alloc_environment();
    g_proc_mode = PROCRUN_MODE_WINAPP;
    g_env = env;
    ac_instance = hInstance;
    rv = procrun_main(__argc, __argv, _environ, env);

    free_environment(env);
    return rv;
}
#elif defined(PROCRUN_CONSOLE)
#pragma message("Compiling CONSOLE Application mode")

void __cdecl main(int argc, char **argv)
{

    procrun_t *env = alloc_environment();
    g_proc_mode = PROCRUN_MODE_CONSOLE;
    g_env = env;

    procrun_main(argc, argv, _environ, env);

    free_environment(env);
}
#elif defined(PROCRUN_WINDLL)
#pragma message("Compiling Control Panel Application mode")

BOOL WINAPI DllMain(HINSTANCE hInst,
                    ULONG ulReason,
                    LPVOID lpReserved)
{ 

    switch (ulReason) {
        case DLL_PROCESS_ATTACH:
            g_env = NULL;
        break;
        case DLL_PROCESS_DETACH:
            free_environment(g_env);
        break;
        default:
        break;
    } 
    return TRUE;     
}

/* DLL mode functions
 * Ment to be used either from other programs
 * or from installers
 */

/* Install the service.
 * This is a wrapper for //IS//
 */
__declspec(dllexport) void InstallService(const char *service_name,
                                          const char *install,
                                          const char *image_path,
                                          const char *display_name,
                                          const char *description)
{
    int argc = 0;
    char *argv[12];
    char b[MAX_PATH];

    procrun_t *env = alloc_environment();
    g_proc_mode = PROCRUN_MODE_WINDLL;
    g_env = env;
    
    argv[argc++] = "PROCRUN.DLL";
    strcpy(b, PROC_ARG_INSTALL_SERVICE);
    strcat(b, service_name);
    argv[argc++] = b;
    argv[argc++] = "--" PROCRUN_PARAMS_IMAGE;
    argv[argc++] = (char *)image_path;
    argv[argc++] = "--" PROCRUN_PARAMS_INSTALL;
    argv[argc++] = (char *)install;
    argv[argc++] = "--" PROCRUN_PARAMS_DISPLAY;
    argv[argc++] = (char *)display_name;
    argv[argc++] = "--" PROCRUN_PARAMS_DESCRIPTION;
    argv[argc++] = (char *)description;
    
    procrun_main(argc, argv, _environ, env);

    free_environment(env);
    g_env = NULL;
}

/* Update the service.
 * This is a wrapper for //US//
 * 
 */

__declspec(dllexport) void UpdateService(const char *service_name,
                                         const char *param,
                                         const char *value)
{
    int argc = 0;
    char *argv[4];
    char b[MAX_PATH], p[MAX_PATH];

    procrun_t *env = alloc_environment();
    g_proc_mode = PROCRUN_MODE_WINDLL;
    g_env = env;
    
    argv[argc++] = "PROCRUN.DLL";
    strcpy(b, PROC_ARG_UPDATE_SERVICE);
    strcat(b, service_name);
    strcpy(p, "--");
    strcat(p, param);
    argv[argc++] = b;
    argv[argc++] = p;
    argv[argc++] = (char *)value;
    
    procrun_main(argc, argv, _environ, env);

    free_environment(env);
    g_env = NULL;
}

/* Remove the service.
 * This is a wrapper for //DS//
 */

__declspec(dllexport) void RemoveService(const char *service_name)
{
    int argc = 0;
    char *argv[4];
    char b[MAX_PATH];

    procrun_t *env = alloc_environment();
    g_proc_mode = PROCRUN_MODE_WINDLL;
    g_env = env;
    
    argv[argc++] = "PROCRUN.DLL";
    strcpy(b, PROC_ARG_DELETE_SERVICE);
    strcat(b, service_name);
    argv[argc++] = b;
    procrun_main(argc, argv, _environ, env);

    free_environment(env);
    g_env = NULL;
}  

#else
#error Unknown application mode
#endif

