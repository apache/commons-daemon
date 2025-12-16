Configuring and Building Apache Commons Daemon on Windows
=========================================================

Using Visual Studio, you can build Apache Commons Daemon.
The Makefile make file has a bunch of documentation about its
options, but a trivial build is simply:

All builds

Set the JAVA_HOME environment variable to point to a Java 8 (or later) JDK.

Windows X64 Build

  For MVS under "C:\Program Files (x86)":
  "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
  
  For MVS under "C:\Program Files":
  "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
  
  nmake CPU=X64

Windows X86 Build

  For MVS under "C:\Program Files (x86)":
  "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"
  
  For MVS under "C:\Program Files":
  "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars.bat"
  
  nmake CPU=X86


Additional configuration
========================

Specifying the installation location (defaults to .\..\..\..\..\..\target which
places the binary in the correct location for a release build):

  nmake CPU=X86 PREFIX=c:\desired\path\of\daemon install

To disable the 'Static Hybrid CRT' build strategy and prevent the the resulting
binaries from working on a clean Windows installation with no additional
dependencies:

  nmake CPU=X86 NO_STATIC_CRT=true
  
  
Release Builds
==============

Release builds must not disable the static hybrid CRT build strategy.

It is not necessary to build a 64-bit version of prunmgr since the 32-bit
version works with both 32-bit and 64-bit services.


Code signing
============

The Windows binaries are signed using the ASF's code signing service. For
details of the service and instructions on how new release managers can request
access to the service see:
https://infra.apache.org/code-signing-access.html
