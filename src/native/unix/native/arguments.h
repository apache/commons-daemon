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

#ifndef __JSVC_ARGUMENTS_H__
#define __JSVC_ARGUMENTS_H__

#ifdef __cplusplus
extern "C"
{
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
    /** Working directory (defaults to /). */
    char *cwd;
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
    /** Show the VM version and continue. */
    bool vershow;
    /** Wether to display the help page or not. */
    bool help;
    /** Only check environment without running the service. */
    bool chck;
    /** Stop running jsvc */
    bool stop;
    /** number of seconds to until service started */
    int wait;
    /** max restarts **/
    int restarts;
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
    /** Program name **/
    char *procname;
    /** Whether to redirect stdin to /dev/null or not. Defaults to true **/
    bool redirectstdin;
    /** What umask to use **/
    int umask;
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
#endif                          /* ifndef __JSVC_ARGUMENTS_H__ */

