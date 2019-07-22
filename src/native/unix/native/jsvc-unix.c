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

#include "jsvc.h"

#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>
#include <errno.h>
#ifdef OS_LINUX
#include <sys/prctl.h>
#include <sys/syscall.h>
#define _LINUX_FS_H
#include <linux/capability.h>
#ifdef HAVE_LIBCAP
#include <sys/capability.h>
#endif
#endif
#include <time.h>

#ifdef OS_CYGWIN
#include <sys/fcntl.h>
#define F_ULOCK 0                      /* Unlock a previously locked region */
#define F_LOCK  1                      /* Lock a region for exclusive use */
#endif
extern char **environ;

static mode_t envmask;          /* mask to create the files */

pid_t controller_pid = 0;       /* The parent process pid */
pid_t controlled = 0;           /* the child process pid */
pid_t logger_pid = 0;           /* the logger process pid */
static volatile bool stopping = false;
static volatile bool doreload = false;
static bool doreopen = false;
static bool dosignal = false;

static int run_controller(arg_data *args, home_data *data, uid_t uid, gid_t gid);
static void set_output(char *outfile, char *errfile, bool redirectstdin, char *procname);

#ifdef OS_CYGWIN
/*
 * File locking routine
 */
static int lockf(int fildes, int function, off_t size)
{
    struct flock buf;

    switch (function) {
        case F_LOCK:
            buf.l_type = F_WRLCK;
            break;
        case F_ULOCK:
            buf.l_type = F_UNLCK;
            break;
        default:
            return -1;
    }
    buf.l_whence = 0;
    buf.l_start = 0;
    buf.l_len = size;

    return fcntl(fildes, F_SETLK, &buf);
}

#endif

static void handler(int sig)
{
    switch (sig) {
        case SIGTERM:
            log_debug("Caught SIGTERM: Scheduling a shutdown");
            if (stopping == true) {
                log_error("Shutdown or reload already scheduled");
            }
            else {
                stopping = true;
                /* Ensure the controller knows a shutdown was requested. */
                kill(controller_pid, sig);
            }
            break;
        case SIGINT:
            log_debug("Caught SIGINT: Scheduling a shutdown");
            if (stopping == true) {
                log_error("Shutdown or reload already scheduled");
            }
            else {
                stopping = true;
                /* Ensure the controller knows a shutdown was requested. */
                kill(controller_pid, sig);
            }
            break;
        case SIGHUP:
            log_debug("Caught SIGHUP: Scheduling a reload");
            if (stopping == true) {
                log_error("Shutdown or reload already scheduled");
            }
            else {
                stopping = true;
                doreload = true;
            }
            break;
        case SIGUSR1:
            log_debug("Caught SIGUSR1: Reopening logs");
            doreopen = true;
            break;
        case SIGUSR2:
            log_debug("Caught SIGUSR2: Scheduling a custom signal");
            dosignal = true;
            break;
        default:
            log_debug("Caught unknown signal %d", sig);
            break;
    }
}

/* user and group */
static int set_user_group(const char *user, int uid, int gid)
{
    if (user != NULL) {
        if (setgid(gid) != 0) {
            log_error("Cannot set group id for user '%s'", user);
            return -1;
        }
        if (initgroups(user, gid) != 0) {
            if (getuid() != uid) {
                log_error("Cannot set supplement group list for user '%s'", user);
                return -1;
            }
            else
                log_debug("Cannot set supplement group list for user '%s'", user);
        }
        if (getuid() == uid) {
            log_debug("No need to change user to '%s'!", user);
            return 0;
        }
        if (setuid(uid) != 0) {
            log_error("Cannot set user id for user '%s'", user);
            return -1;
        }
        log_debug("user changed to '%s'", user);
    }
    return 0;
}

/* Set linux capability, user and group */
#ifdef OS_LINUX
/* CAPSALL is to allow to read/write at any location */
#define LEGACY_CAPSALL  (1 << CAP_NET_BIND_SERVICE) +   \
                        (1 << CAP_SETUID) +             \
                        (1 << CAP_SETGID) +             \
                        (1 << CAP_DAC_READ_SEARCH) +    \
                        (1 << CAP_DAC_OVERRIDE)

#define LEGACY_CAPSMAX  (1 << CAP_NET_BIND_SERVICE) +   \
                        (1 << CAP_DAC_READ_SEARCH) +    \
                        (1 << CAP_DAC_OVERRIDE)

/* That a more reasonable configuration */
#define LEGACY_CAPS     (1 << CAP_NET_BIND_SERVICE) +   \
                        (1 << CAP_DAC_READ_SEARCH) +    \
                        (1 << CAP_SETUID) +             \
                        (1 << CAP_SETGID)

