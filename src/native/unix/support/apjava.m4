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
dnl Version $Id: apjava.m4,v 1.1 2003/09/04 23:28:20 yoavs Exp $
dnl -------------------------------------------------------------------------

AC_DEFUN([AP_PROG_JAVAC_WORKS],[
  AC_CACHE_CHECK([wether the Java compiler ($JAVAC) works],ap_cv_prog_javac_works,[
    echo "public class Test {}" > Test.java
    $JAVAC $JAVACFLAGS Test.java > /dev/null 2>&1
    if test $? -eq 0
    then
      rm -f Test.java Test.class
      ap_cv_prog_javac_works=yes
    else
      rm -f Test.java Test.class
      AC_MSG_RESULT(no)
      AC_MSG_ERROR([installation or configuration problem: javac cannot compile])
    fi
  ])
])

AC_DEFUN([AP_PROG_JAVAC],[
  AC_PATH_PROG(JAVAC,javac,AC_MSG_ERROR([javac not found]),$JAVA_HOME/bin:$PATH)
  AP_PROG_JAVAC_WORKS()
  AC_PROVIDE([$0])
  AC_SUBST(JAVAC)
  AC_SUBST(JAVACFLAGS)
])

AC_DEFUN([AP_PROG_JAR],[
  AC_PATH_PROG(JAR,jar,AC_MSG_ERROR([jar not found]),$JAVA_HOME/bin:$PATH)
  AC_PROVIDE([$0])
  AC_SUBST(JAR)
])

AC_DEFUN([AP_JAVA],[
  AC_ARG_WITH(java,[  --with-java=DIR         Specify the location of your JDK installation],[
    AC_MSG_CHECKING([JAVA_HOME])
    if test -d "$withval"
    then
      JAVA_HOME="$withval"
      AC_MSG_RESULT([$JAVA_HOME])
    else
      AC_MSG_RESULT([failed])
      AC_MSG_ERROR([$withval is not a directory])
    fi
    AC_SUBST(JAVA_HOME)
  ])
  if test x"$JAVA_HOME" = x
  then
    AC_MSG_ERROR([Java Home not defined. Rerun with --with-java=[...] parameter])
  fi
])
