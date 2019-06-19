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
#include <limits.h>
#include <glob.h>

/* Return the argument of a command line option */
static char *optional(int argc, char *argv[], int argi)
{

    argi++;
    if (argi >= argc)
        return NULL;
    if (argv[argi] == NULL)
        return NULL;
    if (argv[argi][0] == '-')
        return NULL;
    return strdup(argv[argi]);
}

static char *memstrcat(char *ptr, const char *str, const char *add)
{
    size_t nl = 1;
    int nas = ptr == NULL;
    if (ptr)
        nl += strlen(ptr);
    if (str)
        nl += strlen(str);
    if (add)
        nl += strlen(add);
    ptr = (char *)realloc(ptr, nl);
    if (ptr) {
        if (nas)
            *ptr = '\0';
        if (str)
            strcat(ptr, str);
        if (add)
            strcat(ptr, add);
    }
    return ptr;
}

static char *eval_ppath(char *strcp, const char *pattern)
{
    glob_t globbuf;
    char jars[PATH_MAX + 1];

    if (strlen(pattern) > (sizeof(jars) - 5)) {
        return memstrcat(strcp, pattern, NULL);
    }
    strcpy(jars, pattern);
    strcat(jars, ".jar");
    memset(&globbuf, 0, sizeof(glob_t));
    if (glob(jars, GLOB_ERR, NULL, &globbuf) == 0) {
        size_t n;
        for (n = 0; n < globbuf.gl_pathc - 1; n++) {
            strcp = memstrcat(strcp, globbuf.gl_pathv[n], ":");
            if (strcp == NULL) {
                globfree(&globbuf);
                return NULL;
            }
        }
        strcp = memstrcat(strcp, globbuf.gl_pathv[n], NULL);
        globfree(&globbuf);
    }
    return strcp;
}

#define JAVA_CLASSPATH      "-Djava.class.path="
/**
 * Call glob on each PATH like string path.
 * Glob is called only if the part ends with asterisk in which
 * case asterisk is replaced by *.jar when searching
 */
static char *eval_cpath(const char *cp)
{
    char *cpy = memstrcat(NULL, JAVA_CLASSPATH, cp);
    char *gcp = NULL;
    char *pos;
    char *ptr;

    if (!cpy)
        return NULL;
    ptr = cpy + sizeof(JAVA_CLASSPATH) - 1;;
    while ((pos = strchr(ptr, ':'))) {
        *pos = '\0';
        if (gcp)
            gcp = memstrcat(gcp, ":", NULL);
        else
            gcp = memstrcat(NULL, JAVA_CLASSPATH, NULL);
        if ((pos > ptr) && (*(pos - 1) == '*')) {
            if (!(gcp = eval_ppath(gcp, ptr))) {
                /* Error.
                 * Return the original string processed so far.
                 */
                return cpy;
            }
        }
        else
            gcp = memstrcat(gcp, ptr, NULL);
        ptr = pos + 1;
    }
    if (*ptr) {
        size_t end = strlen(ptr);
        if (gcp)
            gcp = memstrcat(gcp, ":", NULL);
        else
            gcp = memstrcat(NULL, JAVA_CLASSPATH, NULL);
        if (end > 0 && ptr[end - 1] == '*') {
            /* Last path elemet ends with star
             * Do a globbing.
             */
            gcp = eval_ppath(gcp, ptr);
        }
        else {
            /* Just add the part */
            gcp = memstrcat(gcp, ptr, NULL);
        }
    }
    /* Free the allocated copy */
    if (gcp) {
        free(cpy);
        return gcp;
    }
    else
        return cpy;
}

