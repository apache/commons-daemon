# Microsoft Developer Studio Project File - Name="procrun" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=procrun - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "procrun.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "procrun.mak" CFG="procrun - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "procrun - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "procrun - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "procrun - Win32 Debug CONSOLE" (based on "Win32 (x86) Application")
!MESSAGE "procrun - Win32 Release CONSOLE" (based on "Win32 (x86) Application")
!MESSAGE "procrun - Win32 DebugDLL" (based on "Win32 (x86) Application")
!MESSAGE "procrun - Win32 ReleaseDLL" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "procrun - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "$(JAVA_HOME)\include" /I "$(JAVA_HOME)\include\win32" /D "_WIN32" /D "WIN32" /D "_DEBUG" /D "_DEBUG_TRACE" /D "_WINDOWS" /D "STRICT" /D _WIN32_WINNT=0x0400 /D "PROCRUN_WINAPP" /D "_MBCS" /GZ PRECOMP_VC7_TOBEREMOVED /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "$(JAVA_HOME)\include" /I "$(JAVA_HOME)\include\win32" /D "_WIN32" /D "WIN32" /D "_DEBUG" /D "_DEBUG_TRACE" /D "_WINDOWS" /D "STRICT" /D _WIN32_WINNT=0x0400 /D "PROCRUN_WINAPP" /D "_MBCS" /GZ /c
# ADD BASE MTL /nologo /win32
# ADD MTL /nologo /win32
# ADD BASE RSC /l 0x409 /d "_MSC_VER"
# ADD RSC /l 0x409 /d "_MSC_VER"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib shlwapi.lib /nologo /subsystem:windows /debug /machine:IX86 /out:"Debug\procrunw.exe" /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib shlwapi.lib /nologo /subsystem:windows /debug /machine:IX86 /out:"Debug\procrunw.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "procrun - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /Zi /O2 /I "$(JAVA_HOME)\include" /I "$(JAVA_HOME)\include\win32" /D "_WIN32" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "STRICT" /D _WIN32_WINNT=0x0400 /D "PROCRUN_WINAPP" /D "_MBCS" /GF PRECOMP_VC7_TOBEREMOVED /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "$(JAVA_HOME)\include" /I "$(JAVA_HOME)\include\win32" /D "_WIN32" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "STRICT" /D _WIN32_WINNT=0x0400 /D "PROCRUN_WINAPP" /D "_MBCS" /GF /c
# ADD BASE MTL /nologo /win32
# ADD MTL /nologo /win32
# ADD BASE RSC /l 0x409 /d "_MSC_VER"
# ADD RSC /l 0x409 /d "_MSC_VER"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib shlwapi.lib /nologo /subsystem:windows /debug /machine:IX86 /out:"bin\procrunw.exe" /pdbtype:sept /opt:ref /opt:icf
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib shlwapi.lib /nologo /subsystem:windows /debug /machine:IX86 /out:"bin\procrunw.exe" /pdbtype:sept /opt:ref /opt:icf

!ELSEIF  "$(CFG)" == "procrun - Win32 Debug CONSOLE"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugCONSOLE"
# PROP BASE Intermediate_Dir "DebugCONSOLE"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugCONSOLE"
# PROP Intermediate_Dir "DebugCONSOLE"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "$(JAVA_HOME)\include" /I "$(JAVA_HOME)\include\win32" /D "_WIN32" /D "WIN32" /D "_DEBUG" /D "_DEBUG_TRACE" /D "_CONSOLE" /D "STRICT" /D _WIN32_WINNT=0x0400 /D "PROCRUN_CONSOLE" /D "_MBCS" /GZ PRECOMP_VC7_TOBEREMOVED /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "$(JAVA_HOME)\include" /I "$(JAVA_HOME)\include\win32" /D "_WIN32" /D "WIN32" /D "_DEBUG" /D "_DEBUG_TRACE" /D "_CONSOLE" /D "STRICT" /D _WIN32_WINNT=0x0400 /D "PROCRUN_CONSOLE" /D "_MBCS" /GZ /c
# ADD BASE MTL /nologo /win32
# ADD MTL /nologo /win32
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib shlwapi.lib /nologo /subsystem:console /debug /machine:IX86 /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib shlwapi.lib /nologo /subsystem:console /debug /machine:IX86 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "procrun - Win32 Release CONSOLE"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseCONSOLE"
# PROP BASE Intermediate_Dir "ReleaseCONSOLE"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseCONSOLE"
# PROP Intermediate_Dir "ReleaseCONSOLE"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /Zi /O2 /I "$(JAVA_HOME)\include" /I "$(JAVA_HOME)\include\win32" /D "_WIN32" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "STRICT" /D _WIN32_WINNT=0x0400 /D "PROCRUN_CONSOLE" /D "_MBCS" /GF PRECOMP_VC7_TOBEREMOVED /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "$(JAVA_HOME)\include" /I "$(JAVA_HOME)\include\win32" /D "_WIN32" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "STRICT" /D _WIN32_WINNT=0x0400 /D "PROCRUN_CONSOLE" /D "_MBCS" /GF /c
# ADD BASE MTL /nologo /win32
# ADD MTL /nologo /win32
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib shlwapi.lib /nologo /subsystem:console /debug /machine:IX86 /out:"bin\procrun.exe" /pdbtype:sept /opt:ref /opt:icf
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib shlwapi.lib /nologo /subsystem:console /debug /machine:IX86 /out:"bin\procrun.exe" /pdbtype:sept /opt:ref /opt:icf

