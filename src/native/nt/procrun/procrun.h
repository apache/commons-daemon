/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 *
 * Portions of this software are based upon public domain software
 * originally written at the National Center for Supercomputing Applications,
 * University of Illinois, Urbana-Champaign.
 */

/* ====================================================================
 * procrun
 *
 * Contributed by Mladen Turk <mturk@apache.org>
 *
 * 05 Aug 2002
 * ==================================================================== 
 */

#ifndef PROC_H_INCLUDED
#define PROC_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#if defined(PROCRUN_EXTENDED)
#include "extend.h"
#endif

#define IDD_DLGCONSOLE        101
#define IDS_CONWRAPTITLE      102
#define IDS_CONWRAPCLASS      103
#define IDM_CONSOLE           104
#define IDM_EXIT              105
#define IDI_ICOCONWRAP        106
#define IDC_STATBAR           107
#define IDC_SSTATUS           108
#define IDB_BMPHEADER         109
#define IDL_STDOUT            110
#define IDR_CMENU             111
#define IDM_MENU_EXIT         112
#define IDM_MENU_EDIT         113
#define IDM_MENU_ABOUT        114
#define IDI_ICOCONTRY         115
#define IDR_RTFLIC            116
#define IDB_BMPJAKARTA        117
#define IDD_ABOUTBOX          118
#define IDC_RICHEDIT21        119
#define IDM_ABOUT             120
#define IDI_ICOCONTRYSTOP     121

#define IDM_OPTIONS           122
#define RC_DLG_SRVOPT         130
#define RC_LBL_VER            131
#define RC_LISTVIEW           132

#define RC_TAB_SRVOPT        1000
#define RC_GRP_MSP           1001
#define RC_BTN_APPLY         1002
#define RC_LBL_SN            1003
#define RC_TXT_SN            1004
#define RC_LBL_SD            1005
#define RC_TXT_SD            1006
#define RC_LBL_IP            1007
#define RC_TXT_IP            1008
#define RC_BTN_BIP           1009
#define RC_LBL_WP            1010
#define RC_TXT_WP            1011
#define RC_BTN_BWP           1012
#define RC_CHK_AUTO          1013
#define RC_LBL_UN            1014
#define RC_TXT_UN            1015
#define RC_LBL_UP            1016
#define RC_TXT_UP            1017
#define RC_DLG_JVMOPT        1999
#define RC_GRP_JVMOPT        2000
#define RC_LBL_JVM           2001
#define RC_TXT_JVM           2002
#define RC_BTN_JVM           2003
#define RC_CHK_JVM           2004
#define RC_LBL_JO            2005
#define RC_TXT_JO            2006
#define RC_LBL_SC            2007
#define RC_TXT_SC            2008
#define RC_LBL_EC            2009
#define RC_TXT_EC            2010
#define RC_DLG_STDOPT        2999
#define RC_GRP_STDOPT        3000
#define RC_LBL_STDI          3001
#define RC_TXT_STDI          3002
#define RC_BTN_STDI          3003
#define RC_CHK_STDI          3004
#define RC_LBL_STDO          3005
#define RC_TXT_STDO          3006
#define RC_BTN_STDO          3007
#define RC_LBL_STDE          3008
#define RC_TXT_STDE          3009
#define RC_BTN_STDE          3010
#define RC_ABOUT_TXT         3012

#define IDC_STATIC          -1

#define PROC_ENV_COUNT  32
#define PROC_ARG_COUNT  128
#define PROC_BUFSIZE    4096 
#define PROC_POOL_SIZE  128
#define PROC_VERSION    "1.1.0\0"
#define SERVICE_DEPENDENCIES        "Tcpip\0Afd\0\0"

#define PROC_ARG_ENVPREFIX          "//EP//"
#define PROC_ARG_RUN_JAVA           "//RJ//"
#define PROC_ARG_INSTALL_SERVICE    "//IS//"
#define PROC_ARG_RUN_SERVICE        "//RS//"
#define PROC_ARG_STOP_SERVICE       "//SS//"
#define PROC_ARG_DELETE_SERVICE     "//DS//"
#define PROC_ARG_TEST_SERVICE       "//TS//"
#define PROC_ARG_GUIT_SERVICE       "//GT//"
#define PROC_ARG_GUID_SERVICE       "//GD//"
#define PROC_ARG_GUID_PROCESS       "//GP//"
#define PROC_ARG_UPDATE_SERVICE     "//US//"
#define PROC_ARG_EDIT_SERVICE       "//ES//"

