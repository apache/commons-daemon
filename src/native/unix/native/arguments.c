/* ========================================================================= *
 *                                                                           *
 *                 The Apache Software License,  Version 1.1                 *
 *                                                                           *
 *          Copyright (c) 1999-2003 The Apache Software Foundation.          *
 *                           All rights reserved.                            *
 *                                                                           *
 * ========================================================================= *
 *                                                                           *
 * Redistribution and use in source and binary forms,  with or without modi- *
 * fication, are permitted provided that the following conditions are met:   *
 *                                                                           *
 * 1. Redistributions of source code  must retain the above copyright notice *
 *    notice, this list of conditions and the following disclaimer.          *
 *                                                                           *
 * 2. Redistributions  in binary  form  must  reproduce the  above copyright *
 *    notice,  this list of conditions  and the following  disclaimer in the *
 *    documentation and/or other materials provided with the distribution.   *
 *                                                                           *
 * 3. The end-user documentation  included with the redistribution,  if any, *
 *    must include the following acknowlegement:                             *
 *                                                                           *
 *       "This product includes  software developed  by the Apache  Software *
 *        Foundation <http://www.apache.org/>."                              *
 *                                                                           *
 *    Alternately, this acknowlegement may appear in the software itself, if *
 *    and wherever such third-party acknowlegements normally appear.         *
 *                                                                           *
 * 4. The names  "The  Jakarta  Project",  "WebApp",  and  "Apache  Software *
 *    Foundation"  must not be used  to endorse or promote  products derived *
 *    from this  software without  prior  written  permission.  For  written *
 *    permission, please contact <apache@apache.org>.                        *
 *                                                                           *
 * 5. Products derived from this software may not be called "Apache" nor may *
 *    "Apache" appear in their names without prior written permission of the *
 *    Apache Software Foundation.                                            *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES *
 * INCLUDING, BUT NOT LIMITED TO,  THE IMPLIED WARRANTIES OF MERCHANTABILITY *
 * AND FITNESS FOR  A PARTICULAR PURPOSE  ARE DISCLAIMED.  IN NO EVENT SHALL *
 * THE APACHE  SOFTWARE  FOUNDATION OR  ITS CONTRIBUTORS  BE LIABLE  FOR ANY *
 * DIRECT,  INDIRECT,   INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR  CONSEQUENTIAL *
 * DAMAGES (INCLUDING,  BUT NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE GOODS *
 * OR SERVICES;  LOSS OF USE,  DATA,  OR PROFITS;  OR BUSINESS INTERRUPTION) *
 * HOWEVER CAUSED AND  ON ANY  THEORY  OF  LIABILITY,  WHETHER IN  CONTRACT, *
 * STRICT LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN *
 * ANY  WAY  OUT OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF  ADVISED  OF THE *
 * POSSIBILITY OF SUCH DAMAGE.                                               *
 *                                                                           *
 * ========================================================================= *
 *                                                                           *
 * This software  consists of voluntary  contributions made  by many indivi- *
 * duals on behalf of the  Apache Software Foundation.  For more information *
 * on the Apache Software Foundation, please see <http://www.apache.org/>.   *
 *                                                                           *
 * ========================================================================= */

/* @version $Id: arguments.c,v 1.2 2003/12/31 04:58:31 billbarker Exp $ */
#include "jsvc.h"

/* Return the argument of a command line option */
static char *optional(int argc, char *argv[], int argi) {

    argi++;
    if (argi>=argc) return(NULL);
    if (argv[argi]==NULL) return(NULL);
    if (argv[argi][0]=='-') return(NULL);
    return(strdup(argv[argi]));
}

