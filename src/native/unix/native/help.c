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

/* @version $Id: help.c,v 1.2 2003/09/16 11:50:16 jfclere Exp $ */
#include "jsvc.h"

void help(home_data *data) {
    int x;

    printf("Usage: %s [-options] class [args...]\n",log_prog);
    printf("\n");
    printf("Where options include:\n");
    printf("\n");

    printf("    -jvm <JVM name>\n");
    printf("        use a specific Java Virtual Machine. Available JVMs:\n");
    printf("           ");
    for (x=0; x<data->jnum; x++) printf(" '%s'",data->jvms[x]->name);
    printf("\n");

    printf("    -cp / -classpath <directories and zip/jar files>\n");
    printf("        set search path for service classes and resouces\n");

    printf("    -home <directory>\n");
    printf("        set the path of your JDK or JRE installation (or set\n");
    printf("        the JAVA_HOME environment variable)\n");

    printf("    -version\n");
    printf("        show the current Java environment version (to check\n");
    printf("        correctness of -home and -jvm. Implies -nodetach)\n");

    printf("    -help / -?\n");
    printf("        show this help page (implies -nodetach)\n");

    printf("    -nodetach\n");
    printf("        don't detach from parent process and become a daemon\n");

    printf("    -debug\n");
    printf("        verbosely print debugging information\n");

    printf("    -check\n");
    printf("        only check service (implies -nodetach)\n");
 
    printf("    -user\n");
    printf("        user used to run the daemon (defaults to current user)\n");
 
    printf("    -verbose[:class|gc|jni]\n");
    printf("        enable verbose output\n");
 
    printf("    -outfile </full/path/to/file>\n");
    printf("        Location for output from stdout (defaults to /dev/null)\n");
    printf("        Use the value '&2' to simulate '1>&2'\n");

    printf("    -errfile </full/path/to/file>\n");
    printf("        Location for output from stderr (defaults to /dev/null)\n");
    printf("        Use the value '&1' to simulate '2>&1'\n");

    printf("    -pidfile </full/path/to/file>\n");
    printf("        Location for output from the file containing the pid of jsvc\n");
    printf("        (defaults to /var/run/jsvc.pid)\n");

    printf("    -D<name>=<value>\n");
    printf("        set a Java system property\n");

    printf("    -X<option>\n");
    printf("        set Virtual Machine specific option\n");

    printf("\n");
}