!ELSEIF  "$(CFG)" == "procrun - Win32 DebugDLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugDLL"
# PROP BASE Intermediate_Dir "DebugDLL"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugDLL"
# PROP Intermediate_Dir "DebugDLL"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "$(JAVA_HOME)\include" /I "$(JAVA_HOME)\include\win32" /D "_WIN32" /D "WIN32" /D "_DEBUG" /D "_DEBUG_TRACE" /D "_WINDOWS" /D "STRICT" /D _WIN32_WINNT=0x0400 /D "PROCRUN_WINDLL" /D "_MBCS" /GZ PRECOMP_VC7_TOBEREMOVED /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "$(JAVA_HOME)\include" /I "$(JAVA_HOME)\include\win32" /D "_WIN32" /D "WIN32" /D "_DEBUG" /D "_DEBUG_TRACE" /D "_WINDOWS" /D "STRICT" /D _WIN32_WINNT=0x0400 /D "PROCRUN_WINDLL" /D "_MBCS" /GZ /c
# ADD BASE MTL /nologo /win32
# ADD MTL /nologo /win32
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib shlwapi.lib /nologo /subsystem:windows /dll /debug /machine:IX86 /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib shlwapi.lib /nologo /subsystem:windows /dll /debug /machine:IX86 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "procrun - Win32 ReleaseDLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseDLL"
# PROP BASE Intermediate_Dir "ReleaseDLL"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseDLL"
# PROP Intermediate_Dir "ReleaseDLL"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /Zi /O2 /I "$(JAVA_HOME)\include" /I "$(JAVA_HOME)\include\win32" /D "_WIN32" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "STRICT" /D _WIN32_WINNT=0x0400 /D "PROCRUN_WINDLL" /D "_MBCS" /GF PRECOMP_VC7_TOBEREMOVED /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "$(JAVA_HOME)\include" /I "$(JAVA_HOME)\include\win32" /D "_WIN32" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "STRICT" /D _WIN32_WINNT=0x0400 /D "PROCRUN_WINDLL" /D "_MBCS" /GF /c
# ADD BASE MTL /nologo /win32
# ADD MTL /nologo /win32
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib shlwapi.lib /nologo /subsystem:windows /dll /debug /machine:IX86 /out:"bin\procrun.dll" /pdbtype:sept /opt:ref /opt:icf
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib shlwapi.lib /nologo /subsystem:windows /dll /debug /machine:IX86 /out:"bin\procrun.dll" /pdbtype:sept /opt:ref /opt:icf

!ENDIF 

# Begin Target

# Name "procrun - Win32 Debug"
# Name "procrun - Win32 Release"
# Name "procrun - Win32 Debug CONSOLE"
# Name "procrun - Win32 Release CONSOLE"
# Name "procrun - Win32 DebugDLL"
# Name "procrun - Win32 ReleaseDLL"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;def;odl;idl;hpj;bat;asm"
# Begin Source File

SOURCE=procgui.c
DEP_CPP_PROCG=\
	".\procrun.h"\
	"C:\DEVTOOLS\JAVA\142\include\jni.h"\
	"C:\DEVTOOLS\JAVA\142\include\win32\jni_md.h"\
	
# End Source File
# Begin Source File

SOURCE=procrun.c
DEP_CPP_PROCR=\
	".\procrun.h"\
	"C:\DEVTOOLS\JAVA\142\include\jni.h"\
	"C:\DEVTOOLS\JAVA\142\include\win32\jni_md.h"\
	
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;inc"
# Begin Source File

SOURCE=procrun.h
# End Source File
# Begin Source File

SOURCE=resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "rc;ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE="jakarta-banner.bmp"
# End Source File
# Begin Source File

SOURCE=procrun.rc

!IF  "$(CFG)" == "procrun - Win32 Debug"

!ELSEIF  "$(CFG)" == "procrun - Win32 Release"

# ADD BASE RSC /l 0x41a
# ADD RSC /l 0x41a /d "PROCRUN_WINAPP"

!ELSEIF  "$(CFG)" == "procrun - Win32 Debug CONSOLE"

!ELSEIF  "$(CFG)" == "procrun - Win32 Release CONSOLE"

!ELSEIF  "$(CFG)" == "procrun - Win32 DebugDLL"

!ELSEIF  "$(CFG)" == "procrun - Win32 ReleaseDLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=proctry.ico
# End Source File
# Begin Source File

SOURCE=tomcatp.ico
# End Source File
# Begin Source File

SOURCE=tomcatr.ico
# End Source File
# Begin Source File

SOURCE=.\tomcats.ico
# End Source File
# End Group
# Begin Source File

SOURCE=License.rtf
# End Source File
# End Target
# End Project
