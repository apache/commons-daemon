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

AC_DEFUN(AP_MSG_HEADER,[
  printf "*** %s ***\n" "$1" 1>&2
  AC_PROVIDE([$0])
])

AC_DEFUN(AP_CANONICAL_HOST_CHECK,[
  AC_MSG_CHECKING([cached host system type])
  if { test x"${ac_cv_host_system_type+set}" = x"set"  &&
       test x"$ac_cv_host_system_type" != x"$host" ; }
  then
    AC_MSG_RESULT([$ac_cv_host_system_type])
    AC_MSG_ERROR([remove the \"$cache_file\" file and re-run configure])
  else
    AC_MSG_RESULT(ok)
    ac_cv_host_system_type="$host"
  fi
  AC_PROVIDE([$0])
])

dnl Iteratively interpolate the contents of the second argument
dnl until interpolation offers no new result. Then assign the
dnl final result to $1.
dnl
dnl Example:
dnl
dnl foo=1
dnl bar='${foo}/2'
dnl baz='${bar}/3'
dnl AP_EXPAND_VAR(fraz, $baz)
dnl   $fraz is now "1/2/3"
dnl
AC_DEFUN([AP_EXPAND_VAR], [
ap_last=
ap_cur="$2"
while test "x${ap_cur}" != "x${ap_last}";
do
  ap_last="${ap_cur}"
  ap_cur=`eval "echo ${ap_cur}"`
done
$1="${ap_cur}"
])

dnl
dnl AP_CONFIG_NICE(filename)
dnl
dnl Saves a snapshot of the configure command-line for later reuse
dnl
AC_DEFUN([AP_CONFIG_NICE], [
  rm -f $1
  cat >$1<<EOF
#! /bin/sh
#
# Created by configure

EOF
  if test -n "$CC"; then
    echo "CC=\"$CC\"; export CC" >> $1
  fi
  if test -n "$CFLAGS"; then
    echo "CFLAGS=\"$CFLAGS\"; export CFLAGS" >> $1
  fi
  if test -n "$CPPFLAGS"; then
    echo "CPPFLAGS=\"$CPPFLAGS\"; export CPPFLAGS" >> $1
  fi
  if test -n "$LDFLAGS"; then
    echo "LDFLAGS=\"$LDFLAGS\"; export LDFLAGS" >> $1
  fi
  if test -n "$LIBS"; then
    echo "LIBS=\"$LIBS\"; export LIBS" >> $1
  fi
  if test -n "$STRIPFLAGS"; then
    echo "STRIPFLAGS=\"$STRIPFLAGS\"; export STRIPFLAGS" >> $1
  fi
  if test -n "$INCLUDES"; then
    echo "INCLUDES=\"$INCLUDES\"; export INCLUDES" >> $1
  fi
# Retrieve command-line arguments.
  eval "set x $[0] $ac_configure_args"
  shift

  for arg
  do
    AP_EXPAND_VAR(arg, $arg)
    echo "\"[$]arg\" \\" >> $1
  done
  echo '"[$]@"' >> $1
  chmod +x $1
])dnl
