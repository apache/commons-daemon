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

/* Locations of various JVM files. We have to deal with all this madness since
 * we're not distributed together (yet!) with an official VM distribution. All
 * this CRAP needs improvement, and based on the observation of default
 * distributions of VMs and OSes. If it doesn't work for you, please report
 * your VM layout (ls -laR) and system details (build/config.guess) so that we
 * can improve the search algorithms.
 */

/* If JAVA_HOME is not defined we search this list of paths (OS-dependent)
 * to find the default location of the JVM.
 */
char *location_home[] = {
#if defined(OS_DARWIN)
    "/System/Library/Frameworks/JavaVM.framework/Home",
    "/System/Library/Frameworks/JavaVM.framework/Versions/CurrentJDK/Home/",
#elif defined(OS_LINUX) || defined(OS_SOLARIS) || defined(OS_BSD) || defined(OS_AIX)
    "/usr/java/default",
    "/usr/java",
    "/usr/local/java",
    "/usr/lib/jvm/default-java",
    "/usr/lib/jvm/java",
    "/etc/alternatives/java_sdk",
    "/etc/alternatives/java_sdk_openjdk",
    "/etc/alternatives/jre",
#if defined(__LP64__)
    "/lib64/jvm/java",
    "/lib64/jvm/java-openjdk",
#endif
    "/lib/jvm/java",
    "/lib/jvm/java-openjdk",
#elif defined(OS_CYGWIN)
    "/cygdrive/c/WINNT/system32/java",
#elif defined(OS_SYSV)
    "/opt/java",
    "/opt/java/jdk13",
#elif defined(OS_TRU64)
    "/usr/opt/java142",
    "/usr/opt/java13",
#elif defined(OS_HPUX)
    "/opt/java6",
    "/opt/java1.5",
    "/opt/java1.4",
    "/opt/java1.3",
#endif
    NULL,
};

/* The jvm.cfg file defines the VMs available for invocation. So far, on all
 * all systems I've seen it's in $JAVA_HOME/lib. If this file is not found,
 * then the "default" VMs (from location_jvm_default) is searched, otherwise,
 * we're going to look thru the "configured" VMs (from lod_cfgvm) lying
 * somewhere around JAVA_HOME. (Only two, I'm happy)
 */
char *location_jvm_cfg[] = {
    "$JAVA_HOME/jre/lib/jvm.cfg",          /* JDK */
    "$JAVA_HOME/lib/jvm.cfg",              /* JRE */
    "$JAVA_HOME/jre/lib/" CPU "/jvm.cfg",  /* JDK */
    "$JAVA_HOME/lib/" CPU "/jvm.cfg",      /* JRE */
#if defined(OS_AIX)
    "$JAVA_HOME/jre/lib/ppc/jvm.cfg",      /* JDK */
    "$JAVA_HOME/lib/ppc/jvm.cfg",          /* JRE */
#endif
    NULL,
};

/* This is the list of "defaults" VM (searched when jvm.cfg is not found, as
 * in the case of most JDKs 1.2.2
 */
