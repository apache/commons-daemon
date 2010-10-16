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
# That is for Tomcat-5.0.x (Apache Tomcat/5.0)
#
# Adapt the following lines to your configuration
JAVA_HOME=/home2/java/j2sdk1.4.2_03
CATALINA_HOME=/home/tomcat5/tomcat5/jakarta-tomcat-5/build
DAEMON_HOME=/home/jfclere/daemon
TOMCAT_USER=tomcat5

# for multi instances adapt those lines.
TMP_DIR=/var/tmp
PID_FILE=/var/run/jsvc.pid
CATALINA_BASE=/home/tomcat5/tomcat5/jakarta-tomcat-5/build

CATALINA_OPTS="-Djava.library.path=/home/jfclere/jakarta-tomcat-connectors/jni/native/.libs"
CLASSPATH=\
$JAVA_HOME/lib/tools.jar:\
$CATALINA_HOME/bin/commons-daemon.jar:\
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
    -Dcatalina.base=$CATALINA_BASE \
    -Djava.io.tmpdir=$TMP_DIR \
    -wait 10 \
    -pidfile $PID_FILE \
    -outfile $CATALINA_HOME/logs/catalina.out \
    -errfile '&1' \
    $CATALINA_OPTS \
    -cp $CLASSPATH \
    org.apache.catalina.startup.Bootstrap
    #
    # To get a verbose JVM
    #-verbose \
    # To get a debug of jsvc.
    #-debug \
    exit $?
    ;;

  stop)
    #
    # Stop Tomcat
    #
    $DAEMON_HOME/src/native/unix/jsvc \
    -stop \
    -pidfile $PID_FILE \
    org.apache.catalina.startup.Bootstrap
    exit $?
    ;;

  *)
    echo "Usage tomcat.sh start/stop"
    exit 1;;
esac
