The directory contains examples of java daemons.
The examples are compiled using ant (just type ant). Each example creates a
jar file in ../../dist

SimpleDaemon:

SimpleDaemon demonstrates the feature of the daemon ofered by
jakarta-commons-sandbox/daemon.
To run it adapt the SimpleDaemon.sh file and connect to it using:
telnet localhost 1200
Additional information in ../native/unix/INSTALL.txt

ServiceDaemon:

ServiceDaemon allows to start programs using the jakarta daemon.
That could be usefull when using cygwin under win9x because cygwin only offers
services support under win NT/2000/XP.
(See in ../native/nt/README how to install jsvc as a service in win32).

It uses jakarta Commons Collections:
http://jakarta.apache.org/commons/collections.html
To use it you need at least commons-collections-1.0

You have to create a file named startfile that uses a property format:
name = string to start the program

For example:
sshd=/usr/sbin/sshd -D
router1=/home/Standard/router/router pop3 pop3.example.net
router2=/home/Standard/router/smtp smtp.example.net
socks5=/usr/local/bin/socks5 -f

To run it adapt the ServiceDaemon.sh file. 
