/*
   Licensed to the Apache Software Foundation (ASF) under one or more
   contributor license agreements.  See the NOTICE file distributed with
   this work for additional information regarding copyright ownership.
   The ASF licenses this file to You under the Apache License, Version 2.0
   (the "License"); you may not use this file except in compliance with
   the License.  You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
/* @version $Id$ */
#include "jsvc.h"

#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#ifdef OS_LINUX
#include <sys/prctl.h>
#include <sys/syscall.h>
#define _LINUX_FS_H 
#include <linux/capability.h>
#endif
#include <time.h>

#ifdef OS_CYGWIN
#include <sys/fcntl.h>
#define F_ULOCK 0       /* Unlock a previously locked region */
#define F_LOCK  1       /* Lock a region for exclusive use */
#endif

extern char **environ;

static mode_t envmask; /* mask to create the files */

pid_t controlled=0; /* the child process pid */
static bool stopping=false;
static bool doreload=false;
static void (*handler_int)(int)=NULL;
static void (*handler_hup)(int)=NULL;
static void (*handler_trm)(int)=NULL;

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

static void handler(int sig) {
    switch (sig) {
        case SIGTERM: {
            log_debug("Caught SIGTERM: Scheduling a shutdown");
            if (stopping==true) {
                log_error("Shutdown or reload already scheduled");
            } else {
                stopping=true;
            }
            break;
        }

        case SIGINT: {
            log_debug("Caught SIGINT: Scheduling a shutdown");
            if (stopping==true) {
                log_error("Shutdown or reload already scheduled");
            } else {
                stopping=true;
            }
            break;
        }

        case SIGHUP: {
            log_debug("Caught SIGHUP: Scheduling a reload");
            if (stopping==true) {
                log_error("Shutdown or reload already scheduled");
            } else {
                stopping=true;
                doreload=true;
            }
            break;
        }

        default: {
            log_debug("Caught unknown signal %d",sig);
            break;
        }
    }
}

/* user and group */
static int set_user_group(char *user, int uid, int gid)
{
    if (user!=NULL) {
        if (setgid(gid)!=0) {
            log_error("Cannot set group id for user '%s'",user);
            return(-1);
        }
        if (initgroups(user, gid)!=0) {
            if (getuid()!= uid) {
                log_error("Cannot set supplement group list for user '%s'",user);
                return(-1);
            } else
                log_debug("Cannot set supplement group list for user '%s'",user);
        }
        if (getuid() == uid) {
            log_debug("No need to change user to '%s'!",user);
             return(0);
        }
        if (setuid(uid)!=0) {
            log_error("Cannot set user id for user '%s'",user);
            return(-1);
        }
        log_debug("user changed to '%s'",user);
    }
    return(0);
}
/* Set linux capability, user and group */
#ifdef OS_LINUX
/* CAPSALL is to allow to read/write at any location */
#define CAPSALL (1 << CAP_NET_BIND_SERVICE)+ \
                (1 << CAP_SETUID)+ \
                (1 << CAP_SETGID)+ \
                (1 << CAP_DAC_READ_SEARCH)+ \
                (1 << CAP_DAC_OVERRIDE)
#define CAPSMAX (1 << CAP_NET_BIND_SERVICE)+ \
                (1 << CAP_DAC_READ_SEARCH)+ \
                (1 << CAP_DAC_OVERRIDE)
/* That a more reasonable configuration */
#define CAPS    (1 << CAP_NET_BIND_SERVICE)+ \
                (1 << CAP_DAC_READ_SEARCH)+ \
                (1 << CAP_SETUID)+ \
                (1 << CAP_SETGID)
/* probably the only one Java could use */
#define CAPSMIN (1 << CAP_NET_BIND_SERVICE)+ \
                (1 << CAP_DAC_READ_SEARCH)