/* Parse command line arguments */
static arg_data *parse(int argc, char *argv[])
{
    arg_data *args = NULL;
    char *temp = NULL;
    char *cmnd = NULL;
    int x = 0;

    /* Create the default command line arguments */
    args = (arg_data *)malloc(sizeof(arg_data));
    args->pidf = "/var/run/jsvc.pid";  /* The default PID file */
    args->user = NULL;                 /* No user switching by default */
    args->dtch = true;                 /* Do detach from parent */
    args->vers = false;                /* Don't display version */
    args->help = false;                /* Don't display help */
    args->chck = false;                /* Don't do a check-only startup */
    args->stop = false;                /* Stop a running jsvc */
    args->wait = 0;                    /* Wait until jsvc has started the JVM */
    args->restarts = -1;               /* Infinite restarts by default */
    args->install = false;             /* Don't install as a service */
    args->remove = false;              /* Don't remove the installed service */
    args->service = false;             /* Don't run as a service */
    args->name = NULL;                 /* No VM version name */
    args->home = NULL;                 /* No default JAVA_HOME */
    args->onum = 0;                    /* Zero arguments, but let's have some room */
    args->clas = NULL;                 /* No class predefined */
    args->anum = 0;                    /* Zero class specific arguments but make room */
    args->cwd = "/";                   /* Use root as default */
    args->outfile = "/dev/null";       /* Swallow by default */
    args->errfile = "/dev/null";       /* Swallow by default */
    args->redirectstdin = true;        /* Redirect stdin to /dev/null by default */
    args->procname = "jsvc.exec";
#ifndef JSVC_UMASK
    args->umask = 0077;
#else
    args->umask = JSVC_UMASK;
#endif

    if (!(args->args = (char **)malloc(argc * sizeof(char *))))
        return NULL;
    if (!(args->opts = (char **)malloc(argc * sizeof(char *))))
        return NULL;

    /* Set up the command name */
    cmnd = strrchr(argv[0], '/');
    if (cmnd == NULL)
        cmnd = argv[0];
    else
        cmnd++;
    log_prog = strdup(cmnd);

    /* Iterate thru command line arguments */
    for (x = 1; x < argc; x++) {

        if (!strcmp(argv[x], "-cp") || !strcmp(argv[x], "-classpath")) {
            temp = optional(argc, argv, x++);
            if (temp == NULL) {
                log_error("Invalid classpath specified");
                return NULL;
            }
            args->opts[args->onum] = eval_cpath(temp);
            if (args->opts[args->onum] == NULL) {
                log_error("Invalid classpath specified");
                return NULL;
            }
            free(temp);
            args->onum++;

        }
        else if (!strcmp(argv[x], "-jvm")) {
            args->name = optional(argc, argv, x++);
            if (args->name == NULL) {
                log_error("Invalid Java VM name specified");
                return NULL;
            }
        }
        else if (!strcmp(argv[x], "-client")) {
            args->name = strdup("client");
        }
        else if (!strcmp(argv[x], "-server")) {
            args->name = strdup("server");
        }
        else if (!strcmp(argv[x], "-home") || !strcmp(argv[x], "-java-home")) {
            args->home = optional(argc, argv, x++);
            if (args->home == NULL) {
                log_error("Invalid Java Home specified");
                return NULL;
            }
        }
        else if (!strcmp(argv[x], "-user")) {
            args->user = optional(argc, argv, x++);
            if (args->user == NULL) {
                log_error("Invalid user name specified");
                return NULL;
            }
        }
        else if (!strcmp(argv[x], "-cwd")) {
            args->cwd = optional(argc, argv, x++);
            if (args->cwd == NULL) {
                log_error("Invalid working directory specified");
                return NULL;
            }
        }
        else if (!strcmp(argv[x], "-version")) {
            args->vers = true;
            args->dtch = false;
        }
        else if (!strcmp(argv[x], "-showversion")) {
            args->vershow = true;
        }
        else if (!strcmp(argv[x], "-?") || !strcmp(argv[x], "-help") || !strcmp(argv[x], "--help")) {
            args->help = true;
            args->dtch = false;
            return args;
        }
        else if (!strcmp(argv[x], "-X")) {
            log_error("Option -X currently unsupported");
            log_error("Please use \"java -X\" to see your extra VM options");
        }
        else if (!strcmp(argv[x], "-debug")) {
            log_debug_flag = true;
        }
        else if (!strcmp(argv[x], "-wait")) {
            temp = optional(argc, argv, x++);
            if (temp)
                args->wait = atoi(temp);
            if (args->wait < 10) {
                log_error("Invalid wait time specified (min=10)");
                return NULL;
            }
        }
        else if (!strcmp(argv[x], "-restarts")) {
            temp = optional(argc, argv, x++);
            if (temp)
                args->restarts = atoi(temp);
            if (args->restarts < -1) {
                log_error("Invalid max restarts [-1,0,...)");
                return NULL;
            }
        }
        else if (!strcmp(argv[x], "-umask")) {
            temp = optional(argc, argv, x++);
            if (temp == NULL) {
                log_error("Invalid umask specified");
                return NULL;
            }
            /* Parameter must be in octal */
            args->umask = (int)strtol(temp, NULL, 8);
            if (args->umask < 02) {
                log_error("Invalid umask specified (min=02)");
                return NULL;
            }
        }
        else if (!strcmp(argv[x], "-stop")) {
            args->stop = true;
        }
        else if (!strcmp(argv[x], "-check")) {
            args->chck = true;
            args->dtch = false;
        }
        else if (!strcmp(argv[x], "-nodetach")) {
            args->dtch = false;
        }
        else if (!strcmp(argv[x], "-keepstdin")) {
            args->redirectstdin = false;
        }
        else if (!strcmp(argv[x], "-service")) {
            args->service = true;
        }
        else if (!strcmp(argv[x], "-install")) {
            args->install = true;
        }
        else if (!strcmp(argv[x], "-remove")) {
            args->remove = true;
        }
        else if (!strcmp(argv[x], "-pidfile")) {
            args->pidf = optional(argc, argv, x++);
            if (args->pidf == NULL) {
                log_error("Invalid PID file specified");
                return NULL;
            }
        }
        else if (!strcmp(argv[x], "-outfile")) {
            args->outfile = optional(argc, argv, x++);
            if (args->outfile == NULL) {
                log_error("Invalid Output File specified");
                return NULL;
            }
        }
        else if (!strcmp(argv[x], "-errfile")) {
            args->errfile = optional(argc, argv, x++);
            if (args->errfile == NULL) {
                log_error("Invalid Error File specified");
                return NULL;
            }
        }
        else if (!strncmp(argv[x], "-verbose", 8)) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strcmp(argv[x], "-D")) {
            log_error("Parameter -D must be followed by <name>=<value>");
            return NULL;
        }
        else if (!strncmp(argv[x], "-D", 2)) {
            temp = strchr(argv[x], '=');
            if (temp == argv[x] + 2) {
                log_error("A property name must be specified before '='");
                return NULL;
            }
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strncmp(argv[x], "-X", 2)) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strncmp(argv[x], "-ea", 3)) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strncmp(argv[x], "-enableassertions", 17)) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strncmp(argv[x], "-da", 3)) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strncmp(argv[x], "-disableassertions", 18)) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strcmp(argv[x], "-esa")) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strcmp(argv[x], "-enablesystemassertions")) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strcmp(argv[x], "-dsa")) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strcmp(argv[x], "-disablesystemassertions")) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strcmp(argv[x], "-procname")) {
            args->procname = optional(argc, argv, x++);
            if (args->procname == NULL) {
                log_error("Invalid process name specified");
                return NULL;
            }
        }
        else if (!strncmp(argv[x], "-agentlib:", 10)) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strncmp(argv[x], "-agentpath:", 11)) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strncmp(argv[x], "-javaagent:", 11)) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        /* Java 9 specific options */
        else if (!strncmp(argv[x], "--add-modules=", 14)) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strncmp(argv[x], "--module-path=", 14)) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strncmp(argv[x], "--upgrade-module-path=", 22)) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strncmp(argv[x], "--add-reads=", 12)) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strncmp(argv[x], "--add-exports=", 14)) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strncmp(argv[x], "--add-opens=", 12)) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strncmp(argv[x], "--limit-modules=", 16)) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strncmp(argv[x], "--patch-module=", 15)) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (!strncmp(argv[x], "--illegal-access=", 17)) {
            args->opts[args->onum++] = strdup(argv[x]);
        }
        else if (*argv[x] == '-') {
            log_error("Invalid option %s", argv[x]);
            return NULL;
        }
        else {
            args->clas = strdup(argv[x]);
            break;
        }
    }

    if (args->clas == NULL && args->remove == false) {
        log_error("No class specified");
        return NULL;
    }

    x++;
    while (x < argc) {
        args->args[args->anum++] = strdup(argv[x++]);
    }
    return args;
}