char *location_jvm_default[] = {
#if defined(OS_DARWIN)
    "$JAVA_HOME/../Libraries/libjvm.dylib",
    "$JAVA_HOME/jre/lib/server/libjvm.dylib",
    "$JAVA_HOME/jre/lib/client/libjvm.dylib",
#elif defined(OS_CYGWIN)
    "$JAVA_HOME/jre/bin/classic/jvm.dll",           /* Sun JDK 1.3 */
    "$JAVA_HOME/jre/bin/client/jvm.dll",            /* Sun JDK 1.4 */
#elif defined(OS_LINUX) || defined(OS_SOLARIS) || defined(OS_BSD) || defined(OS_SYSV) || defined(OS_FREEBSD) || defined(OS_TRU64) || defined(OS_AIX)
    "$JAVA_HOME/jre/lib/" CPU "/classic/libjvm.so", /* Sun JDK 1.2 */
    "$JAVA_HOME/jre/lib/" CPU "/server/libjvm.so",  /* Sun JDK 1.4 */
    "$JAVA_HOME/jre/lib/" CPU "/client/libjvm.so",  /* Sun JDK 1.3 */
    "$JAVA_HOME/jre/lib/" CPU "/libjvm.so",         /* Sun JDK */
    "$JAVA_HOME/lib/" CPU "/classic/libjvm.so",     /* Sun JRE 1.2 */
    "$JAVA_HOME/lib/" CPU "/server/libjvm.so",      /* Sun JRE 1.4 */
    "$JAVA_HOME/lib/" CPU "/client/libjvm.so",      /* Sun JRE 1.3 */
    "$JAVA_HOME/lib/" CPU "/libjvm.so",             /* Sun JRE */
    "$JAVA_HOME/jre/bin/" CPU "/classic/libjvm.so", /* IBM JDK 1.3 */
    "$JAVA_HOME/jre/bin/" CPU "/libjvm.so",         /* IBM JDK */
    "$JAVA_HOME/bin/" CPU "/classic/libjvm.so",     /* IBM JRE 1.3 */
    "$JAVA_HOME/bin/" CPU "/libjvm.so",             /* IBM JRE */
    /* Those are "weirdos: if we got here, we're probably in troubles and
     *  we're not going to find anything, but hope never dies...
     */
    "$JAVA_HOME/jre/lib/" CPU "/classic/green_threads/libjvm.so",
#if defined(OSD_POSIX)
    "$JAVA_HOME/lib/s390/client/green_threads/libjvm.so",
    "$JAVA_HOME/lib/sparc/client/green_threads/libjvm.so",
#endif
    "$JAVA_HOME/jre/lib/classic/libjvm.so",
    "$JAVA_HOME/jre/lib/client/libjvm.so",
    "$JAVA_HOME/jre/lib/libjvm.so",
    "$JAVA_HOME/lib/classic/libjvm.so",
    "$JAVA_HOME/lib/client/libjvm.so",
    "$JAVA_HOME/lib/libjvm.so",
    "$JAVA_HOME/jre/bin/classic/libjvm.so",
    "$JAVA_HOME/jre/bin/client/libjvm.so",
    "$JAVA_HOME/jre/bin/libjvm.so",
    "$JAVA_HOME/bin/classic/libjvm.so",
    "$JAVA_HOME/bin/client/libjvm.so",
    "$JAVA_HOME/bin/libjvm.so",
    "$JAVA_HOME/jre/lib/" CPU "/fast64/libjvm.so",
    "$JAVA_HOME/jre/lib/" CPU "/fast32/libjvm.so",
    "$JAVA_HOME/lib/" CPU "/fast64/libjvm.so",
    "$JAVA_HOME/lib/" CPU "/fast32/libjvm.so",
#elif defined(OS_HPUX)
    "$JAVA_HOME/jre/lib/" CPU "/server/libjvm." SO_EXT,
    "$JAVA_HOME/jre/lib/" CPU "/client/libjvm." SO_EXT,
    "$JAVA_HOME/jre/lib/" CPU "/hotspot/libjvm." SO_EXT,
    "$JAVA_HOME/jre/lib/" CPU "/classic/libjvm." SO_EXT,
#endif
    "/usr/lib/libgcj.so.7",            /* gcc java libraries */
    "/usr/lib/libgcj.so.6",
    NULL,
};

/* This is the list of "configured" VM (searched when jvm.cfg is found, as
 * in the case of most JDKs 1.3 (not IBM, for example), way easier than
 * before, and lovely, indeed...
 */
char *location_jvm_configured[] = {
#if defined(OS_DARWIN)
    "$JAVA_HOME/lib/$VM_NAME/libjvm.dylib",             /* Java 9 */
    "$JAVA_HOME/jre/lib/$VM_NAME/libjvm.dylib",
    "$JAVA_HOME/../Libraries/lib$VM_NAME.dylib",
#elif defined(OS_CYGWIN)
    "$JAVA_HOME/jre/bin/$VM_NAME/jvm.dll",              /* Sun JDK 1.3 */
#elif defined(OS_LINUX) || defined(OS_SOLARIS) || defined(OS_BSD) || defined(OS_FREEBSD) || defined(OS_TRU64) || defined(OS_AIX)
    "$JAVA_HOME/lib/$VM_NAME/libjvm.so",                /* Java 9 */
    "$JAVA_HOME/jre/lib/" CPU "/$VM_NAME/libjvm.so",    /* Sun JDK 1.3 */
    "$JAVA_HOME/lib/" CPU "/$VM_NAME/libjvm.so",        /* Sun JRE 1.3 */
#if defined(OS_AIX)
    "$JAVA_HOME/jre/lib/ppc/$VM_NAME/libjvm.so",        /* Sun JDK 1.3 */
    "$JAVA_HOME/lib/ppc/$VM_NAME/libjvm.so",            /* Sun JRE 1.3 */
#endif
#elif defined(OS_HPUX)
    "$JAVA_HOME/jre/lib/" CPU "/$VM_NAME/libjvm." SO_EXT,
    "$JAVA_HOME/lib/" CPU "/$VM_NAME/libjvm." SO_EXT,
#elif defined(OS_SYSV)
    "$JAVA_HOME/jre/lib/" CPU "/$VM_NAME/dce_threads/libjvm.so",
    "$JAVA_HOME/jre/lib/" CPU "/$VM_NAME/green_threads/libjvm.so",
    "$JAVA_HOME/lib/" CPU "/$VM_NAME/dce_threads/libjvm.so",
    "$JAVA_HOME/lib/" CPU "/$VM_NAME/green_threads/libjvm.so",
#endif
    NULL,
};
