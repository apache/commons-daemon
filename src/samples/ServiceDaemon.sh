#!/bin/sh
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
#    -user $TOMCAT_USER \
