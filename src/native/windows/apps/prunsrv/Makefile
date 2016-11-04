# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# @author Mladen Turk
#
#
TARGET = EXE
PROJECT = prunsrv
UNICODE = 1
!include <..\..\include\Makefile.inc>

!IF !DEFINED(PREFIX) || "$(PREFIX)" == ""
PREFIX = .\..\..\dist
!ENDIF
!IF !DEFINED(SRCDIR) || "$(SRCDIR)" == ""
SRCDIR = .\..\..
!ENDIF

!IF "$(CPU)" == "X86"
LFLAGS = $(LFLAGS) /stack:0x64000
!ENDIF

LFLAGS = $(LFLAGS) /version:1.0 /DYNAMICBASE  /NXCOMPAT
LIBS = $(LIBS) user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib shlwapi.lib netapi32.lib
INCLUDES = -I$(SRCDIR)\include -I$(SRCDIR)\src $(JAVA_INCLUDES)

PDBFLAGS = -Fo$(WORKDIR)\ -Fd$(WORKDIR)\$(PROJECT)-src
OBJECTS = \
	$(WORKDIR)\cmdline.obj \
	$(WORKDIR)\console.obj \
	$(WORKDIR)\gui.obj \
	$(WORKDIR)\handles.obj \
	$(WORKDIR)\javajni.obj \
	$(WORKDIR)\log.obj \
	$(WORKDIR)\mclib.obj \
	$(WORKDIR)\registry.obj \
	$(WORKDIR)\rprocess.obj \
	$(WORKDIR)\service.obj \
	$(WORKDIR)\utils.obj \
	$(WORKDIR)\prunsrv.obj

BUILDEXE = $(WORKDIR)\$(PROJECT).exe
BUILDLOC = $(PREFIX)
!IF "$(CPU)" == "X64"
BUILDLOC = $(PREFIX)\amd64
!ELSEIF "$(CPU)" == "I64"
BUILDLOC = $(PREFIX)\ia64
!ENDIF
BUILDPDB = $(WORKDIR)\$(PROJECT).pdb
BUILDRES = $(WORKDIR)\$(PROJECT).res
BUILDMAN = $(BUILDEXE).manifest

all : $(WORKDIR) $(BUILDEXE)

$(BUILDLOC) :
	@if not exist "$(BUILDLOC)\$(NULL)" mkdir "$(BUILDLOC)"

$(WORKDIR) :
	@$(MAKEWORKDIR)

{$(SRCDIR)\src}.c{$(WORKDIR)}.obj:
	$(CC) $(CFLAGS) $(INCLUDES) $(PDBFLAGS) $<

{$(SRCDIR)\apps\prunsrv}.c{$(WORKDIR)}.obj:
	$(CC) $(CFLAGS) $(INCLUDES) $(PDBFLAGS) $<

$(BUILDRES): $(SRCDIR)/apps/prunsrv/prunsrv.rc
	$(RC) $(RCFLAGS) /i "$(SRCDIR)\include" /fo $(BUILDRES) $(SRCDIR)/apps/prunsrv/prunsrv.rc

$(BUILDEXE): $(WORKDIR) $(OBJECTS) $(BUILDRES)
	$(LINK) $(LFLAGS) $(OBJECTS) $(BUILDRES) $(LIBS) $(LDIRS) /pdb:$(BUILDPDB) /out:$(BUILDEXE)
	IF EXIST $(BUILDMAN) \
		mt -nologo -manifest $(BUILDMAN) -outputresource:$(BUILDEXE);1

clean:
	@$(CLEANTARGET)

install: $(BUILDLOC) $(WORKDIR) $(BUILDEXE)
	@xcopy "$(WORKDIR)\*.exe" "$(BUILDLOC)" /Y /Q
