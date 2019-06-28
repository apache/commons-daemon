Configuring and Building Apache Commons Daemon on Windows
=========================================================

Using Visual Studio, you can build Apache Commons Daemon.
The Makefile make file has a bunch of documentation about its
options, but a trivial build is simply;

  nmake CPU=X86
  nmake CPU=X86 PREFIX=c:\desired\path\of\daemon install


Release Builds
==============

We jump through some additional hoops for release builds to avoid additional
dependencies over and above those DLLs that are known to be present on every
Windows install. If you build the binaries with a recent version of Visual
Studio then it is likely the resulting binaries will have additional
dependencies. You can check this with the Depends.exe tool provided with Visual
Studio.

Release builds are build with Mladen Turk's (mturk) Custom Microsoft Compiler
Toolkit Compilation. This can be obtained from:
https://github.com/mturk/cmsc
Hash: cb6be932c8c95a46262a64a89e68aae620dfdcee
Compile as per <cmsc-root>/tools/README.txt

A detailed description of the full environment used for recent release builds is
provided at:
https://cwiki.apache.org/confluence/display/TOMCAT/Common+Native+Build+Environment

The steps to produce the Windows binaries is then:

1. cd $GIT_CLONE_DIR\src\native\windows\apps\prunmgr

2. $CMCS_ROOT\setenv.bat /x86

3. nmake -f Makefile

4. cd ..\prunsrv

5. nmake -f Makefile

6. $CMCS_ROOT\setenv.bat /x64

7. nmake -f Makefile


It is not necessary to build a 64-bit version of prunmgr since the 32-bit
version works with both 32-bit and 64-bit services.
