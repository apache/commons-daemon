#!/bin/sh
#
# Small shell script to show how to start the sample services.
#
# Adapt the following lines to your configuration
JAVA_HOME=/usr/java/jdk1.3.1
DAEMON_HOME=/home1/jakarta/jakarta-commons-sandbox/daemon
TOMCAT_USER=jakarta
CLASSPATH=\
$DAEMON_HOME/dist/commons-daemon.jar:\
$DAEMON_HOME/dist/SimpleDaemon.jar

$DAEMON_HOME/dist/jsvc \
    -user $TOMCAT_USER \
    -home $JAVA_HOME \
    -cp $CLASSPATH \
    SimpleDaemon 
#
# To get a verbose JVM
#-verbose \
# To get a debug of jsvc.
#-debug \
