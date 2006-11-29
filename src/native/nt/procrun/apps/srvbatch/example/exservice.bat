@echo off
REM Licensed to the Apache Software Foundation (ASF) under one or more
REM contributor license agreements.  See the NOTICE file distributed with
REM this work for additional information regarding copyright ownership.
REM The ASF licenses this file to You under the Apache License, Version 2.0
REM (the "License"); you may not use this file except in compliance with
REM the License.  You may obtain a copy of the License at
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
