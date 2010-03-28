The directory contains examples of Java daemons.
The examples are compiled using Ant (just type ant). Each example creates a
jar file in ../../dist

SimpleDaemon
------------

SimpleDaemon demonstrates the feature of the daemon offered by
Apache Commons Daemon.
To run it adapt the SimpleDaemon.sh file and connect to it using:
telnet localhost 1200
Additional information in ../native/unix/INSTALL.txt

ServiceDaemon
-------------

ServiceDaemon allows to start programs using the Commons Daemon.

That could be useful when using Cygwin under Win9x because Cygwin only offers
services support under Win NT/2000/XP.
(See in ../native/nt/README how to install jsvc as a service in Win32).

It uses Apache Commons Collections:
http://commons.apache.org/collections/
To use it you need at least commons-collections-1.0
Check in build.xml that the property commons-collections.jar correspond to thei
location of your commons-collections.jar file.

You have to create a file named startfile that uses a property format:
name = string to start the program

For example:
sshd=/usr/sbin/sshd -D
router1=/home/Standard/router/router pop3 pop3.example.net
router2=/home/Standard/router/smtp smtp.example.net
socks5=/usr/local/bin/socks5 -f

To run it adapt the ServiceDaemon.sh file.

AloneService
------------

AloneService is like ServiceDaemon except it does not use the Daemon interface.

ProcrunService
--------------
This is a simple Windows Service application.
It can be run either in Jvm or Java modes.
See ProcrunServiceInstall.cmd for a sample installation script.