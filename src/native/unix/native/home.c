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
#include "jsvc.h"

/* Check if a path is a directory */
static bool checkdir(char *path)
{
    struct stat home;

    if (path == NULL)
        return (false);
    if (stat(path, &home) != 0)
        return (false);
    if (S_ISDIR(home.st_mode))
        return (true);
    return (false);
}

/* Check if a path is a file */
static bool checkfile(char *path)
{
    struct stat home;

    if (path == NULL)
        return (false);
    if (stat(path, &home) != 0)
        return (false);
    if (S_ISREG(home.st_mode))
        return (true);
    return (false);
}

/* Parse a VM configuration file */
static bool parse(home_data *data)
{
    FILE *cfgf = fopen(data->cfgf, "r");
    char *ret = NULL, *sp;
    char buf[1024];

    if (cfgf == NULL) {
        log_debug("Can't open %s\n", data->cfgf);
        return (false);
    }

    data->jvms = (home_jvm **)malloc(256 * sizeof(home_jvm *));

    while ((ret = fgets(buf, 1024, cfgf)) != NULL) {
        char *tmp = strchr(ret, '#');
        int pos;

        /* Clear the string at the first occurrence of '#' */
        if (tmp != NULL)
            tmp[0] = '\0';

        /* Trim the string (including leading '-' chars */
        while ((ret[0] == ' ') || (ret[0] == '\t') || (ret[0] == '-'))
            ret++;
        pos = strlen(ret);
        while (pos >= 0) {
            if ((ret[pos] == '\r') || (ret[pos] == '\n') || (ret[pos] == '\t')
                || (ret[pos] == '\0') || (ret[pos] == ' ')) {
                ret[pos--] = '\0';
            }
            else
                break;
        }
        /* Format changed for 1.4 JVMs */
        sp = strchr(ret, ' ');
        if (sp != NULL)
            *sp = '\0';

        /* Did we find something significant? */
        if (strlen(ret) > 0) {
            char *libf = NULL;
            int x = 0;

            log_debug("Found VM %s definition in configuration", ret);
            while (location_jvm_configured[x] != NULL) {
                char *orig = location_jvm_configured[x];
                char temp[1024];
                char repl[1024];
                int k;

                k = replace(temp, 1024, orig, "$JAVA_HOME", data->path);
                if (k != 0) {
                    log_error("Can't replace home in VM library (%d)", k);
                    return (false);
                }
                k = replace(repl, 1024, temp, "$VM_NAME", ret);
                if (k != 0) {
                    log_error("Can't replace name in VM library (%d)", k);
                    return (false);
                }

                log_debug("Checking library %s", repl);
                if (checkfile(repl)) {
                    libf = strdup(repl);
                    break;
                }
                x++;
            }

            if (libf == NULL) {
                log_debug("Cannot locate library for VM %s (skipping)", ret);
            }
            else {
                data->jvms[data->jnum] = (home_jvm *)malloc(sizeof(home_jvm));
                data->jvms[data->jnum]->name = strdup(ret);
                data->jvms[data->jnum]->libr = libf;
                data->jnum++;
                data->jvms[data->jnum] = NULL;
            }
        }
    }
    fclose(cfgf);
    return (true);
}

/* Build a Java Home structure for a path */
static home_data *build(char *path)
{
    home_data *data = NULL;
    char *cfgf = NULL;
    char buf[1024];
    int x = 0;
    int k = 0;

    if (path == NULL)
        return (NULL);

    log_debug("Attempting to locate Java Home in %s", path);
    if (checkdir(path) == false) {
        log_debug("Path %s is not a directory", path);
        return (NULL);
    }

    while (location_jvm_cfg[x] != NULL) {
        if ((k = replace(buf, 1024, location_jvm_cfg[x], "$JAVA_HOME", path)) != 0) {
            log_error("Error replacing values for jvm.cfg (%d)", k);
            return (NULL);
        }
        log_debug("Attempting to locate VM configuration file %s", buf);
        if (checkfile(buf) == true) {
            log_debug("Found VM configuration file at %s", buf);
            cfgf = strdup(buf);
            break;
        }
        x++;
    }

    data = (home_data *)malloc(sizeof(home_data));
    data->path = strdup(path);
    data->cfgf = cfgf;
    data->jvms = NULL;
    data->jnum = 0;

    /* We don't have a jvm.cfg configuration file, so all we have to do is
       trying to locate the "default" Java Virtual Machine library */
    if (cfgf == NULL) {
        log_debug("VM configuration file not found");
        x = 0;
        while (location_jvm_default[x] != NULL) {
            char *libr = location_jvm_default[x];

            if ((k = replace(buf, 1024, libr, "$JAVA_HOME", path)) != 0) {
                log_error("Error replacing values for JVM library (%d)", k);
                return (NULL);
            }
            log_debug("Attempting to locate VM library %s", buf);
            if (checkfile(buf) == true) {
                data->jvms = (home_jvm **)malloc(2 * sizeof(home_jvm *));
                data->jvms[0] = (home_jvm *)malloc(sizeof(home_jvm));
                data->jvms[0]->name = NULL;
                data->jvms[0]->libr = strdup(buf);
                data->jvms[1] = NULL;
                data->jnum = 1;
                return (data);
            }
            x++;
        }

        return (data);
    }

    /* If we got here, we most definitely found a jvm.cfg file */
    if (parse(data) == false) {
        log_error("Cannot parse VM configuration file %s", data->cfgf);
    }

    return (data);
}

/* Find the Java Home */
static home_data *find(char *path)
{
    home_data *data = NULL;
    int x = 0;

    if (path == NULL || *path == '\0' || strcmp(path, "/") == 0) {
        log_debug("Home not specified on command line, using environment");
        path = getenv("JAVA_HOME");
        if (path == NULL || *path == '\0' || strcmp(path, "/") == 0) {
            /* guard against empty JAVA_HOME */
            path = NULL;
        }
    }

    if (path == NULL) {
        log_debug("Home not on command line or in environment, searching");
        while (location_home[x] != NULL) {
            if ((data = build(location_home[x])) != NULL) {
                log_debug("Java Home located in %s", data->path);
                return (data);
            }
            x++;
        }
    }
    else {
        if ((data = build(path)) != NULL) {
            log_debug("Java Home located in %s", data->path);
            return (data);
        }
    }

    return (NULL);
}

/* Main entry point: locate home and dump structure */
home_data *home(char *path)
{
    home_data *data = find(path);
    int x = 0;

    if (data == NULL) {
        log_error("Cannot locate Java Home");
        return (NULL);
    }

    if (log_debug_flag == true) {
        log_debug("+-- DUMPING JAVA HOME STRUCTURE ------------------------");
        log_debug("| Java Home:       \"%s\"", PRINT_NULL(data->path));
        log_debug("| Java VM Config.: \"%s\"", PRINT_NULL(data->cfgf));
        log_debug("| Found JVMs:      %d", data->jnum);
        for (x = 0; x < data->jnum; x++) {
            home_jvm *jvm = data->jvms[x];
            log_debug("| JVM Name:        \"%s\"", PRINT_NULL(jvm->name));
            log_debug("|                  \"%s\"", PRINT_NULL(jvm->libr));
        }
        log_debug("+-------------------------------------------------------");
    }

    return (data);
}