static int set_caps(int caps)
{
    struct __user_cap_header_struct caphead;
    struct __user_cap_data_struct cap;
 
    memset(&caphead, 0, sizeof caphead);
    caphead.version = _LINUX_CAPABILITY_VERSION;
    caphead.pid = 0;
    memset(&cap, 0, sizeof cap);
    cap.effective = caps;
    cap.permitted = caps;
    cap.inheritable = caps;
    if (syscall(__NR_capset, &caphead, &cap) < 0) {
        log_error("syscall failed in set_caps");
        return(-1);
    }
    return(0);
}
static int linuxset_user_group(char *user, int uid, int gid)
{
    int caps_set = 0;
    /* set capabilities enough for binding port 80 setuid/getuid */
    if (getuid() == 0) {
        if (set_caps(CAPS)!=0) {
            if (getuid()!= uid) {
                log_error("set_caps(CAPS) failed");
                return(-1);
            }
            log_debug("set_caps(CAPS) failed");
        }
        /* make sure they are kept after setuid */ 
        if (prctl(PR_SET_KEEPCAPS,1,0,0,0) < 0) {
            log_error("prctl failed in linuxset_user_group");
            return(-1);
        }
        caps_set = 1;
    }

    /* set setuid/getuid */
    if (set_user_group(user,uid,gid)!=0) {
        log_error("set_user_group failed in linuxset_user_group");
        return(-1);
    }

    if (caps_set) {
        /* set capability to binding port 80 read conf */
        if (set_caps(CAPSMIN)!=0) {
            if (getuid()!= uid) {
                log_error("set_caps(CAPSMIN) failed");
                return(-1);
            }
            log_debug("set_caps(CAPSMIN) failed");
        }
    }

    return(0);
}
#endif


static bool checkuser(char *user, uid_t *uid, gid_t *gid) {
    struct passwd *pwds=NULL;
    int status=0;
    pid_t pid=0;

    /* Do we actually _have_ to switch user? */
    if (user==NULL) return(true);

    pwds=getpwnam(user);
    if (pwds==NULL) {
        log_error("Invalid user name '%s' specified",user);
        return(false);
    }

    *uid=pwds->pw_uid;
    *gid=pwds->pw_gid;

    /* Validate the user name in another process */
    pid=fork();
    if (pid==-1) {
        log_error("Cannot validate user name");
        return(false);
    }

    /* If we're in the child process, let's validate */
    if (pid==0) {
        if (set_user_group(user,*uid,*gid)!=0)
            exit(1);
        /* If we got here we switched user/group */
        exit(0);
    }

    while (waitpid(pid,&status,0)!=pid);

    /* The child must have exited cleanly */
    if (WIFEXITED(status)) {
        status=WEXITSTATUS(status);

        /* If the child got out with 0 the user is ok */
        if (status==0) {
            log_debug("User '%s' validated",user);
            return(true);
        }
    }

    log_error("Error validating user '%s'",user);
    return(false);
}

#ifdef OS_CYGWIN
static void cygwincontroller(void) {
    raise(SIGTERM);
}
#endif
static void controller(int sig) {
    switch (sig) {
        case SIGTERM:
        case SIGINT:
        case SIGHUP:
            log_debug("Forwarding signal %d to process %d",sig,controlled);
            kill(controlled,sig);
            signal(sig,controller);
            break;
        default:
            log_debug("Caught unknown signal %d",sig);
            break;
    }
}
/*
 * Return the address of the current signal handler and set the new one.
 */
static void * signal_set(int sig, void * newHandler) {
    void *hand;

    hand=signal(sig,newHandler);
#ifdef SIG_ERR
    if (hand==SIG_ERR)
        hand=NULL;
#endif
    if (hand==handler || hand==controller)
        hand=NULL;
    return(hand);
}

/*
 * Check pid and if still running
 */

