#!/bin/sh
# 
#   Copyright 1999-2004 The Apache Software Foundation
# 
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
# 
#       http://www.apache.org/licenses/LICENSE-2.0
# 
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

# @author  Pier Fumagalli <mailto:pier.fumagalli@eng.sun.com>
# @version $Id: buildconf.sh,v 1.3 2004/02/27 08:40:46 jfclere Exp $

# The cache of automake always brings problems when changing *.m4 files.
rm -rf autom4te.cache

if test -f configure.in ; then
  autoconf
  if test $? -ne 0 ; then
    echo "$0: cannot generate configure script"
  else
    echo "$0: configure script generated successfully"
  fi
else
  echo "$0: cannot find source file configure.in"
fi