#define PROCRUN_VERSION_STR         "1.1"

#ifndef PROCRUN_REGKEY_ROOT
#define PROCRUN_REGKEY_ROOT         "SOFTWARE\\Apache Software Foundation\\Process Runner " PROCRUN_VERSION_STR
#endif

#ifndef PROCRUN_GUI_DISPLAY
#define PROCRUN_GUI_DISPLAY         "Apache Process Runner"
#endif

#define PROCRUN_REGKEY_SERVICES     "System\\CurrentControlSet\\Services\\%s"
#define PROCRUN_REGKEY_PARAMS       "System\\CurrentControlSet\\Services\\%s\\Parameters"
#define PROCRUN_REGKEY_RSERVICES    PROCRUN_REGKEY_ROOT "\\%s"
#define PROCRUN_REGKEY_RPARAMS      PROCRUN_REGKEY_ROOT "\\%s\\Parameters"

#define JAVASOFT_REGKEY             "SOFTWARE\\JavaSoft\\Java Runtime Environment\\"
#define JAVAHOME_REGKEY             "SOFTWARE\\JavaSoft\\Java Development Kit\\"
 
#define PROCRUN_PARAMS_DISPLAY      "DisplayName"
#define PROCRUN_PARAMS_IMAGE        "ImagePath"
#define PROCRUN_PARAMS_DESCRIPTION  "Description"
#define PROCRUN_PARAMS_CMDARGS      "Arguments"
#define PROCRUN_PARAMS_WORKPATH     "WorkingPath"
#define PROCRUN_PARAMS_JVM          "Java"
#define PROCRUN_PARAMS_JVM_OPTS     "JavaOptions"
#define PROCRUN_PARAMS_STDINFILE    "StdInputFile"
#define PROCRUN_PARAMS_STDOUTFILE   "StdOutputFile"
#define PROCRUN_PARAMS_STDERRFILE   "StdErrorFile"
#define PROCRUN_PARAMS_STARTCLASS   "StartupClass"
#define PROCRUN_PARAMS_STOPCLASS    "ShutdownClass"
#define PROCRUN_PARAMS_STARTUP      "Startup"
#define PROCRUN_PARAMS_ACCOUNT      "User"
#define PROCRUN_PARAMS_PASSWORD     "Password"
#define PROCRUN_PARAMS_INSTALL      "Install"
#define PROCRUN_PARAMS_ENVIRONMENT  "Environment"
/* Console Window position and color */
#define PROCRUN_PARAMS_WINPOS       "WindowPosition"
#define PROCRUN_PARAMS_WINCLR       "WindowColor"
#define PROCRUN_PARAMS_WINBACK      "WindowBackground"


#define PROCRUN_DEFAULT_CLASS       "Main"

#define PROCRUN_ENV_PPID            "PROCRUN_PPID="
#define PROCRUN_ENV_STDIN           "PROCRUN_STDIN="
#define PROCRUN_ENV_STDOUT          "PROCRUN_STDOUT="
#define PROCRUN_ENV_STDERR          "PROCRUN_STDERR="
#define PROCRUN_ENV_ERRFILE         "PROCRUN_ERRFILE="
#define PROCRUN_ENV_SHMNAME         "PROCRUN_SHMNAME="