static int check_pid(arg_data *args) {
    int fd;
    FILE *pidf;
    char buff[80];
    pid_t pidn=getpid();
    int i,pid;

    fd = open(args->pidf,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
    if (fd<0) {
        log_error("Cannot open PID file %s, PID is %d",args->pidf,pidn);
        return(-1);
    } else {
        lockf(fd,F_LOCK,0);
        i = read(fd,buff,sizeof(buff));
        if (i>0) {
            buff[i] = '\0';
            pid = atoi(buff);
            if (kill(pid, 0)==0) {
                log_error("Still running according to PID file %s, PID is %d",args->pidf,pid);
                lockf(fd,F_ULOCK,0);
                close(fd);
                return(122);
            }
        }

        /* skip writing the pid file if version or check */
        if (args->vers!=true && args->chck!=true) {
            lseek(fd, SEEK_SET, 0);
            pidf = fdopen(fd,"r+");
            fprintf(pidf,"%d\n",(int)getpid());
            fflush(pidf);
            lockf(fd,F_ULOCK,0);
            fclose(pidf);
            close(fd);
        } else {
            lockf(fd,F_ULOCK,0);
            close(fd);
        }
    }
    return(0);
}

/*
 * read the pid from the pidfile
 */
static int get_pidf(arg_data *args) {
    int fd;
    int i;
    char buff[80];

    fd = open(args->pidf, O_RDONLY, 0);
    log_debug("get_pidf: %d in %s", fd, args->pidf);
    if (fd<0)
        return(-1); /* something has gone wrong the JVM has stopped */
    lockf(fd,F_LOCK,0);
    i = read(fd,buff,sizeof(buff));
    lockf(fd,F_ULOCK,0);
    close(fd);
    if (i>0) {
        buff[i] = '\0';
        i = atoi(buff);
        log_debug("get_pidf: pid %d", i);
        if (kill(i, 0)==0)
            return(i);
    }
    return(-1);
}

/*
 * Check temporatory file created by controller
 * /tmp/pid.jsvc_up
 * Notes:
 * we fork several times
 * 1 - to be a daemon before the setsid(), the child is the controler process.
 * 2 - to start the JVM in the child process. (whose pid is stored in pidfile).
 */
static int check_tmp_file(arg_data *args) {
    int pid;
    char buff[80];
    int fd;
    pid = get_pidf(args);
    if (pid<0)
        return(-1);
    sprintf(buff,"/tmp/%d.jsvc_up", pid);
    log_debug("check_tmp_file: %s", buff);
    fd = open(buff, O_RDONLY);
    if (fd<0)
        return(-1);
    close(fd);
    return(0);
}
static void create_tmp_file(arg_data *args) {
    char buff[80];
    int fd;
    sprintf(buff,"/tmp/%d.jsvc_up", (int) getpid());
    log_debug("create_tmp_file: %s", buff);
    fd = open(buff, O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
    if (fd<0)
        return;
    close(fd);
}
static void remove_tmp_file(arg_data *args) {
    char buff[80];
    sprintf(buff,"/tmp/%d.jsvc_up", (int) getpid());
    log_debug("remove_tmp_file: %s", buff);
    unlink(buff);
}

/*
 * wait until jsvc create the I am ready file
 * pid is the controller and args->pidf the JVM itself.
 */
static int wait_child(arg_data *args, int pid) {
    int count=10;
    bool havejvm=false;
    int fd;
    char buff[80];
    int i, status, waittime;
    log_debug("wait_child %d", pid);
    waittime = args->wait/10;
    if (waittime>10) {
        count = waittime;
        waittime = 10;
    }
    while (count>0) {
        sleep(1);
        /* check if the controler is still running */
        if (waitpid(pid,&status,WNOHANG)==pid) {
            if (WIFEXITED(status))
                return(WEXITSTATUS(status));
            else
                return(1);
        }

        /* check if the pid file process exists */
        fd = open(args->pidf, O_RDONLY);
        if (fd<0 && havejvm)
            return(1); /* something has gone wrong the JVM has stopped */
        lockf(fd,F_LOCK,0);
        i = read(fd,buff,sizeof(buff));
        lockf(fd,F_ULOCK,0);
        close(fd);
        if (i>0) {
            buff[i] = '\0';
            i = atoi(buff);
            if (kill(i, 0)==0) {
                /* the JVM process has started */
                havejvm=true;
                if (check_tmp_file(args)==0) {
                    /* the JVM is started */
                    if (waitpid(pid,&status,WNOHANG)==pid) {
                        if (WIFEXITED(status))
                            return(WEXITSTATUS(status));
                        else
                            return(1);
                    }
                    return(0); /* ready JVM started */
                }
            }
        }
        sleep(waittime);
        count--;
    }
    return(1); /* It takes more than the wait time to start, something must be wrong */
}

/*
 * stop the running jsvc
 */
static int stop_child(arg_data *args) {
    int pid=get_pidf(args);
    int count=10;
    if (pid>0) {
        /* kill the process and wait until the pidfile has been removed by the controler */
        kill(pid,SIGTERM);
        while (count>0) {
            sleep(6);
            pid=get_pidf(args);
            if (pid<=0)
                return(0); /* JVM has stopped */
            count--;
        }
    }
    return(-1);
}

/*
 * child process logic.
 */

static int child(arg_data *args, home_data *data, uid_t uid, gid_t gid) {
    int ret=0;

    /* check the pid file */
    ret = check_pid(args); 
    if (args->vers!=true && args->chck!=true) {
        if (ret==122)
            return(ret);
        if (ret<0)
            return(ret);
    }

    /* create a new process group to prevent kill 0 killing the monitor process */
#if defined(OS_FREEBSD) || defined(OS_DARWIN)
    setpgid(0,0);
#else
    setpgrp();
#endif

#ifdef OS_LINUX
    /* setuid()/setgid() only apply the current thread so we must do it now */
    if (linuxset_user_group(args->user,uid,gid)!=0)
        return(4);
#endif
    /* Initialize the Java VM */
    if (java_init(args,data)!=true) {
        log_debug("java_init failed");
        return(1);
    } else
        log_debug("java_init done");

    /* Check wether we need to dump the VM version */
    if (args->vers==true) {
        if (java_version()!=true) {
            return(-1);
        } else return(0);
    }

    /* Do we have to do a "check-only" initialization? */
    if (args->chck==true) {
        if (java_check(args)!=true) return(2);
        printf("Service \"%s\" checked successfully\n",args->clas);
        return(0);
    }

    /* Load the service */
    if (java_load(args)!=true) {
        log_debug("java_load failed");
        return(3);
    } else
        log_debug("java_load done");

    /* Downgrade user */
#ifdef OS_LINUX
    if (set_caps(0)!=0) {
        log_debug("set_caps (0) failed");
        return(4);
    }
#else
    if (set_user_group(args->user,uid,gid)!=0)
        return(4);
#endif

    /* Start the service */
    umask(envmask);
    if (java_start()!=true) {
        log_debug("java_start failed");
        return(5);
    } else
        log_debug("java_start done");

    /* Install signal handlers */
    handler_hup=signal_set(SIGHUP,handler);
    handler_trm=signal_set(SIGTERM,handler);
    handler_int=signal_set(SIGINT,handler);
    controlled = getpid();
    log_debug("Waiting for a signal to be delivered");
    create_tmp_file(args);
    while (!stopping) {
#if defined(OSD_POSIX) || defined(HAVE_KAFFEVM)
        java_sleep(60);
        /* pause(); */
#else
        sleep(60); /* pause() not threadsafe */
#endif
    }
    remove_tmp_file(args);
    log_debug("Shutdown or reload requested: exiting");

    /* Stop the service */
    if (java_stop()!=true) return(6);

    if (doreload==true) ret=123;
    else ret=0;

    /* Destroy the service */
    java_destroy();

    /* Destroy the Java VM */
    if (JVM_destroy(ret)!=true) return(7);

    return(ret);
}

/*
 * freopen close the file first and then open the new file
 * that is not very good if we are try to trace the output
 * note the code assumes that the errors are configuration errors.
 */
static FILE *loc_freopen(char *outfile, char *mode, FILE *stream)
{
    FILE *ftest;
    ftest = fopen(outfile,mode);
    if (ftest == NULL) {
      fprintf(stderr,"Unable to redirect to %s\n", outfile);
      return(stream);
    }
    fclose(ftest);
    return(freopen(outfile,mode,stream));
}

/**
 *  Redirect stdin, stdout, stderr.
 */
static void set_output(char *outfile, char *errfile, bool redirectstdin) {
    if (redirectstdin==true) {
        freopen("/dev/null", "r", stdin);
    }

    log_debug("redirecting stdout to %s and stderr to %s",outfile,errfile);

    /* make sure the debug goes out */
    if (log_debug_flag==true && strcmp(errfile,"/dev/null") == 0)
      return;

    /* Handle malicious case here */
    if(strcmp(outfile, "&2") == 0 && strcmp(errfile,"&1") == 0) {
      outfile="/dev/null";
    }
    if(strcmp(outfile, "&2") != 0) {
      loc_freopen(outfile, "a", stdout);
    }

    if(strcmp(errfile,"&1") != 0) {
      loc_freopen(errfile, "a", stderr);
    } else {
      close(2);
      dup(1);
    }
    if(strcmp(outfile, "&2") == 0) {
      close(1);
      dup(2);
    }
}

int main(int argc, char *argv[]) {
    arg_data *args=NULL;
    home_data *data=NULL;
    int status=0;
    pid_t pid=0;
    uid_t uid=0;
    gid_t gid=0;
    time_t laststart;

    /* Parse command line arguments */
    args=arguments(argc,argv);
    if (args==NULL) return(1);

    /* Stop running jsvc if required */
    if (args->stop==true)
        return(stop_child(args));

    /* Let's check if we can switch user/group IDs */
    if (checkuser(args->user, &uid, &gid)==false) return(1);

    /* Retrieve JAVA_HOME layout */
    data=home(args->home);
    if (data==NULL) return(1);

    /* Check for help */
    if (args->help==true) {
        help(data);
        return(0);
    }

#ifdef OS_LINUX
    /* On some UNIX operating systems, we need to REPLACE this current
       process image with another one (thru execve) to allow the correct
       loading of VMs (notably this is for Linux). Set, replace, and go. */
    if (strcmp(argv[0],args->procname)!=0) {
        char *oldpath=getenv("LD_LIBRARY_PATH");
        char *libf=java_library(args,data);
        char *filename;
        char buf[2048];
        int  ret;
        char *tmp=NULL;
        char *p1=NULL;
        char *p2=NULL;

        /*
         * There is no need to change LD_LIBRARY_PATH
         * if we were not able to find a path to libjvm.so
         * (additionaly a strdup(NULL) cores dump on my machine).
         */
        if (libf!=NULL) {
            p1=strdup(libf);
            tmp=strrchr(p1,'/');
            if (tmp!=NULL) tmp[0]='\0';

            p2=strdup(p1);
            tmp=strrchr(p2,'/');
            if (tmp!=NULL) tmp[0]='\0';

            if (oldpath==NULL) snprintf(buf,2048,"%s:%s",p1,p2);
            else snprintf(buf,2048,"%s:%s:%s",oldpath,p1,p2);

            tmp=strdup(buf);
            setenv("LD_LIBRARY_PATH",tmp,1);

            log_debug("Invoking w/ LD_LIBRARY_PATH=%s",getenv("LD_LIBRARY_PATH"));
        }

        /* execve needs a full path */
        ret = readlink("/proc/self/exe",buf,sizeof(buf)-1);
        if (ret<=0)
          strcpy(buf,argv[0]);
        else
          buf[ret]='\0';
  
        filename=buf;

        argv[0]=args->procname;
        execve(filename,argv,environ);
        log_error("Cannot execute JSVC executor process (%s)",filename);
        return(1);
    }
    log_debug("Running w/ LD_LIBRARY_PATH=%s",getenv("LD_LIBRARY_PATH"));
#endif /* ifdef OS_LINUX */

    /* If we have to detach, let's do it now */
    if (args->dtch==true) {
        pid=fork();
        if (pid==-1) {
            log_error("Cannot detach from parent process");
            return(1);
        }
        /* If we're in the parent process */
        if (pid!=0) {
            if (args->wait>=10)
                return(wait_child(args,pid));
            else
                return(0);
        }
#ifndef NO_SETSID
        setsid();
#endif
    }

    envmask = umask(0077);
    set_output(args->outfile, args->errfile, args->redirectstdin);

    /* We have to fork: this process will become the controller and the other
       will be the child */
    while ((pid=fork())!=-1) {
        /* We forked (again), if this is the child, we go on normally */
        if (pid==0) exit(child(args,data,uid,gid));
        laststart = time(NULL);

        /* We are in the controller, we have to forward all interesting signals
           to the child, and wait for it to die */
        controlled=pid;
#ifdef OS_CYGWIN
       SetTerm(cygwincontroller);
#endif
        signal(SIGHUP,controller);
        signal(SIGTERM,controller);
        signal(SIGINT,controller);

        while (waitpid(pid,&status,0)!=pid);

        /* The child must have exited cleanly */
        if (WIFEXITED(status)) {
            status=WEXITSTATUS(status);

            /* Delete the pid file */
            if (args->vers!=true && args->chck!=true && status!=122)
                unlink(args->pidf);

            /* If the child got out with 123 he wants to be restarted */
            /* See java_abort123 (we use this return code to restart when the JVM aborts) */
            if (status==123) {
                log_debug("Reloading service");
                /* prevent looping */
                if (laststart+60>time(NULL)) {
                  log_debug("Waiting 60 s to prevent looping");
                  sleep(60);
                } 
                continue;
            }
            /* If the child got out with 0 he is shutting down */
            if (status==0) {
                log_debug("Service shut down");
                return(0);
            }
            /* Otherwise we don't rerun it */
            log_error("Service exit with a return value of %d",status);
            return(1);

        } else {
            if (WIFSIGNALED(status)) {
                log_error("Service killed by signal %d",WTERMSIG(status));
                /* prevent looping */
                if (laststart+60>time(NULL)) {
                  log_debug("Waiting 60 s to prevent looping");
                  sleep(60);
                } 
                continue;
            }
            log_error("Service did not exit cleanly",status);
            return(1);
        }
    }

    /* Got out of the loop? A fork() failed then. */
    log_error("Cannot decouple controller/child processes");
    return(1);

}

void main_reload(void) {
    log_debug("Killing self with HUP signal");
    kill(controlled,SIGHUP);
}

void main_shutdown(void) {
    log_debug("Killing self with TERM signal");
    kill(controlled,SIGTERM);
}