/* Parse command line arguments */
static arg_data *parse(int argc, char *argv[]) {
    arg_data *args=NULL;
    char *temp=NULL;
    char *cmnd=NULL;
    int tlen=0;
    int x=0;

    /* Create the default command line arguments */
    args=(arg_data *)malloc(sizeof(arg_data));
    args->pidf="/var/run/jsvc.pid"; /* The default PID file */
    args->user=NULL;            /* No user switching by default */
    args->dtch=true;            /* Do detach from parent */
    args->vers=false;           /* Don't display version */
    args->help=false;           /* Don't display help */
    args->chck=false;           /* Don't do a check-only startup */
    args->install=false;        /* Don't install as a service */
    args->remove=false;         /* Don't remove the installed service */
    args->service=false;        /* Don't run as a service */
    args->name=NULL;            /* No VM version name */
    args->home=NULL;            /* No default JAVA_HOME */
    args->onum=0;               /* Zero arguments, but let's have some room */
    args->opts=(char **)malloc(argc*sizeof(char *));
    args->clas=NULL;            /* No class predefined */
    args->anum=0;               /* Zero class specific arguments but make room*/
    args->outfile="/dev/null";   /* Swallow by default */
    args->errfile="/dev/null";   /* Swallow by default */
    args->args=(char **)malloc(argc*sizeof(char *));

    /* Set up the command name */
    cmnd=strrchr(argv[0],'/');
    if (cmnd==NULL) cmnd=argv[0];
    else cmnd++;
    log_prog=strdup(cmnd);

    /* Iterate thru command line arguments */
    for (x=1; x<argc; x++) {

        if ((strcmp(argv[x],"-cp")==0)||(strcmp(argv[x],"-classpath")==0)) {
            temp=optional(argc,argv,x++);
            if (temp==NULL) {
                log_error("Invalid classpath specified");
                return(NULL);
            }
            tlen=strlen(temp)+20;
            args->opts[args->onum]=(char *)malloc(tlen*sizeof(char));
            sprintf(args->opts[args->onum],"-Djava.class.path=%s",temp);
            args->onum++;

        } else if (strcmp(argv[x],"-jvm")==0) {
            args->name=optional(argc,argv,x++);
            if (args->name==NULL) {
                log_error("Invalid Java VM name specified");
                return(NULL);
            }

        } else if (strcmp(argv[x],"-home")==0) {
            args->home=optional(argc,argv,x++);
            if (args->home==NULL) {
                log_error("Invalid Java Home specified");
                return(NULL);
            }

        } else if (strcmp(argv[x],"-user")==0) {
            args->user=optional(argc,argv,x++);
            if (args->user==NULL) {
                log_error("Invalid user name specified");
                return(NULL);
            }

        } else if (strcmp(argv[x],"-version")==0) {
            args->vers=true;
            args->dtch=false;

        } else if ((strcmp(argv[x],"-?")==0)||(strcmp(argv[x],"-help")==0)
                   ||(strcmp(argv[x],"--help")==0)) {
            args->help=true;
            args->dtch=false;
            return(args);

        } else if (strcmp(argv[x],"-X")==0) {
            log_error("Option -X currently unsupported");
            log_error("Please use \"java -X\" to see your extra VM options");

        } else if (strcmp(argv[x],"-debug")==0) {
            log_debug_flag=true;

        } else if (strcmp(argv[x],"-check")==0) {
            args->chck=true;
            args->dtch=false;

        } else if (strcmp(argv[x],"-nodetach")==0) {
            args->dtch=false;

        } else if (strcmp(argv[x],"-service")==0) {
            args->service=true;

        } else if (strcmp(argv[x],"-install")==0) {
            args->install=true;

        } else if (strcmp(argv[x],"-remove")==0) {
            args->remove=true;

        } else if (strcmp(argv[x],"-pidfile")==0) {
            args->pidf=optional(argc,argv,x++);
            if (args->pidf==NULL) {
                log_error("Invalid PID file specified");
                return(NULL);
            }

        } else if(strcmp(argv[x],"-outfile") == 0) {
            args->outfile=optional(argc, argv, x++);
            if(args->outfile == NULL) {
                log_error("Invalid Output File specified");
                return(NULL);
            }
        } else if(strcmp(argv[x],"-errfile") == 0) {
            args->errfile=optional(argc, argv, x++);
            if(args->errfile == NULL) {
                log_error("Invalid Error File specified");
                return(NULL);
            }
        }else if (strstr(argv[x],"-verbose")==argv[x]) {
            args->opts[args->onum++]=strdup(argv[x]);

        } else if (strcmp(argv[x],"-D")==0) {
            log_error("Parameter -D must be followed by <name>=<value>");
            return(NULL);

        } else if (strstr(argv[x],"-D")==argv[x]) {
            temp=strchr(argv[x],'=');
            if (temp==NULL) {
                log_error("Parameter -D must contain one '=' character");
                return(NULL);
            }
            if (temp==argv[x]+2) {
                log_error("A property name must be specified before '='");
                return(NULL);
            }
            args->opts[args->onum++]=strdup(argv[x]);

        } else if (strstr(argv[x],"-X")==argv[x]) {
            args->opts[args->onum++]=strdup(argv[x]);

        } else if (strstr(argv[x],"-")==argv[x]) {
            log_error("Invalid option %s",argv[x]);
            return(NULL);

        } else {
            args->clas=strdup(argv[x]);
            break;
        }
    }

    if (args->clas==NULL && args->remove==false) {
        log_error("No class specified");
        return(NULL);
    }

    x++;
    while (x<argc) args->args[args->anum++]=strdup(argv[x++]);

    return(args);
}
static char *IsYesNo(bool par)
{
    switch (par) {
        case false: return("No");
        case true:  return("Yes");
    }
    return ("[Error]");
}
static char *IsTrueFalse(bool par)
{
    switch (par) {
        case false: return("False");
        case true:  return("True");
    }
    return ("[Error]");
}
static char *IsEnabledDisabled(bool par)
{
    switch (par) {
        case true:   return("Enabled");
        case false:  return("Disabled");
    }
    return ("[Error]");
}

/* Main entry point: parse command line arguments and dump them */
arg_data *arguments(int argc, char *argv[]) {
    arg_data *args=parse(argc,argv);
    int x=0;

    if (args==NULL) {
        log_error("Cannot parse command line arguments");
        return(NULL);
    }

    if (log_debug_flag==true) {
        char *temp;

        log_debug("+-- DUMPING PARSED COMMAND LINE ARGUMENTS --------------");

        log_debug("| Detach:          %s",IsTrueFalse(args->dtch));

        log_debug("| Show Version:    %s",IsYesNo(args->vers));

        log_debug("| Show Help:       %s",IsYesNo(args->help));

        log_debug("| Check Only:      %s",IsEnabledDisabled(args->chck));

        log_debug("| Run as service:  %s",IsYesNo(args->service));

        log_debug("| Install service: %s",IsYesNo(args->install));

        log_debug("| Remove service:  %s",IsYesNo(args->remove));

        log_debug("| JVM Name:        \"%s\"",PRINT_NULL(args->name));
        log_debug("| Java Home:       \"%s\"",PRINT_NULL(args->home));
        log_debug("| PID File:        \"%s\"",PRINT_NULL(args->pidf));
        log_debug("| User Name:       \"%s\"",PRINT_NULL(args->user));

        log_debug("| Extra Options:   %d",args->onum);
        for (x=0; x<args->onum; x++) log_debug("|   \"%s\"",args->opts[x]);

        log_debug("| Class Invoked:   \"%s\"",PRINT_NULL(args->clas));

        log_debug("| Class Arguments: %d",args->anum);
        for (x=0; x<args->anum; x++)log_debug("|   \"%s\"",args->args[x]);
        log_debug("+-------------------------------------------------------");
    }

    return(args);
}
