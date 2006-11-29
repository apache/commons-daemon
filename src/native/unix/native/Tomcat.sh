#!/bin/sh
##############################################################################
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
##############################################################################
#
# Small shell script to show how to start/stop Tomcat using jsvc
# If you want to have Tomcat running on port 80 please modify the server.xml
# file:
#
#    <!-- Define a non-SSL HTTP/1.1 Connector on port 80 -->
#    <Connector className="org.apache.catalina.connector.http.HttpConnector"
#               port="80" minProcessors="5" maxProcessors="75"
#               enableLookups="true" redirectPort="8443"
#               acceptCount="10" debug="0" connectionTimeout="60000"/>
#
# This is for of Tomcat-4.1.x (Apache Tomcat/4.1)
#
# Adapt the following lines to your configuration
JAVA_HOME=/usr/java/jdk1.3.1
CATALINA_HOME=/home1/jakarta/jakarta-tomcat-4.1/build
DAEMON_HOME=/home1/jakarta/jakarta-commons/daemon
TOMCAT_USER=jakarta
TMP_DIR=/var/tmp
CATALINA_OPTS=
CLASSPATH=\
$JAVA_HOME/lib/tools.jar:\
$DAEMON_HOME/dist/commons-daemon.jar:\
$CATALINA_HOME/bin/bootstrap.jar

case "$1" in
  start)
    #
    # Start Tomcat
    #
    $DAEMON_HOME/src/native/unix/jsvc \
    -user $TOMCAT_USER \
    -home $JAVA_HOME \
    -Dcatalina.home=$CATALINA_HOME \
    -Djava.io.tmpdir=$TMP_DIR \
    -outfile $CATALINA_HOME/logs/catalina.out \
    -errfile '&1' \
    $CATALINA_OPTS \
    -cp $CLASSPATH \
    org.apache.catalina.startup.BootstrapService
    #
    # To get a verbose JVM
    #-verbose \
    # To get a debug of jsvc.
    #-debug \
    ;;

  stop)
    #
    # Stop Tomcat
    #
    PID=`cat /var/run/jsvc.pid`
    kill $PID
    ;;

  *)
    echo "Usage tomcat.sh start/stop"
    exit 1;;
esac
