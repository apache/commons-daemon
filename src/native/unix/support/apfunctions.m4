dnl
dnl Copyright 1999-2004 The Apache Software Foundation
dnl
dnl Licensed under the Apache License, Version 2.0 (the "License");
dnl you may not use this file except in compliance with the License.
dnl You may obtain a copy of the License at
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
dnl Version $Id: apfunctions.m4,v 1.2 2004/02/27 08:40:46 jfclere Exp $
dnl -------------------------------------------------------------------------

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
