#!/bin/sh
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# for linux ;-) and Mac OS X
case `uname -s` in
  Darwin)
    JAVA_HOME=/System/Library/Frameworks/JavaVM.framework/Versions/CurrentJDK
    INCLUDE=Headers
    OS=
    FLAGS=-dynamiclib
    ;;
  Linux)
    OS=linux
    INCLUDE=include
    FLAGS=-shared
    ;;
esac

gcc -c -I${JAVA_HOME}/${INCLUDE} -I${JAVA_HOME}/${INCLUDE}/${OS} Native.c
gcc ${FLAGS} -o Native.so Native.o
