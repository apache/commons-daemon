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

/* @version $Id: arguments.h,v 1.1 2003/09/04 23:28:20 yoavs Exp $ */
#ifndef __JSVC_ARGUMENTS_H__
#define __JSVC_ARGUMENTS_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The structure holding all parsed command line options.
 */
typedef struct {
    /** The name of the PID file. */
    char *pidf;
    /** The name of the user. */
    char *user;
    /** The name of the JVM to use. */
    char *name;
    /** The JDK or JRE installation path (JAVA_HOME). */
    char *home;
    /** Options used to invoke the JVM. */
    char **opts;
    /** Number of JVM options. */
    int onum;
    /** The name of the class to invoke. */
    char *clas;
    /** Command line arguments to the class. */
    char **args;
    /** Number of class command line arguments. */
    int anum;
    /** Wether to detach from parent process or not. */
    bool dtch;
    /** Wether to print the VM version number or not. */
    bool vers;
    /** Wether to display the help page or not. */
    bool help;
    /** Only check environment without running the service. */
    bool chck;
    /** Install as a service (win32) */
    bool install;
    /** Remove when installed as a service (win32) */
    bool remove;
    /** Run as a service (win32) */
    bool service;
    /** Destination for stdout */
    char *outfile;
    /** Destination for stderr */
    char *errfile;
} arg_data;

/**
 * Parse command line arguments.
 *
 * @param argc The number of command line arguments.
 * @param argv Pointers to the different arguments.
 * @return A pointer to a arg_data structure containing the parsed command
 *         line arguments, or NULL if an error was detected.
 */
arg_data *arguments(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif
#endif /* ifndef __JSVC_ARGUMENTS_H__ */
