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
JAVA_HOME=/cygdrive/c/jdk1.3.1_02
HOME=c:\\cygwin\\home\\Standard
DAEMON_HOME=$HOME\\jakarta-commons-sandbox\\daemon
DAEMON_HOME_SH=/home/Standard/jakarta-commons-sandbox/daemon
TOMCAT_USER=jakarta
CLASSPATH=\
$DAEMON_HOME\\dist\\commons-daemon.jar\;\
$HOME\\commons-collections-1.0\\commons-collections.jar\;\
$DAEMON_HOME\\dist\\Service.jar

$DAEMON_HOME_SH/dist/jsvc \
    -home $JAVA_HOME \
    -cp $CLASSPATH \
    ServiceDaemon 
#
# To get a verbose JVM
#-verbose \
# To get a debug of jsvc.
#-debug \
