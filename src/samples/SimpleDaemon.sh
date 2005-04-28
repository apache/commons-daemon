#!/bin/sh
#
#   Copyright 1999-2005 The Apache Software Foundation
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
#
# Small shell script to show how to start the sample services.
#
# Adapt the following lines to your configuration
JAVA_HOME=`echo $JAVA_HOME`
DAEMON_HOME=`(cd ../..; pwd)`
TOMCAT_USER=`echo $USER`
CLASSPATH=\
$DAEMON_HOME/dist/commons-daemon.jar:\
$DAEMON_HOME/dist/SimpleDaemon.jar

$DAEMON_HOME/src/native/unix/jsvc \
    -user $TOMCAT_USER \
    -home $JAVA_HOME \
    -cp $CLASSPATH \
    -pidfile ./pidfile \
    -wait 90 \
    -debug \
    -outfile toto.txt \
    -errfile '&1' \
    -Dnative.library=${DAEMON_HOME}/src/samples/Native.so \
    SimpleDaemon \
#
# To get a verbose JVM
#-verbose \
# To get a debug of jsvc.
#-debug \

echo "result: $?"