/* probably the only one Java could use */
#define LEGACY_CAPSMIN  (1 << CAP_NET_BIND_SERVICE) +   \
                        (1 << CAP_DAC_READ_SEARCH)

#define LEGACY_CAP_VERSION  0x19980330
static int set_legacy_caps(int caps)
{
    struct __user_cap_header_struct caphead;
    struct __user_cap_data_struct cap;

    memset(&caphead, 0, sizeof caphead);
    caphead.version = LEGACY_CAP_VERSION;
    caphead.pid = 0;
    memset(&cap, 0, sizeof cap);
    cap.effective = caps;
    cap.permitted = caps;
    cap.inheritable = caps;
    if (syscall(__NR_capset, &caphead, &cap) < 0) {
        log_error("set_caps: failed to set capabilities");
        log_error("check that your kernel supports capabilities");
        return -1;
    }
    return 0;
}

#ifdef HAVE_LIBCAP
static cap_value_t caps_std[] = {
    CAP_NET_BIND_SERVICE,
    CAP_SETUID,
    CAP_SETGID,
    CAP_DAC_READ_SEARCH
};

static cap_value_t caps_min[] = {
    CAP_NET_BIND_SERVICE,
    CAP_DAC_READ_SEARCH
};

#define CAPS     1
#define CAPSMIN  2


typedef int (*fd_cap_free) (void *);
typedef cap_t (*fd_cap_init) (void);
typedef int (*fd_cap_clear) (cap_t);
typedef int (*fd_cap_get_flag) (cap_t, cap_value_t, cap_flag_t, cap_flag_value_t *);
typedef int (*fd_cap_set_flag) (cap_t, cap_flag_t, int, const cap_value_t *, cap_flag_value_t);
typedef int (*fd_cap_set_proc) (cap_t);

static dso_handle hlibcap = NULL;
static fd_cap_free fp_cap_free;
static fd_cap_init fp_cap_init;
static fd_cap_clear fp_cap_clear;
static fd_cap_get_flag fp_cap_get_flag;
static fd_cap_set_flag fp_cap_set_flag;
static fd_cap_set_proc fp_cap_set_proc;

static const char *libcap_locs[] = {
#ifdef __LP64__
    "/lib64/libcap.so.2",
    "/lib64/libcap.so.1",
    "/lib64/libcap.so",
    "/usr/lib64/libcap.so.2",
    "/usr/lib64/libcap.so.1",
    "/usr/lib64/libcap.so",
#endif
    "/lib/libcap.so.2",
    "/lib/libcap.so.1",
    "/lib/libcap.so",
    "/usr/lib/libcap.so.2",
    "/usr/lib/libcap.so.1",
    "/usr/lib/libcap.so",
    "libcap.so.2",
    "libcap.so.1",
    "libcap.so",
    NULL
};

