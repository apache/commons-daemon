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
# That is for linux, if your are using cygwin look to ServiceDaemon.sh.
#
# Adapt the following lines to your configuration
JAVA_HOME=`echo $JAVA_HOME`
DAEMON_HOME=`(cd ../..; pwd)`
USER_HOME=`(cd ../../../..; pwd)`
TOMCAT_USER=`echo $USER`
CLASSPATH=\
$DAEMON_HOME/dist/commons-daemon.jar:\
$USER_HOME/commons-collections-2.1/commons-collections.jar:\
$DAEMON_HOME/dist/aloneservice.jar

$DAEMON_HOME/src/native/unix/jsvc \
    -home $JAVA_HOME \
    -cp $CLASSPATH \
    -pidfile ./pidfile \
    -debug \
    AloneService
#
# To get a verbose JVM
#-verbose \
# To get a debug of jsvc.
#-debug \
#    -user $TOMCAT_USER \
