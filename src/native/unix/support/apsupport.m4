dnl =========================================================================
dnl
dnl                 The Apache Software License,  Version 1.1
dnl
dnl          Copyright (c) 1999-2001 The Apache Software Foundation.
dnl                           All rights reserved.
dnl
dnl =========================================================================
dnl
dnl Redistribution and use in source and binary forms,  with or without modi-
dnl fication, are permitted provided that the following conditions are met:
dnl
dnl 1. Redistributions of source code  must retain the above copyright notice
dnl    notice, this list of conditions and the following disclaimer.
dnl
dnl 2. Redistributions  in binary  form  must  reproduce the  above copyright
dnl    notice,  this list of conditions  and the following  disclaimer in the
dnl    documentation and/or other materials provided with the distribution.
dnl
dnl 3. The end-user documentation  included with the redistribution,  if any,
dnl    must include the following acknowlegement:
dnl
dnl       "This product includes  software developed  by the Apache  Software
dnl        Foundation <http://www.apache.org/>."
dnl
dnl    Alternately, this acknowlegement may appear in the software itself, if
dnl    and wherever such third-party acknowlegements normally appear.
dnl
dnl 4. The names  "The  Jakarta  Project",  "WebApp",  and  "Apache  Software
dnl    Foundation"  must not be used  to endorse or promote  products derived
dnl    from this  software without  prior  written  permission.  For  written
dnl    permission, please contact <apache@apache.org>.
dnl
dnl 5. Products derived from this software may not be called "Apache" nor may
dnl    "Apache" appear in their names without prior written permission of the
dnl    Apache Software Foundation.
dnl
dnl THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES
dnl INCLUDING, BUT NOT LIMITED TO,  THE IMPLIED WARRANTIES OF MERCHANTABILITY
dnl AND FITNESS FOR  A PARTICULAR PURPOSE  ARE DISCLAIMED.  IN NO EVENT SHALL
dnl THE APACHE  SOFTWARE  FOUNDATION OR  ITS CONTRIBUTORS  BE LIABLE  FOR ANY
dnl DIRECT,  INDIRECT,   INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR  CONSEQUENTIAL
dnl DAMAGES (INCLUDING,  BUT NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE GOODS
dnl OR SERVICES;  LOSS OF USE,  DATA,  OR PROFITS;  OR BUSINESS INTERRUPTION)
dnl HOWEVER CAUSED AND  ON ANY  THEORY  OF  LIABILITY,  WHETHER IN  CONTRACT,
dnl STRICT LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
dnl ANY  WAY  OUT OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF  ADVISED  OF THE
dnl POSSIBILITY OF SUCH DAMAGE.
dnl
dnl =========================================================================
dnl
dnl This software  consists of voluntary  contributions made  by many indivi-
dnl duals on behalf of the  Apache Software Foundation.  For more information
dnl on the Apache Software Foundation, please see <http://www.apache.org/>.
dnl
dnl =========================================================================

dnl -------------------------------------------------------------------------
dnl Author  Pier Fumagalli <mailto:pier.fumagalli@eng.sun.com>
dnl Version $Id: apsupport.m4,v 1.3 2003/09/25 13:38:27 jfclere Exp $
dnl -------------------------------------------------------------------------

AC_DEFUN(AP_SUPPORTED_HOST,[
  AC_MSG_CHECKING([C flags dependant on host system type])
  case $host_cpu in
  powerpc)
    CFLAGS="$CFLAGS -DCPU=\\\"$host_cpu\\\"" ;;
  sparc)
    CFLAGS="$CFLAGS -DCPU=\\\"$host_cpu\\\"" ;;
  i?86)
    CFLAGS="$CFLAGS -DCPU=\\\"i386\\\"" ;;
  bs2000)
    CFLAGS="$CFLAGS -DCPU=\\\"osd\\\" -DCHARSET_EBCDIC -DOSD_POSIX"
    supported_os="osd"
    LDFLAGS="-Kno_link_stdlibs -B llm4 -l BLSLIB $LDFLAGS"
    LDCMD="cc"
    ;;
  mips)
    CFLAGS="$CFLAGS -DCPU=\\\"mips\\\""
    supported_os="mips"
    ;;
  *)
    AC_MSG_RESULT([failed])
    AC_MSG_ERROR([Unsupported CPU architecture "$host_cpu"]);;
  esac

  case $host_os in
  darwin*)
    CFLAGS="$CFLAGS -DOS_DARWIN -DDSO_DYLD"
    supported_os="darwin"
    ;;
  solaris*)
    CFLAGS="$CFLAGS -DOS_SOLARIS -DDSO_DLFCN"
    supported_os="solaris"
    LDFLAGS="$LDFLAGS -ldl -lthread"
    ;;
  linux*)
    CFLAGS="$CFLAGS -DOS_LINUX -DDSO_DLFCN"
    supported_os="linux"
    LDFLAGS="$LDFLAGS -ldl"
    ;;
  cygwin)
    CFLAGS="$CFLAGS -DOS_CYGWIN -DDSO_DLFCN -DNO_SETSID"
    supported_os="win32"
    ;;
  sysv)
    CFLAGS="$CFLAGS -DOS_SYSV -DDSO_DLFCN"
    LDFLAGS="$LDFLAGS -ldl"
    ;;
  sysv4)
    CFLAGS="$CFLAGS -DOS_SYSV -DDSO_DLFCN -Kthread"
    LDFLAGS="-Kthread $LDFLAGS -ldl"
    ;;
  freebsd4.?)
    CFLAGS="$CFLAGS -DOS_FREEBSD -DDSO_DLFCN -D_THREAD_SAFE -pthread"
    LDFLAGS="-pthread $LDFLAGS"
    supported_os="freebsd"
    ;;
  *)
    AC_MSG_RESULT([failed])
    AC_MSG_ERROR([Unsupported operating system "$host_os"])
    ;;
  esac
  AC_MSG_RESULT([ok])
  AC_SUBST(CFLAGS)
])
