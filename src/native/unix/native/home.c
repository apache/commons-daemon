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

/* @version $Id: home.c,v 1.3 2003/12/31 04:58:31 billbarker Exp $ */
#include "jsvc.h"

/* Check if a path is a directory */
static bool checkdir(char *path) {
    struct stat home;

    if (path==NULL) return(false);
    if (stat(path,&home)!=0) return(false);
    if (S_ISDIR(home.st_mode)) return(true);
    return(false);
}

/* Check if a path is a file */
static bool checkfile(char *path) {
    struct stat home;

    if (path==NULL) return(false);
    if (stat(path,&home)!=0) return(false);
    if (S_ISREG(home.st_mode)) return(true);
    return(false);
}

/* Parse a VM configuration file */
static bool parse(home_data *data) {
    FILE *cfgf=fopen(data->cfgf,"r");
    char *ret=NULL, *sp;
    char buf[1024];

    if (cfgf==NULL) return(false);

    data->jvms=(home_jvm **)malloc(256*sizeof(home_jvm *));

    while((ret=fgets(buf,1024,cfgf))!=NULL) {
        char *tmp=strchr(ret,'#');
        int pos;

        /* Clear the string at the first occurrence of '#' */
        if (tmp!=NULL) tmp[0]='\0';

        /* Trim the string (including leading '-' chars */
        while((ret[0]==' ')||(ret[0]=='\t')||(ret[0]=='-')) ret++;
        pos=strlen(ret);
        while(pos>=0) {
            if ((ret[pos]=='\r')||(ret[pos]=='\n')||(ret[pos]=='\t')||
                (ret[pos]=='\0')||(ret[pos]==' ')) {
                ret[pos--]='\0';
            } else break;
        }
        /* Format changed for 1.4 JVMs */
        sp = strchr(ret, ' ');
        if(sp != NULL)
            *sp = '\0'

        /* Did we find something significant? */
        if (strlen(ret)>0) {
            char *libf=NULL;
            int x=0;

            log_debug("Found VM %s definition in configuration",ret);
            while(location_jvm_configured[x]!=NULL) {
                char *orig=location_jvm_configured[x];
                char temp[1024];
                char repl[1024];
                int k;

                k=replace(temp,1024,orig,"$JAVA_HOME",data->path);
                if (k!=0) {
                    log_error("Can't replace home in VM library (%d)",k);
                    return(false);
                }
                k=replace(repl,1024,temp,"$VM_NAME",ret);
                if (k!=0) {
                    log_error("Can't replace name in VM library (%d)",k);
                    return(false);
                }

                log_debug("Checking library %s",repl);
                if (checkfile(repl)) {
                    libf=strdup(repl);
                    break;
                }
                x++;
            }

            if (libf==NULL) {
                log_debug("Cannot locate library for VM %s (skipping)",ret);
            } else {
                data->jvms[data->jnum]=(home_jvm *)malloc(sizeof(home_jvm));
                data->jvms[data->jnum]->name=strdup(ret);
                data->jvms[data->jnum]->libr=libf;
                data->jnum++;
                data->jvms[data->jnum]=NULL;
            }
        }
    }
    return(true);
}

/* Build a Java Home structure for a path */
static home_data *build(char *path) {
    home_data *data=NULL;
    char *cfgf=NULL;
    char buf[1024];
    int x=0;
    int k=0;

    if (path==NULL) return(NULL);

    log_debug("Attempting to locate Java Home in %s",path);
    if (checkdir(path)==false) {
        log_debug("Path %s is not a directory",path);
        return(NULL);
    }

    while(location_jvm_cfg[x]!=NULL) {
        if ((k=replace(buf,1024,location_jvm_cfg[x],"$JAVA_HOME",path))!=0) {
            log_error("Error replacing values for jvm.cfg (%d)",k);
            return(NULL);
        }
        log_debug("Attempting to locate VM configuration file %s",buf);
        if(checkfile(buf)==true) {
            log_debug("Found VM configuration file at %s",buf);
            cfgf=strdup(buf);
            break;
        }
        x++;
    }

    data=(home_data *)malloc(sizeof(home_data));
    data->path=strdup(path);
    data->cfgf=cfgf;
    data->jvms=NULL;
    data->jnum=0;

    /* We don't have a jvm.cfg configuration file, so all we have to do is
       trying to locate the "default" Java Virtual Machine library */
    if (cfgf==NULL) {
        log_debug("VM configuration file not found");
        x=0;
        while(location_jvm_default[x]!=NULL) {
            char *libr=location_jvm_default[x];

            if ((k=replace(buf,1024,libr,"$JAVA_HOME",path))!=0) {
                log_error("Error replacing values for JVM library (%d)",k);
                return(NULL);
            }
            log_debug("Attempting to locate VM library %s",buf);
            if (checkfile(buf)==true) {
                data->jvms=(home_jvm **)malloc(2*sizeof(home_jvm *));
                data->jvms[0]=(home_jvm *)malloc(sizeof(home_jvm));
                data->jvms[0]->name=NULL;
                data->jvms[0]->libr=strdup(buf);
                data->jvms[1]=NULL;
                data->jnum=1;
                return(data);
            }
            x++;
        }

        return(data);
    }

    /* If we got here, we most definitely found a jvm.cfg file */
    if (parse(data)==false) {
        log_error("Cannot parse VM configuration file %s",data->cfgf);
    }

    return(data);
}

/* Find the Java Home */
static home_data *find(char *path) {
    home_data *data=NULL;
    int x=0;

    if (path==NULL) {
        log_debug("Home not specified on command line, using environment");
        path=getenv("JAVA_HOME");
    }

    if (path==NULL) {
        log_debug("Home not on command line or in environment, searching");
        while (location_home[x]!=NULL) {
            if ((data=build(location_home[x]))!=NULL) {
                log_debug("Java Home located in %s",data->path);
                return(data);
            }
            x++;
        }
    } else {
        if ((data=build(path))!=NULL) {
            log_debug("Java Home located in %s",data->path);
            return(data);
        }
    }

    return(NULL);
}

/* Main entry point: locate home and dump structure */
home_data *home(char *path) {
    home_data *data=find(path);
    int x=0;

    if (data==NULL) {
        log_error("Cannot locate Java Home");
        return(NULL);
    }

    if (log_debug_flag==true) {
        log_debug("+-- DUMPING JAVA HOME STRUCTURE ------------------------");
        log_debug("| Java Home:       \"%s\"",PRINT_NULL(data->path));
        log_debug("| Java VM Config.: \"%s\"",PRINT_NULL(data->cfgf));
        log_debug("| Found JVMs:      %d",data->jnum);
        for (x=0; x<data->jnum; x++) {
            home_jvm *jvm=data->jvms[x];
            log_debug("| JVM Name:        \"%s\"",jvm->name);
            log_debug("|                  \"%s\"",jvm->libr);
        }
        log_debug("+-------------------------------------------------------");
    }

    return(data);
}
