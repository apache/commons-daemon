#!/bin/sh
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
JAVA_HOME=/usr/java/j2sdk1.4.2_03
CATALINA_HOME=/home/tomcat5/jakarta-tomcat-5/build
DAEMON_HOME=/home/tomcat5/jakarta-commons/daemon
TOMCAT_USER=tomcat5
TMP_DIR=/var/tmp
CATALINA_OPTS=
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
    -Djava.io.tmpdir=$TMP_DIR \
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
