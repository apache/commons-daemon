To build the JAVA part:
ant dist

The Java portion of Commons Daemon requires Java 1.3 or later to build

To build the native parts:
1 - jsvc:
  jsvc is only for Un*x systems
  cd src/native/unix; configure; make
  You need a gnu make.
  The jsvc executable is created in the dist directory.
  There is a INSTALL.txt in src/native/unix - please have a look at it.

2 - procrun:
  procrun is only for windows
  cd src\native\windows\apps
  cd prunsrv
  nmake [CPU=(X86|X64|I64)]
  cd ..\prunmgr
  nmake [CPU=(X86|X64|I64)]
  (It is also possible to use the MS development tools).

  See also the README files in src\native\windows

To build the documentation: (See http://commons.apache.org/building.html).
mvn site:generate
(Do not forget to get ../commons-build: (cd ..; svn co http://svn.apache.org/repos/asf/commons/proper/commons-build/trunk/ commons-build))

To deploy the documentation to the apache site:
mvn -Dmaven.username=${user.name} site:deploy

To deploy the Java jars to the Nexus staging repo:

mvn clean
mvn deploy -Prelease [-Ptest-deploy]

The test-deploy profile will deploy to target/deploy; omit for the live deploy

Note: do not use clean in the same invocation.
Not sure why, but with Commons-Parent v21 this results deploying the non-Maven archives (zip and tar.gz) as well.
They can be deleted before closing the repo, but it is easier not to create them. 