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

void help(home_data *data)
{
    int x;

    printf("Usage: %s [-options] class [args...]\n", log_prog);
    printf("\n");
    printf("Where options include:\n");
    printf("\n");
    printf("    -help | --help | -?\n");
    printf("        show this help page (implies -nodetach)\n");
    printf("    -jvm <JVM name>\n");
    printf("        use a specific Java Virtual Machine. Available JVMs:\n");
    printf("           ");
    for (x = 0; x < data->jnum; x++) {
        printf(" '%s'", PRINT_NULL(data->jvms[x]->name));
    }
    printf("\n");
    printf("    -client\n");
    printf("        use a client Java Virtual Machine.\n");
    printf("    -server\n");
    printf("        use a server Java Virtual Machine.\n");
    printf("    -cp | -classpath <directories and zip/jar files>\n");
    printf("        set search path for service classes and resouces\n");
    printf("    -java-home | -home <directory>\n");
    printf("        set the path of your JDK or JRE installation (or set\n");
    printf("        the JAVA_HOME environment variable)\n");

    printf("    -version\n");
    printf("        show the current Java environment version (to check\n");
    printf("        correctness of -home and -jvm. Implies -nodetach)\n");
    printf("    -showversion\n");
    printf("        show the current Java environment version (to check\n");
    printf("        correctness of -home and -jvm) and continue execution.\n");
    printf("    -nodetach\n");
    printf("        don't detach from parent process and become a daemon\n");
    printf("    -debug\n");
    printf("        verbosely print debugging information\n");
    printf("    -check\n");
    printf("        only check service (implies -nodetach)\n");
    printf("    -user <user>\n");
    printf("        user used to run the daemon (defaults to current user)\n");
    printf("    -verbose[:class|gc|jni]\n");
    printf("        enable verbose output\n");
    printf("    -cwd </full/path>\n");
    printf("        set working directory to given location (defaults to /)\n");
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
    printf("    -ea[:<packagename>...|:<classname>]\n");
    printf("    -enableassertions[:<packagename>...|:<classname>]\n");
    printf("        enable assertions\n");
    printf("    -da[:<packagename>...|:<classname>]\n");
    printf("    -disableassertions[:<packagename>...|:<classname>]\n");
    printf("        disable assertions\n");
    printf("    -esa | -enablesystemassertions\n");
    printf("        enable system assertions\n");
    printf("    -dsa | -disablesystemassertions\n");
    printf("        disable system assertions\n");
    printf("    -agentlib:<libname>[=<options>]\n");
    printf("        load native agent library <libname>, e.g. -agentlib:hprof\n");
    printf("    -agentpath:<pathname>[=<options>]\n");
    printf("        load native agent library by full pathname\n");
    printf("    -javaagent:<jarpath>[=<options>]\n");
    printf("        load Java programming language agent, see java.lang.instrument\n");
    printf("    -procname <procname>\n");
    printf("        use the specified process name\n");
    printf("    -wait <waittime>\n");
    printf("        wait waittime seconds for the service to start\n");
    printf("        waittime should multiple of 10 (min=10)\n");
    printf("    -restarts <maxrestarts>\n");
    printf("        maximum automatic restarts (integer)\n");
    printf("        -1=infinite (default), 0=none, 1..(INT_MAX-1)=fixed restart count\n");
    printf("    -stop\n");
    printf("        stop the service using the file given in the -pidfile option\n");
    printf("    -keepstdin\n");
    printf("        does not redirect stdin to /dev/null\n");
    printf("    --add-modules=<module name>\n");
    printf("        Java 9 --add-modules option. Passed as it is to JVM\n");
    printf("    --module-path=<module path>\n");
    printf("        Java 9 --module-path option. Passed as it is to JVM\n");
    printf("    --upgrade-module-path=<module path>\n");
    printf("        Java 9 --upgrade-module-path option. Passed as it is to JVM\n");
    printf("    --add-reads=<module name>\n");
    printf("        Java 9 --add-reads option. Passed as it is to JVM\n");
    printf("    --add-exports=<module name>\n");
    printf("        Java 9 --add-exports option. Passed as it is to JVM\n");
    printf("    --add-opens=<module name>\n");
    printf("        Java 9 --add-opens option. Passed as it is to JVM\n");
    printf("    --limit-modules=<module name>\n");
    printf("        Java 9 --limit-modules option. Passed as it is to JVM\n");
    printf("    --patch-module=<module name>\n");
    printf("        Java 9 --patch-module option. Passed as it is to JVM\n");
    printf("    --illegal-access=<value>\n");
    printf("        Java 9 --illegal-access option. Passed as it is to JVM.\n");
    printf("        Refer java help for possible values.\n");
    printf("\njsvc (Apache Commons Daemon) " JSVC_VERSION_STRING "\n");
    printf("Copyright (c) 1999-2019 Apache Software Foundation.\n");

    printf("\n");
}
