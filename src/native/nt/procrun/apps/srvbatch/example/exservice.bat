@echo off
REM Copyright 2000-2004 The Apache Software Foundation
REM
REM Licensed under the Apache License, Version 2.0 (the "License");
REM you may not use this file except in compliance with the License.
REM You may obtain a copy of the License at
REM
REM     http://www.apache.org/licenses/LICENSE-2.0
REM
REM Unless required by applicable law or agreed to in writing, software
REM distributed under the License is distributed on an "AS IS" BASIS,
REM WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM See the License for the specific language governing permissions and
REM limitations under the License.
REM

@if not "%ECHO%" == ""  echo %ECHO%
@if "%OS%" == "Windows_NT"  setlocal

set SERVICE_EXECUTABLE=example.exe

REM Figure out the running mode

@if "%1" == "install"   goto cmdInstall
@if "%1" == "uninstall" goto cmdUninstall
@if "%1" == "start"     goto cmdStart
@if "%1" == "stop"      goto cmdStop
@if "%1" == "restart"   goto cmdRestart
echo Usage
goto cmdEnd

:cmdInstall
..\Debug\srvbatch.exe -iwdcl SrvbatchExample "%CD%" "Srvbatch Example Service" "This is an Example service" exservice.bat 
goto cmdEnd

:cmdUninstall
..\Debug\srvbatch.exe -u SrvbatchExample
goto cmdEnd

:cmdStart
%SERVICE_EXECUTABLE% start
goto cmdEnd

:cmdStop
%SERVICE_EXECUTABLE% stop
goto cmdEnd

:cmdRestart
%SERVICE_EXECUTABLE% stop
%SERVICE_EXECUTABLE% start
goto cmdEnd

:cmdEnd
