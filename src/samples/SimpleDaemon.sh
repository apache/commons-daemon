#!/bin/sh
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Small shell script to show how to start the sample services.
#
# Adapt the following lines to your configuration
JAVA_HOME=`echo $JAVA_HOME`
#JAVA_HOME=/opt/java
#JAVA_HOME=/opt/kaffe
DAEMON_HOME=`(cd ../..; pwd)`
TOMCAT_USER=`echo $USER`
CLASSPATH=\
$DAEMON_HOME/dist/commons-daemon.jar:\
$DAEMON_HOME/dist/SimpleDaemon.jar

PATH=$PATH:$DAEMON_HOME/src/native/unix
export PATH

# library could be used to test restart after a core.
#    -Dnative.library=${DAEMON_HOME}/src/samples/Native.so \

# options below are for kaffe.
#    -Xclasspath/a:$CLASSPATH \
#    (to debug the class loader
#    -vmdebug VMCLASSLOADER \

# option below is for the sun JVM.
#    -cp $CLASSPATH \

if [ -f $JAVA_HOME/bin/kaffe ]
then
  CLOPT="-Xclasspath/a:$CLASSPATH"
else
  CLOPT="-cp $CLASSPATH"
fi

jsvc \
    -user $TOMCAT_USER \
    -home $JAVA_HOME \
    $CLOPT \
    -pidfile ./pidfile \
    -wait 90 \
    -debug \
    -outfile toto.txt \
    -errfile '&1' \
    -Dnative.library=${DAEMON_HOME}/src/samples/Native.so \
    SimpleDaemon \
#
# To get a verbose JVM (sun JVM for example)
#-verbose \
# To get a debug of jsvc.
#-debug \

echo "result: $?"