static int ld_libcap(void)
{
    int i = 0;
    dso_handle dso = NULL;
#define CAP_LDD(name) \
    if ((fp_##name = dso_symbol(dso, #name)) == NULL) { \
        log_error("cannot locate " #name " in libcap.so -- %s", dso_error());  \
        dso_unlink(dso);    \
        return -1;          \
    } else log_debug("loaded " #name " from libcap.")

    if (hlibcap != NULL)
        return 0;
    while (libcap_locs[i] && dso == NULL) {
        if ((dso = dso_link(libcap_locs[i++])))
            break;
    };
    if (dso == NULL) {
        log_error("failed loading capabilities library -- %s.", dso_error());
        return -1;
    }
    CAP_LDD(cap_free);
    CAP_LDD(cap_init);
    CAP_LDD(cap_clear);

    CAP_LDD(cap_get_flag);
    CAP_LDD(cap_set_flag);
    CAP_LDD(cap_set_proc);
    hlibcap = dso;
#undef CAP_LDD
    return 0;
}


static int set_caps(int cap_type)
{
    cap_t c;
    int ncap;
    int flag = CAP_SET;
    cap_value_t *caps;
    const char *type;

    if (ld_libcap()) {
        return set_legacy_caps(cap_type);
    }
    if (cap_type == CAPS) {
        ncap = sizeof(caps_std) / sizeof(cap_value_t);
        caps = caps_std;
        type = "default";
    }
    else if (cap_type == CAPSMIN) {
        ncap = sizeof(caps_min) / sizeof(cap_value_t);
        caps = caps_min;
        type = "min";
    }
    else {
        ncap = sizeof(caps_min) / sizeof(cap_value_t);
        caps = caps_min;
        type = "null";
        flag = CAP_CLEAR;
    }
    c = (*fp_cap_init) ();
    (*fp_cap_clear) (c);
    (*fp_cap_set_flag) (c, CAP_EFFECTIVE, ncap, caps, flag);
    (*fp_cap_set_flag) (c, CAP_INHERITABLE, ncap, caps, flag);
    (*fp_cap_set_flag) (c, CAP_PERMITTED, ncap, caps, flag);
    if ((*fp_cap_set_proc) (c) != 0) {
        log_error("failed setting %s capabilities.", type);
        return -1;
    }
    (*fp_cap_free) (c);
    if (cap_type == CAPS)
        log_debug("increased capability set.");
    else if (cap_type == CAPSMIN)
        log_debug("decreased capability set to min required.");
    else
        log_debug("dropped capabilities.");
    return 0;
}

#else /* !HAVE_LIBCAP */
/* CAPSALL is to allow to read/write at any location */
#define CAPSALL LEGACY_CAPSALL
#define CAPSMAX LEGACY_CAPSMAX
#define CAPS    LEGACY_CAPS
#define CAPSMIN LEGACY_CAPSMIN
static int set_caps(int caps)
{
    return set_legacy_caps(caps);
}
#endif

static int linuxset_user_group(const char *user, int uid, int gid)
{
    int caps_set = 0;

    if (user == NULL)
        return 0;
    /* set capabilities enough for binding port 80 setuid/getuid */
    if (getuid() == 0) {
        if (set_caps(CAPS) != 0) {
            if (getuid() != uid) {
                log_error("set_caps(CAPS) failed for user '%s'", user);
                return -1;
            }
            log_debug("set_caps(CAPS) failed for user '%s'", user);
        }
        /* make sure they are kept after setuid */
        if (prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0) < 0) {
            log_error("prctl failed in for user '%s'", user);
            return -1;
        }
        caps_set = 1;
    }

    /* set setuid/getuid */
    if (set_user_group(user, uid, gid) != 0) {
        log_error("set_user_group failed for user '%s'", user);
        return -1;
    }

    if (caps_set) {
        /* set capability to binding port 80 read conf */
        if (set_caps(CAPSMIN) != 0) {
            if (getuid() != uid) {
                log_error("set_caps(CAPSMIN) failed for user '%s'", user);
                return -1;
            }
            log_debug("set_caps(CAPSMIN) failed for user '%s'", user);
        }
    }

    return 0;
}
#endif


static bool checkuser(char *user, uid_t * uid, gid_t * gid)
{
    struct passwd *pwds = NULL;
    int status = 0;
    pid_t pid = 0;

    /* Do we actually _have_ to switch user? */
    if (user == NULL)
        return true;

    pwds = getpwnam(user);
    if (pwds == NULL) {
        log_error("Invalid user name '%s' specified", user);
        return false;
    }

    *uid = pwds->pw_uid;
    *gid = pwds->pw_gid;

    /* Validate the user name in another process */
    pid = fork();
    if (pid == -1) {
        log_error("Cannot validate user name");
        return false;
    }

    /* If we're in the child process, let's validate */
    if (pid == 0) {
        if (set_user_group(user, *uid, *gid) != 0)
            exit(1);
        /* If we got here we switched user/group */
        exit(0);
    }

    while (waitpid(pid, &status, 0) != pid) {
        /* Just wait */
    }

    /* The child must have exited cleanly */
    if (WIFEXITED(status)) {
        status = WEXITSTATUS(status);

        /* If the child got out with 0 the user is ok */
        if (status == 0) {
            log_debug("User '%s' validated", user);
            return true;
        }
    }

    log_error("Error validating user '%s'", user);
    return false;
}

#ifdef OS_CYGWIN
static void cygwincontroller(void)
{
    raise(SIGTERM);
}
#endif
static void controller(int sig, siginfo_t * sip, void *ucp)
{
    switch (sig) {
        case SIGTERM:
        case SIGINT:
            if (!stopping) {
                /*
                 * Only forward a signal that requests shutdown once (the
                 * issue being that the child also forwards the signal to
                 * the parent and we need to avoid loops).
                 *
                 * Note that there are * two * instances of the stopping
                 * variable ... one in the parent and the second in the
                 * child.
                 */
                stopping = true;
                if (sip == NULL || !(sip->si_code <= 0 && sip->si_pid == controlled)) {
                    log_debug("Forwarding signal %d to process %d", sig, controlled);
                    kill(controlled, sig);
                }
            }
            break;
        case SIGHUP:
        case SIGUSR1:
        case SIGUSR2:
            log_debug("Forwarding signal %d to process %d", sig, controlled);
            kill(controlled, sig);
            break;
        default:
            log_debug("Caught unknown signal %d", sig);
            break;
    }
}

static int mkdir0(const char *name, int perms)
{
    if (mkdir(name, perms) == 0)
        return 0;
    else
        return errno;
}

static int mkdir1(char *name, int perms)
{
    int rc;

    rc = mkdir0(name, perms);
    if (rc == EEXIST)
        return 0;
    if (rc == ENOENT) {                /* Missing an intermediate dir */
        char *pos;
        if ((pos = strrchr(name, '/'))) {
            *pos = '\0';
            if (*name) {
                if (!(rc = mkdir1(name, perms))) {
                    /* Try again, now with parents created
                     */
                    *pos = '/';
                    rc = mkdir0(name, perms);
                }
            }
            *pos = '/';
        }
    }
    return rc;
}

static int mkdir2(const char *name, int perms)
{
    int rc = 0;
    char *pos;
    char *dir = strdup(name);

    if (!dir)
        return ENOMEM;
    if ((pos = strrchr(dir, '/'))) {
        *pos = '\0';
        if (*dir)
            rc = mkdir1(dir, perms);
    }
    free(dir);
    return rc;
}

/*
 * Check pid and if still running
 */

static int check_pid(arg_data *args)
{
    int fd;
    FILE *pidf;
    char buff[80];
    pid_t pidn = getpid();
    int i, pid;
    int once = 0;

    /* skip writing the pid file if version or check */
    if (args->vers || args->chck) {
        return 0;
    }

retry:
    fd = open(args->pidf, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        if (once == 0 && (errno == ENOTDIR || errno == ENOENT)) {
            once = 1;
            if (mkdir2(args->pidf, S_IRWXU | S_IXGRP | S_IRGRP | S_IXOTH | S_IROTH) == 0)
                goto retry;
        }
        log_error("Cannot open PID file %s, PID is %d", args->pidf, pidn);
        return -1;
    }
    else {
        lockf(fd, F_LOCK, 0);
        i = read(fd, buff, sizeof(buff));
        if (i > 0) {
            buff[i] = '\0';
            pid = atoi(buff);
            if (kill(pid, 0) == 0) {
                log_error("Still running according to PID file %s, PID is %d", args->pidf, pid);
                lockf(fd, F_ULOCK, 0);
                close(fd);
                return 122;
            }
        }
        lseek(fd, SEEK_SET, 0);
        pidf = fdopen(fd, "r+");
        fprintf(pidf, "%d\n", (int)getpid());
        fflush(pidf);
        fclose(pidf);
        lockf(fd, F_ULOCK, 0);
        close(fd);
    }
    return 0;
}

/*
 * Delete the pid file
 */
static void remove_pid_file(arg_data *args, int pidn)
{
    char buff[80];
    int fd, i, pid;

    fd = open(args->pidf, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    log_debug("remove_pid_file: open %s: fd=%d", args->pidf, fd);
    if (fd < 0) {
        return;
    }
    lockf(fd, F_LOCK, 0);
    i = read(fd, buff, sizeof(buff));
    if (i > 0) {
        buff[i] = '\0';
        pid = atoi(buff);
    }
    else {
        pid = -1;
    }
    if (pid == pidn) {
        /* delete the file while it's still locked */
        unlink(args->pidf);
    }
    else {
        log_debug
            ("remove_pid_file: pid changed (%d->%d), not removing pid file %s",
             pidn, pid, args->pidf);
    }
    lockf(fd, F_ULOCK, 0);
    close(fd);
}

/*
 * read the pid from the pidfile
 */
static int get_pidf(arg_data *args, bool quiet)
{
    int fd;
    int i;
    char buff[80];

    fd = open(args->pidf, O_RDONLY, 0);
    if (!quiet)
        log_debug("get_pidf: %d in %s", fd, args->pidf);
    if (fd < 0) {
        /* something has gone wrong the JVM has stopped */
        return -1;
    }
    lockf(fd, F_LOCK, 0);
    i = read(fd, buff, sizeof(buff));
    lockf(fd, F_ULOCK, 0);
    close(fd);
    if (i > 0) {
        buff[i] = '\0';
        i = atoi(buff);
        if (!quiet)
            log_debug("get_pidf: pid %d", i);
        if (kill(i, 0) == 0)
            return i;
    }
    return -1;
}

/*
 * Check temporatory file created by controller
 * /tmp/pid.jsvc_up
 * Notes:
 * we fork several times
 * 1 - to be a daemon before the setsid(), the child is the controler process.
 * 2 - to start the JVM in the child process. (whose pid is stored in pidfile).
 */
static int check_tmp_file(arg_data *args)
{
    int pid;
    char buff[80];
    int fd;

    pid = get_pidf(args, false);
    if (pid < 0)
        return -1;
    sprintf(buff, "/tmp/%d.jsvc_up", pid);
    log_debug("check_tmp_file: %s", buff);
    fd = open(buff, O_RDONLY);
    if (fd == -1)
        return -1;
    close(fd);
    return 0;
}

static void create_tmp_file(arg_data *args)
{
    char buff[80];
    int fd;

    sprintf(buff, "/tmp/%d.jsvc_up", (int)getpid());
    log_debug("create_tmp_file: %s", buff);
    fd = open(buff, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd != -1)
        close(fd);
}

static void remove_tmp_file(arg_data *args)
{
    char buff[80];

    sprintf(buff, "/tmp/%d.jsvc_up", (int)getpid());
    log_debug("remove_tmp_file: %s", buff);
    unlink(buff);
}

/*
 * wait until jsvc create the I am ready file
 * pid is the controller and args->pidf the JVM itself.
 */
static int wait_child(arg_data *args, int pid)
{
    int count = 10;
    bool havejvm = false;
    int fd;
    char buff[80];
    int i, status, waittime;

    log_debug("wait_child %d", pid);
    waittime = args->wait / 10;
    if (waittime > 10) {
        count = waittime;
        waittime = 10;
    }
    while (count > 0) {
        sleep(1);
        /* check if the controler is still running */
        if (waitpid(pid, &status, WNOHANG) == pid) {
            if (WIFEXITED(status))
                return (WEXITSTATUS(status));
            else
                return 1;
        }

        /* check if the pid file process exists */
        fd = open(args->pidf, O_RDONLY);
        if (fd < 0 && havejvm) {
            /* something has gone wrong the JVM has stopped */
            return 1;
        }
        lockf(fd, F_LOCK, 0);
        i = read(fd, buff, sizeof(buff));
        lockf(fd, F_ULOCK, 0);
        close(fd);
        if (i > 0) {
            buff[i] = '\0';
            i = atoi(buff);
            if (kill(i, 0) == 0) {
                /* the JVM process has started */
                havejvm = true;
                if (check_tmp_file(args) == 0) {
                    /* the JVM is started */
                    if (waitpid(pid, &status, WNOHANG) == pid) {
                        if (WIFEXITED(status))
                            return (WEXITSTATUS(status));
                        else
                            return 1;
                    }
                    return 0;          /* ready JVM started */
                }
            }
        }
        sleep(waittime);
        count--;
    }
    /* It takes more than the wait time to start,
     * something must be wrong
     */
    return 1;
}

/*
 * stop the running jsvc
 */
static int stop_child(arg_data *args)
{
    int pid = get_pidf(args, false);
    int count = 60;

    if (pid > 0) {
        /* kill the process and wait until the pidfile has been
         * removed by the controler
         */
        kill(pid, SIGTERM);
        while (count > 0) {
            sleep(1);
            pid = get_pidf(args, true);
            if (pid <= 0) {
                /* JVM has stopped */
                return 0;
            }
            count--;
        }
    }
    return -1;
}

/*
 * child process logic.
 */

static int child(arg_data *args, home_data *data, uid_t uid, gid_t gid)
{
    int ret = 0;
    struct sigaction act;

    /* check the pid file */
    ret = check_pid(args);
    if (args->vers != true && args->chck != true) {
        if (ret == 122)
            return ret;
        if (ret < 0)
            return ret;
    }

#ifdef OS_LINUX
    /* setuid()/setgid() only apply the current thread so we must do it now */
    if (linuxset_user_group(args->user, uid, gid) != 0)
        return 4;
#endif
    /* Initialize the Java VM */
    if (java_init(args, data) != true) {
        log_debug("java_init failed");
        return 1;
    }
    else
        log_debug("java_init done");

    /* Check wether we need to dump the VM version */
    if (args->vers == true) {
        log_error("jsvc (Apache Commons Daemon) " JSVC_VERSION_STRING);
        log_error("Copyright (c) 1999-2019 Apache Software Foundation.");
        if (java_version() != true) {
            return -1;
        }
        else
            return 0;
    }
    /* Check wether we need to dump the VM version */
    else if (args->vershow == true) {
        if (java_version() != true) {
            return 7;
        }
    }

    /* Do we have to do a "check-only" initialization? */
    if (args->chck == true) {
        if (java_check(args) != true)
            return 2;
        printf("Service \"%s\" checked successfully\n", args->clas);
        return 0;
    }

    /* Load the service */
    if (java_load(args) != true) {
        log_debug("java_load failed");
        return 3;
    }
    else
        log_debug("java_load done");

    /* Downgrade user */
#ifdef OS_LINUX
    if (args->user && set_caps(0) != 0) {
        log_debug("set_caps (0) failed");
        return 4;
    }
#else
    if (set_user_group(args->user, uid, gid) != 0)
        return 4;
#endif

    /* Start the service */
    if (java_start() != true) {
        log_debug("java_start failed");
        return 5;
    }
    else
        log_debug("java_start done");

    /* Install signal handlers */
    memset(&act, '\0', sizeof(act));
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGUSR2, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);

    log_debug("Waiting for a signal to be delivered");
    create_tmp_file(args);
    while (!stopping) {
#if defined(OSD_POSIX)
        java_sleep(60);
        /* pause(); */
#else
        /* pause() is not threadsafe */
        sleep(60);
#endif
        if (doreopen) {
            doreopen = false;
            set_output(args->outfile, args->errfile, args->redirectstdin, args->procname);
        }
        if (dosignal) {
            dosignal = false;
            java_signal();
        }
    }
    remove_tmp_file(args);
    log_debug("Shutdown or reload requested: exiting");

    /* Stop the service */
    if (java_stop() != true)
        return 6;

    if (doreload == true)
        ret = 123;
    else
        ret = 0;

    /* Destroy the service */
    java_destroy();

    /* Destroy the Java VM */
    if (JVM_destroy(ret) != true)
        return 7;

    return ret;
}

/*
 * freopen close the file first and then open the new file
 * that is not very good if we are try to trace the output
 * note the code assumes that the errors are configuration errors.
 */
static FILE *loc_freopen(char *outfile, char *mode, FILE * stream)
{
    FILE *ftest;

    mkdir2(outfile, S_IRWXU);
    ftest = fopen(outfile, mode);
    if (ftest == NULL) {
        fprintf(stderr, "Unable to redirect to %s\n", outfile);
        return stream;
    }
    fclose(ftest);
    return freopen(outfile, mode, stream);
}

#define LOGBUF_SIZE 1024

/* Read from file descriptors. Log to syslog. */
static int logger_child(int out_fd, int err_fd, char *procname)
{
    fd_set rfds;
    struct timeval tv;
    int retval, nfd = -1, rc = 0;
    ssize_t n;
    char buf[LOGBUF_SIZE];

    if (out_fd == -1 && err_fd == -1)
        return EINVAL;
    if (out_fd == -1)
        nfd = err_fd;
    else if (err_fd == -1)
        nfd = out_fd;
    else
        nfd = out_fd > err_fd ? out_fd : err_fd;
    ++nfd;

    openlog(procname, LOG_PID, LOG_DAEMON);

    while (out_fd != -1 || err_fd != -1) {
        FD_ZERO(&rfds);
        if (out_fd != -1) {
            FD_SET(out_fd, &rfds);
        }
        if (err_fd != -1) {
            FD_SET(err_fd, &rfds);
        }
        tv.tv_sec = 60;
        tv.tv_usec = 0;
        retval = select(nfd, &rfds, NULL, NULL, &tv);
        if (retval == -1) {
            rc = errno;
            syslog(LOG_ERR, "select: %s", strerror(errno));
            /* If select failed no point to continue */
            break;
        }
        else if (retval) {
            if (out_fd != -1 && FD_ISSET(out_fd, &rfds)) {
                do {
                    n = read(out_fd, buf, LOGBUF_SIZE - 1);
                } while (n == -1 && errno == EINTR);
                if (n == -1) {
                    syslog(LOG_ERR, "read: %s", strerror(errno));
                    close(out_fd);
                    if (err_fd == -1)
                        break;
                    nfd = err_fd + 1;
                    out_fd = -1;
                }
                else if (n > 0 && buf[0] != '\n') {
                    buf[n] = 0;
                    syslog(LOG_INFO, "%s", buf);
                }
            }
            if (err_fd != -1 && FD_ISSET(err_fd, &rfds)) {
                do {
                    n = read(err_fd, buf, LOGBUF_SIZE - 1);
                } while (n == -1 && errno == EINTR);
                if (n == -1) {
                    syslog(LOG_ERR, "read: %s", strerror(errno));
                    close(err_fd);
                    if (out_fd == -1)
                        break;
                    nfd = out_fd + 1;
                    err_fd = -1;
                }
                else if (n > 0 && buf[0] != '\n') {
                    buf[n] = 0;
                    syslog(LOG_ERR, "%s", buf);
                }
            }
        }
    }
    return rc;
}

/**
 *  Redirect stdin, stdout, stderr.
 */
static void set_output(char *outfile, char *errfile, bool redirectstdin, char *procname)
{
    int out_pipe[2] = { -1, -1 };
    int err_pipe[2] = { -1, -1 };
    int fork_needed = 0;

    if (redirectstdin == true) {
        freopen("/dev/null", "r", stdin);
    }

    log_debug("redirecting stdout to %s and stderr to %s", outfile, errfile);

    /* make sure the debug goes out */
    if (log_debug_flag == true && strcmp(errfile, "/dev/null") == 0)
        return;
    if (strcmp(outfile, "&1") == 0 && strcmp(errfile, "&2") == 0)
        return;
    if (strcmp(outfile, "SYSLOG") == 0) {
        freopen("/dev/null", "a", stdout);
        /* Send stdout to syslog through a logger process */
        if (pipe(out_pipe) == -1) {
            log_error("cannot create stdout pipe: %s", strerror(errno));
        }
        else {
            fork_needed = 1;
            log_stdout_syslog_flag = true;
        }
    }
    else if (strcmp(outfile, "&2")) {
        if (strcmp(outfile, "&1")) {
            /* Redirect stdout to a file */
            loc_freopen(outfile, "a", stdout);
        }
    }

    if (strcmp(errfile, "SYSLOG") == 0) {
        freopen("/dev/null", "a", stderr);
        /* Send stderr to syslog through a logger process */
        if (pipe(err_pipe) == -1) {
            log_error("cannot create stderr pipe: %s", strerror(errno));
        }
        else {
            fork_needed = 1;
            log_stderr_syslog_flag = true;
        }
    }
    else if (strcmp(errfile, "&1")) {
        if (strcmp(errfile, "&2")) {
            /* Redirect stderr to a file */
            loc_freopen(errfile, "a", stderr);
        }
    }
    if (strcmp(errfile, "&1") == 0 && strcmp(outfile, "&1")) {
        /*
         * -errfile &1 -outfile foo
         * Redirect stderr to stdout
         */
        close(2);
        dup2(1, 2);
    }
    if (strcmp(outfile, "&2") == 0 && strcmp(errfile, "&2")) {
        /*
         * -outfile &2 -errfile foo
         * Redirect stdout to stderr
         */
        close(1);
        dup2(2, 1);
    }

    if (fork_needed) {
        pid_t pid = fork();
        if (pid == -1) {
            log_error("cannot create logger process: %s", strerror(errno));
        }
        else {
            if (pid != 0) {
                /* Parent process.
                 * Close child pipe endpoints.
                 */
                logger_pid = pid;
                if (out_pipe[0] != -1) {
                    close(out_pipe[0]);
                    if (dup2(out_pipe[1], 1) == -1) {
                        log_error("cannot redirect stdout to pipe for syslog: %s", strerror(errno));
                    }
                }
                if (err_pipe[0] != -1) {
                    close(err_pipe[0]);
                    if (dup2(err_pipe[1], 2) == -1) {
                        log_error("cannot redirect stderr to pipe for syslog: %s", strerror(errno));
                    }
                }
            }
            else {
                exit(logger_child(out_pipe[0], err_pipe[0], procname));
            }
        }
    }
}

int main(int argc, char *argv[])
{
    arg_data *args = NULL;
    home_data *data = NULL;
    pid_t pid = 0;
    uid_t uid = 0;
    gid_t gid = 0;
    int res;

    /* Parse command line arguments */
    args = arguments(argc, argv);
    if (args == NULL)
        return 1;

    /* Stop running jsvc if required */
    if (args->stop == true)
        return (stop_child(args));

    /* Let's check if we can switch user/group IDs */
    if (checkuser(args->user, &uid, &gid) == false)
        return 1;

    /* Retrieve JAVA_HOME layout */
    data = home(args->home);
    if (data == NULL)
        return 1;

    /* Check for help */
    if (args->help == true) {
        help(data);
        return 0;
    }

#ifdef OS_LINUX
    /* On some UNIX operating systems, we need to REPLACE this current
       process image with another one (thru execve) to allow the correct
       loading of VMs (notably this is for Linux). Set, replace, and go. */
    if (strcmp(argv[0], args->procname) != 0) {
        char *oldpath = getenv("LD_LIBRARY_PATH");
        char *libf = java_library(args, data);
        char *filename;
        char buf[2048];
        int ret;
        char *tmp = NULL;
        char *p1 = NULL;
        char *p2 = NULL;

        /* We don't want to use a form of exec() that searches the
         * PATH, so require that argv[0] be either an absolute or
         * relative path.  Error out if this isn't the case.
         */
        tmp = strchr(argv[0], '/');
        if (tmp == NULL) {
            log_error("JSVC re-exec requires execution with an absolute or relative path");
            return 1;
        }

        /*
         * There is no need to change LD_LIBRARY_PATH
         * if we were not able to find a path to libjvm.so
         * (additionaly a strdup(NULL) cores dump on my machine).
         */
        if (libf != NULL) {
            p1 = strdup(libf);
            tmp = strrchr(p1, '/');
            if (tmp != NULL)
                tmp[0] = '\0';

            p2 = strdup(p1);
            tmp = strrchr(p2, '/');
            if (tmp != NULL)
                tmp[0] = '\0';

            if (oldpath == NULL)
                snprintf(buf, 2048, "%s:%s", p1, p2);
            else
                snprintf(buf, 2048, "%s:%s:%s", oldpath, p1, p2);

            tmp = strdup(buf);
            setenv("LD_LIBRARY_PATH", tmp, 1);

            log_debug("Invoking w/ LD_LIBRARY_PATH=%s", getenv("LD_LIBRARY_PATH"));
        }

        /* execve needs a full path */
        ret = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (ret <= 0)
            strcpy(buf, argv[0]);
        else
            buf[ret] = '\0';

        filename = buf;

        argv[0] = args->procname;
        execve(filename, argv, environ);
        log_error("Cannot execute JSVC executor process (%s)", filename);
        return 1;
    }
    log_debug("Running w/ LD_LIBRARY_PATH=%s", getenv("LD_LIBRARY_PATH"));
#endif /* ifdef OS_LINUX */

    /* If we have to detach, let's do it now */
    if (args->dtch == true) {
        pid = fork();
        if (pid == -1) {
            log_error("Cannot detach from parent process");
            return 1;
        }
        /* If we're in the parent process */
        if (pid != 0) {
            if (args->wait >= 10)
                return wait_child(args, pid);
            else
                return 0;
        }
#ifndef NO_SETSID
        setsid();
#endif
    }

    if (chdir(args->cwd)) {
        log_error("ERROR: jsvc was unable to " "change directory to: %s", args->cwd);
    }
    /*
     * umask() uses inverse logic; bits are CLEAR for allowed access.
     */
    if (~args->umask & 0022) {
        log_error("NOTICE: jsvc umask of %03o allows "
                  "write permission to group and/or other", args->umask);
    }
    envmask = umask(args->umask);
    set_output(args->outfile, args->errfile, args->redirectstdin, args->procname);
    log_debug("Switching umask from %03o to %03o", envmask, args->umask);
    res = run_controller(args, data, uid, gid);
    if (logger_pid != 0) {
        kill(logger_pid, SIGTERM);
    }

    return res;
}

static int run_controller(arg_data *args, home_data *data, uid_t uid, gid_t gid)
{
    pid_t pid = 0;
    int restarts = 0;
    struct sigaction act;

    controller_pid = getpid();

    /*
     * Install signal handlers for the parent process.
     * These will be replaced in the child process.
     */
    memset(&act, '\0', sizeof(act));
    act.sa_handler = controller;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART | SA_NOCLDSTOP | SA_SIGINFO;

    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGUSR2, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);

    /* We have to fork: this process will become the controller and the other
       will be the child */
    while ((pid = fork()) != -1) {
        time_t laststart;
        int status = 0;
        /* We forked (again), if this is the child, we go on normally */
        if (pid == 0)
            exit(child(args, data, uid, gid));
        laststart = time(NULL);

        /* We are in the controller, we have to forward all interesting signals
           to the child, and wait for it to die */
        controlled = pid;

#ifdef OS_CYGWIN
        SetTerm(cygwincontroller);
#endif

        while (waitpid(pid, &status, 0) != pid) {
            /* Wait for process */
        }

        /* The child must have exited cleanly */
        if (WIFEXITED(status)) {
            status = WEXITSTATUS(status);

            /* Delete the pid file */
            if (args->vers != true && args->chck != true && status != 122)
                remove_pid_file(args, pid);

            /* If the child got out with 123 he wants to be restarted */
            /* See java_abort123 (we use this return code to restart when the JVM aborts) */
            if (!stopping) {
                if (status == 123) {
                    if (args->restarts == 0) {
                        log_debug("Service failure, restarts disabled");
                        return 1;
                    }
                    if (args->restarts != -1 && args->restarts <= restarts) {
                        log_debug("Service failure, restart limit reached, aborting");
                        return 1;
                    }
                    log_debug("Reloading service");
                    restarts++;
                    /* prevent looping */
                    if (laststart + 60 > time(NULL)) {
                        log_debug("Waiting 60 s to prevent looping");
                        sleep(60);
                    }
                    continue;
                }
            }
            /* If the child got out with 0 he is shutting down */
            if (status == 0) {
                log_debug("Service shut down");
                return 0;
            }
            /* Otherwise we don't rerun it */
            log_error("Service exit with a return value of %d", status);
            return 1;

        }
        else {
            if (WIFSIGNALED(status)) {
                log_error("Service killed by signal %d", WTERMSIG(status));
                /* prevent looping */
                if (!stopping) {
                    if (laststart + 60 > time(NULL)) {
                        log_debug("Waiting 60 s to prevent looping");
                        sleep(60);
                    }
                    /* Normal or user controlled termination, reset restart counter */
                    restarts = 0;
                    continue;
                }
            }
            log_error("Service did not exit cleanly", status);
            return 1;
        }
    }

    /* Got out of the loop? A fork() failed then. */
    log_error("Cannot decouple controller/child processes");
    return 1;

}

void main_reload(void)
{
    log_debug("Killing self with HUP signal");
    kill(controlled, SIGHUP);
}

void main_shutdown(void)
{
    log_debug("Killing self with TERM signal");
    kill(controlled, SIGTERM);
}
