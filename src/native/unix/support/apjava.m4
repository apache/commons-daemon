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

AC_DEFUN([AP_FIND_JAVA],[
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
  if test "x$JAVA_HOME" = x
  then
    AC_MSG_CHECKING([for JDK location])
    # Oh well, nobody set JAVA_HOME, have to guess
    # Check if we have java in the PATH.
    java_prog="`which java 2>/dev/null || true`"
    if test "x$java_prog" != x
    then
      java_bin="`dirname $java_prog`"
      java_top="`dirname $java_bin`"
      if test -f "$java_top/include/jni.h"
      then
        JAVA_HOME="$java_top"
        AC_MSG_RESULT([${java_top}])
      fi
    fi
  fi
  if test x"$JAVA_HOME" = x
  then
    AC_MSG_ERROR([Java Home not defined. Rerun with --with-java=[...] parameter])
  fi
])

AC_DEFUN([AP_FIND_JAVA_OS],[
  tempval=""
  JAVA_OS=""
  AC_ARG_WITH(os-type,[  --with-os-type[=SUBDIR]   Location of JDK os-type subdirectory.],
  [
    tempval=$withval
    if test ! -d "$JAVA_HOME/$tempval"
    then
      AC_MSG_ERROR(Not a directory: ${JAVA_HOME}/${tempval})
    fi
    JAVA_OS=$tempval
  ],
  [
    AC_MSG_CHECKING(for JDK os include directory)
    JAVA_OS=NONE
    if test -f $JAVA_HOME/$JAVA_INC/jni_md.h
    then
      JAVA_OS=""
    else
      for f in $JAVA_HOME/$JAVA_INC/*/jni_md.h
      do
        if test -f $f; then
            JAVA_OS=`dirname $f`
            JAVA_OS=`basename $JAVA_OS`
            echo " $JAVA_OS"
            break
        fi
      done
      if test "x$JAVA_OS" = "xNONE"; then
        AC_MSG_RESULT(Cannot find jni_md.h in ${JAVA_HOME}/${OS})
        AC_MSG_ERROR(You should retry --with-os-type=SUBDIR)
      fi
    fi
  ])
])