#define STRN_SIZE(x) (sizeof(x) - 1)
#define STRN_COMPARE(x, s) (strncmp((x), (s), sizeof((s)) -1) == 0)
#define STRNI_COMPARE(x, s) (strnicmp((x), (s), sizeof((s)) -1) == 0)

    enum { 
        PROCRUN_MODE_WINAPP = 1,
        PROCRUN_MODE_GUI,
        PROCRUN_MODE_CONSOLE,
        PROCRUN_MODE_WINDLL
    };

    enum { 
        PROCRUN_CMD_ENVPREFIX = 1,
        PROCRUN_CMD_RUN_JAVA,
        PROCRUN_CMD_INSTALL_SERVICE,
        PROCRUN_CMD_RUN_SERVICE,
        PROCRUN_CMD_TEST_SERVICE,
        PROCRUN_CMD_GUIT_SERVICE,
        PROCRUN_CMD_GUID_SERVICE,
        PROCRUN_CMD_STOP_SERVICE,
        PROCRUN_CMD_DELETE_SERVICE,
        PROCRUN_CMD_UPDATE_SERVICE,
        PROCRUN_CMD_EDIT_SERVICE,
        PROCRUN_CMD_GUID_PROCESS
    };

    typedef struct mpool_t mpool_t;
    struct mpool_t {
        void    *m;
        HANDLE  h;
    };

    typedef struct pool_t pool_t;
    struct pool_t {
        mpool_t   mp[PROC_POOL_SIZE];
        CRITICAL_SECTION lock;
        int       size;
    };

    typedef struct buffer_t buffer_t;
    struct buffer_t {
        char *buf;
        int  siz;
        int  len;
        int  pos;
    };
    
    typedef struct service_t service_t;
    struct service_t {
        char                  *name;
        char                  *description;
        char                  *display;
        char                  *path;
        char                  *image;
        char                  *account;
        char                  *password;
        char                  *environment;
        HANDLE                infile;
        HANDLE                outfile;
        HANDLE                errfile;
        char                  *inname;
        char                  *outname;
        char                  *errname;
        SERVICE_STATUS        status;
        SERVICE_STATUS_HANDLE h_status;
        int                   mode;
        int                   startup;
    };

    typedef struct java_t java_t;
    struct java_t {
        char         *path;
        char         *jpath;
        char         *jbin;
        char         *start_class;
        char         *stop_class;
        HMODULE      dll;
        JavaVM       *jvm;
        JNIEnv       *env;
        jclass       start_bridge;
        jclass       stop_bridge;
        jmethodID    start_mid; 
        jmethodID    stop_mid; 
        char         *start_method;
        char         *stop_method;
        char         *start_param;
        char         *stop_param;
        char         *opts;
        char         *display;
    };

    typedef struct process_t process_t;
    struct process_t {
        HANDLE      events[4];
        HANDLE      h_stdin[4];
        HANDLE      h_stdout[4];
        HANDLE      h_stderr[4];
        char        *files[3];
        buffer_t    buff_in;
        buffer_t    buff_out;
        buffer_t    buff_err;
        PROCESS_INFORMATION  pinfo;
        int         ppid;
        char        *env[PROC_ENV_COUNT];
        char        **envp;
        char        *argw;
        char        *envw;
        char        *env_prefix;
        service_t   service;
        java_t      java;
        pool_t      *pool;
    };

    typedef struct procrun_t procrun_t;
    struct procrun_t {
        process_t   *m;
        process_t   *c;
    };
    
    int procrun_main(int argc, char **argv, char **envp, procrun_t *env);
    int procrun_update_service(process_t *proc, int argc, char **argv);
    void save_service_params(process_t *proc, char *java);
    pool_t *pool_create();
    int pool_destroy(pool_t *pool);

#if !defined(PROCRUN_CONSOLE)

extern DWORD WINAPI gui_thread(LPVOID param);

extern void ac_add_list_string(const char *str, int len);
extern int  ac_use_try;
extern int  ac_use_dlg;
extern int  ac_use_show;
extern int  ac_use_props;
extern int  ac_use_lview;

extern  RECT        ac_winpos;
extern  HINSTANCE   ac_instance;
extern  HWND        ac_main_hwnd;
extern  HWND        ac_list_hwnd;
extern  char        *ac_cmdname;

void    ac_show_try_icon(HWND hwnd, DWORD message, const char *tip, int stop);
void    ac_center_window(HWND hwnd);

#if defined(PROCRUN_EXTENDED)

void acx_process_splash(const char *str);
void acx_create_view(HWND hdlg, LPRECT pr, LPRECT pw);
void acx_parse_list_string(const char *str);
void acx_create_spash(HWND hwnd);
void acx_close_spash();
void acx_init_extended();


#endif

#endif

#ifdef __cplusplus
}
#endif

#endif /* PROC_H_INCLUDED */

