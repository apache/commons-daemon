dnl
dnl Licensed to the Apache Software Foundation (ASF) under one or more
dnl contributor license agreements.  See the NOTICE file distributed with
dnl this work for additional information regarding copyright ownership.
dnl The ASF licenses this file to You under the Apache License, Version 2.0
dnl (the "License"); you may not use this file except in compliance with
dnl the License.  You may obtain a copy of the License at
dnl
dnl     http://www.apache.org/licenses/LICENSE-2.0
dnl
dnl Unless required by applicable law or agreed to in writing, software
dnl distributed under the License is distributed on an "AS IS" BASIS,
dnl WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
dnl See the License for the specific language governing permissions and
dnl limitations under the License.
dnl

dnl -------------------------------------------------------------------------
dnl Author  Pier Fumagalli <mailto:pier.fumagalli@eng.sun.com>
dnl Version $Id$
dnl -------------------------------------------------------------------------

AC_DEFUN(AP_SUPPORTED_HOST,[
  AC_MSG_CHECKING([C flags dependant on host system type])

  case $host_os in
  darwin*)
    CFLAGS="$CFLAGS -DOS_DARWIN -DDSO_DLFCN"
    supported_os="darwin"
    ;;
  solaris*)
    CFLAGS="$CFLAGS -DOS_SOLARIS -DDSO_DLFCN"
    supported_os="solaris"
    LIBS="$LIBS -ldl -lthread"
    ;;
  linux*)
    CFLAGS="$CFLAGS -DOS_LINUX -DDSO_DLFCN"
    supported_os="linux"
    LIBS="$LIBS -ldl -lpthread"
    ;;
  cygwin)
    CFLAGS="$CFLAGS -DOS_CYGWIN -DDSO_DLFCN -DNO_SETSID"
    supported_os="win32"
    ;;
  sysv)
    CFLAGS="$CFLAGS -DOS_SYSV -DDSO_DLFCN"
    LIBS="$LIBS -ldl"
    ;;
  sysv4)
    CFLAGS="$CFLAGS -DOS_SYSV -DDSO_DLFCN -Kthread"
    LDFLAGS="-Kthread $LDFLAGS"
    LIBS="$LIBS -ldl"
    ;;
  freebsd*)
    CFLAGS="$CFLAGS -DOS_FREEBSD -DDSO_DLFCN -D_THREAD_SAFE -pthread"
    LDFLAGS="-pthread $LDFLAGS"
    supported_os="freebsd"
    ;;
  osf5*)
    CFLAGS="$CFLAGS -pthread -DOS_TRU64 -DDSO_DLFCN -D_XOPEN_SOURCE_EXTENDED"
    LDFLAGS="$LDFLAGS -pthread"
    ;;
  hpux*)
    CFLAGS="$CFLAGS -pthread -DOS_HPUX -DDSO_DLFCN"
    LDFLAGS="$LDFLAGS -pthread"
    LIBS="$LIBS -lpthread"
    supported_os="hp-ux"
    ;;
  aix5*)
    CFLAGS="$CFLAGS -DOS_AIX -DDSO_DLFCN"
    LDFLAGS="$LDFLAGS -ldl"
    ;;
  kfreebsd*-gnu)
    CFLAGS="$CFLAGS -DOS_BSD -DDSO_DLFCN -pthread"
    supported_os="kfreebsd-gnu"
    LIBS="$LIBS -ldl -lpthread"
    ;;
  gnu*)
    CFLAGS="$CFLAGS -DOS_HURD -DDSO_DLFCN -pthread "
    supported_os="hurd-gnu"
    LIBS="$LIBS -ldl -lpthread"
    ;;
  *)
    AC_MSG_RESULT([failed])
    AC_MSG_ERROR([Unsupported operating system "$host_os"])
    ;;
  esac
  case $host_cpu in
  powerpc*)
    CFLAGS="$CFLAGS -DCPU=\\\"$host_cpu\\\""
    HOST_CPU=$host_cpu;;
  sparc*)
    CFLAGS="$CFLAGS -DCPU=\\\"$host_cpu\\\""
    HOST_CPU=$host_cpu;;
  i?86|x86)
    CFLAGS="$CFLAGS -DCPU=\\\"i386\\\""
    HOST_CPU=i386;;
  x86_64 | amd64)
    CFLAGS="$CFLAGS -DCPU=\\\"amd64\\\""
    HOST_CPU=amd64;;
  bs2000)
    CFLAGS="$CFLAGS -DCPU=\\\"osd\\\" -DCHARSET_EBCDIC -DOSD_POSIX"
    supported_os="osd"
    LDFLAGS="-Kno_link_stdlibs -B llm4"
    LIBS="$LIBS -lBLSLIB"
    LDCMD="/opt/C/bin/cc"
    HOST_CPU=osd;;
  mips | mipsn32 | mips64)
    CFLAGS="$CFLAGS -DCPU=\\\"mips\\\""
    supported_os="mips"
    HOST_CPU=mips;;
  alpha*)
    CFLAGS="$CFLAGS -DCPU=\\\"alpha\\\""
    supported_os="alpha"
    HOST_CPU=alpha;;
  hppa2.0w|hppa64)
    CFLAGS="$CFLAGS -DCPU=\\\"PA_RISC2.0W\\\" -DSO_EXT=\\\"sl\\\""
    HOST_CPU=PA_RISC2.0W;;
  hppa2.0n)
    CFLAGS="$CFLAGS -DCPU=\\\"PA_RISC2.0N\\\" -DSO_EXT=\\\"sl\\\""
    HOST_CPU=PA_RISC2.0N;;
  hppa2.0)
    CFLAGS="$CFLAGS -DCPU=\\\"PA_RISC2.0\\\" -DSO_EXT=\\\"sl\\\""
    HOST_CPU=PA_RISC2.0;;
  mipsel | mipsn32el | mips64el)
    CFLAGS="$CFLAGS -DCPU=\\\"mipsel\\\""
    supported_os="mipsel"
    HOST_CPU=mipsel;;
  ia64w)
    if test "$supported_os" = "hp-ux"
    then
        CFLAGS="$CFLAGS -mlp64 -DCPU=\\\"IA64W\\\" -DSO_EXT=\\\"so\\\""
        LDFLAGS="$LDFLAGS -mlp64"
    else
        CFLAGS="$CFLAGS -DCPU=\\\"ia64\\\""
    fi
    HOST_CPU=ia64;;
  ia64|ia64n)
    if test "$supported_os" = "hp-ux"
    then
        CFLAGS="$CFLAGS -milp32 -DCPU=\\\"IA64N\\\" -DSO_EXT=\\\"so\\\""
        LDFLAGS="$LDFLAGS -milp32"
        HOST_CPU=IA64N
    else
        CFLAGS="$CFLAGS -DCPU=\\\"ia64\\\""
        HOST_CPU=ia64
    fi
    ;;
  sh*)
    CFLAGS="$CFLAGS -DCPU=\\\"$host_cpu\\\""
    HOST_CPU=$host_cpu;;
  s390 | s390x)
    CFLAGS="$CFLAGS -DCPU=\\\"s390\\\""
    supported_os="s390"
    HOST_CPU=s390;;
  arm*)
    CFLAGS="$CFLAGS -DCPU=\\\"arm\\\""
    supported_os="arm"
    HOST_CPU=arm  
    ;;
  aarch64*)
    CFLAGS="$CFLAGS -DCPU=\\\"aarch64\\\""
    supported_os="linux"
    HOST_CPU=aarch64
    ;;  
  *)
    AC_MSG_RESULT([failed])
    AC_MSG_ERROR([Unsupported CPU architecture "$host_cpu"]);;
  esac

  AC_MSG_RESULT([ok])
  AC_SUBST(CFLAGS)
  AC_SUBST(LDFLAGS)
])
