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

/* @version $Id: location.c,v 1.1 2003/09/04 23:28:20 yoavs Exp $ */
#include "jsvc.h"

/* Locations of various JVM files. We have to deal with all this madness since
   we're not distributed togheter (yet!) with an official VM distribution. All
   this CRAP needs improvement, and based on the observation of default
   distributions of VMs and OSes. If it doesn't work for you, please report
   your VM layout (ls -laR) and system details (build/config.guess) so that we
   can improve the search algorithms. */

/* If JAVA_HOME is not defined we search this list of paths (OS-dependant)
   to find the default location of the JVM. */
char *location_home[] = {
#if defined(OS_DARWIN)
    "/System/Library/Frameworks/JavaVM.framework/Home",
    "/System/Library/Frameworks/JavaVM.framework/Versions/CurrentJDK/Home/",
#elif defined(OS_LINUX) || defined(OS_SOLARIS) || defined(OS_BSD)
    "/usr/java",
    "/usr/local/java",
#elif defined(OS_CYGWIN)
    "/cygdrive/c/WINNT/system32/java",
#elif defined(OS_SYSV)
    "/opt/java",
    "/opt/java/jdk13",
#endif
    NULL,
};

/* The jvm.cfg file defines the VMs available for invocation. So far, on all
   all systems I've seen it's in $JAVA_HOME/lib. If this file is not found,
   then the "default" VMs (from location_jvm_default) is searched, otherwise,
   we're going to look thru the "configured" VMs (from lod_cfgvm) lying
   somewhere around JAVA_HOME. (Only two, I'm happy) */
char *location_jvm_cfg[] = {
    "$JAVA_HOME/jre/lib/jvm.cfg", /* JDK */
    "$JAVA_HOME/lib/jvm.cfg",     /* JRE */
    NULL,
};

/* This is the list of "defaults" VM (searched when jvm.cfg is not found, as
   in the case of most JDKs 1.2.2 */
char *location_jvm_default[] = {
#if defined(OS_DARWIN)
    "$JAVA_HOME/../Libraries/libjvm.dylib",
#elif defined(OS_CYGWIN)
    "$JAVA_HOME/jre/bin/classic/jvm.dll",           /* Sun JDK 1.3 */
    "$JAVA_HOME/jre/bin/client/jvm.dll",            /* Sun JDK 1.4 */
#elif defined(OS_LINUX) || defined(OS_SOLARIS) || defined(OS_BSD) || defined(OS_SYSV)
    "$JAVA_HOME/jre/lib/" CPU "/classic/libjvm.so", /* Sun JDK 1.2 */
    "$JAVA_HOME/jre/lib/" CPU "/client/libjvm.so",  /* Sun JDK 1.3 */
    "$JAVA_HOME/jre/lib/" CPU "/libjvm.so",         /* Sun JDK */
    "$JAVA_HOME/lib/" CPU "/classic/libjvm.so",     /* Sun JRE 1.2 */
    "$JAVA_HOME/lib/" CPU "/client/libjvm.so",      /* Sun JRE 1.3 */
    "$JAVA_HOME/lib/" CPU "/libjvm.so",             /* Sun JRE */
    "$JAVA_HOME/jre/bin/" CPU "/classic/libjvm.so", /* IBM JDK 1.3 */
    "$JAVA_HOME/jre/bin/" CPU "/libjvm.so",         /* IBM JDK */
    "$JAVA_HOME/bin/" CPU "/classic/libjvm.so",     /* IBM JRE 1.3 */
    "$JAVA_HOME/bin/" CPU "/libjvm.so",             /* IBM JRE */
    /* Those are "weirdos: if we got here, we're probably in troubles and
       we're not going to find anything, but hope never dies... */
    "$JAVA_HOME/jre/lib/" CPU "/classic/green_threads/libjvm.so",
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
#endif
    NULL,
};

/* This is the list of "configured" VM (searched when jvm.cfg is found, as
   in the case of most JDKs 1.3 (not IBM, for example), way easier than
   before, and lovely, indeed... */
char *location_jvm_configured[] = {
#if defined(OS_DARWIN)
    "$JAVA_HOME/../Libraries/lib$VM_NAME.dylib",
#elif defined(OS_CYGWIN)
    "$JAVA_HOME/jre/bin/$VM_NAME/jvm.dll",          /* Sun JDK 1.3 */
#elif defined(OS_LINUX) || defined(OS_SOLARIS) || defined(OS_BSD)
    "$JAVA_HOME/jre/lib/" CPU "/$VM_NAME/libjvm.so",/* Sun JDK 1.3 */
    "$JAVA_HOME/lib/" CPU "/$VM_NAME/libjvm.so",    /* Sun JRE 1.3 */
#elif defined(OS_SYSV)
    "$JAVA_HOME/jre/lib/" CPU "/$VM_NAME/dce_threads/libjvm.so",
    "$JAVA_HOME/jre/lib/" CPU "/$VM_NAME/green_threads/libjvm.so",
    "$JAVA_HOME/lib/" CPU "/$VM_NAME/dce_threads/libjvm.so",
    "$JAVA_HOME/lib/" CPU "/$VM_NAME/green_threads/libjvm.so",
#endif
    NULL,
};
