# Microsoft Developer Studio Project File - Name="vdmonisvc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=vdmonisvc - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vdmonisvc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vdmonisvc.mak" CFG="vdmonisvc - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vdmonisvc - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "vdmonisvc - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vdmonisvc - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\vdmonisvc"
# PROP BASE Intermediate_Dir ".\vdmonisvc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../lib" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "WINPC" /FR /YX /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib gdi32.lib user32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib msvcrt.lib oldnames.lib /nologo /subsystem:console /machine:I386 /nodefaultlib /out:"../../bin/vdmonisvc.exe"

!ELSEIF  "$(CFG)" == "vdmonisvc - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\CleanSv0"
# PROP BASE Intermediate_Dir ".\CleanSv0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MD /W3 /Gm /GX /Zi /Od /I "../../lib" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "WINPC" /FR /YX /FD /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 user32.lib oldnames.lib kernel32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib msvcrt.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib /out:"../../bin/vdmonisvc.exe"

!ENDIF 

# Begin Target

# Name "vdmonisvc - Win32 Release"
# Name "vdmonisvc - Win32 Debug"
# Begin Group "supcalls_nt"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\supcalls_nt\vdenv.c
# End Source File
# End Group
# Begin Group "signals"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\signals\kills.c
# End Source File
# End Group
# Begin Group "moni"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\moni\vdmonisvc.c
# End Source File
# End Group
# Begin Group "moni_nt"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\moni_nt\service.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\vdmonisvc.rc
# End Source File
# End Target
# End Project
