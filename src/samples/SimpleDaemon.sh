#!/bin/sh
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
    SimpleDaemon \
#
# To get a verbose JVM
#-verbose \
# To get a debug of jsvc.
#-debug \