static const char *IsYesNo(bool par)
{
    switch (par) {
        case false:
            return "No";
        case true:
            return "Yes";
    }
    return "[Error]";
}

static const char *IsTrueFalse(bool par)
{
    switch (par) {
        case false:
            return "False";
        case true:
            return "True";
    }
    return "[Error]";
}

static const char *IsEnabledDisabled(bool par)
{
    switch (par) {
        case true:
            return "Enabled";
        case false:
            return "Disabled";
    }
    return "[Error]";
}

/* Main entry point: parse command line arguments and dump them */
arg_data *arguments(int argc, char *argv[])
{
    arg_data *args = parse(argc, argv);
    int x = 0;

    if (args == NULL) {
        log_error("Cannot parse command line arguments");
        return NULL;
    }

    if (log_debug_flag == true) {
        log_debug("+-- DUMPING PARSED COMMAND AND ARGUMENTS ---------------");
        log_debug("| Executable:      %s", PRINT_NULL(argv[0]));
        log_debug("| Detach:          %s", IsTrueFalse(args->dtch));
        log_debug("| Show Version:    %s", IsYesNo(args->vers));
        log_debug("| Show Help:       %s", IsYesNo(args->help));
        log_debug("| Check Only:      %s", IsEnabledDisabled(args->chck));
        log_debug("| Stop:            %s", IsTrueFalse(args->stop));
        log_debug("| Wait:            %d", args->wait);
        log_debug("| Restarts:        %d", args->restarts);
        log_debug("| Run as service:  %s", IsYesNo(args->service));
        log_debug("| Install service: %s", IsYesNo(args->install));
        log_debug("| Remove service:  %s", IsYesNo(args->remove));
        log_debug("| JVM Name:        \"%s\"", PRINT_NULL(args->name));
        log_debug("| Java Home:       \"%s\"", PRINT_NULL(args->home));
        log_debug("| PID File:        \"%s\"", PRINT_NULL(args->pidf));
        log_debug("| User Name:       \"%s\"", PRINT_NULL(args->user));
        log_debug("| Extra Options:   %d", args->onum);
        for (x = 0; x < args->onum; x++) {
            log_debug("|   \"%s\"", args->opts[x]);
        }

        log_debug("| Class Invoked:   \"%s\"", PRINT_NULL(args->clas));
        log_debug("| Class Arguments: %d", args->anum);
        for (x = 0; x < args->anum; x++) {
            log_debug("|   \"%s\"", args->args[x]);
        }
        log_debug("+-------------------------------------------------------");
    }
    return args;
}
