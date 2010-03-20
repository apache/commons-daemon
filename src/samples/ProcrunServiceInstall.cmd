@echo off
rem 
rem Licensed to the Apache Software Foundation (ASF) under one or more
rem contributor license agreements.  See the NOTICE file distributed with
rem this work for additional information regarding copyright ownership.
rem The ASF licenses this file to You under the Apache License, Version 2.0
rem (the "License"); you may not use this file except in compliance with
rem the License.  You may obtain a copy of the License at
rem
rem     http://www.apache.org/licenses/LICENSE-2.0
rem
rem Unless required by applicable law or agreed to in writing, software
rem distributed under the License is distributed on an "AS IS" BASIS,
rem WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
rem See the License for the specific language governing permissions and
rem limitations under the License.

rem Batch script for defining the ProcrunService

rem Copy this file and ProcrunService.jar into the same directory as prunsrv (or adjust the paths below)

setlocal

rem The service name (make sure it does not clash with an existing service)
set SERVICE=ProcrunService

rem my location
set MYPATH=%~dp0

rem location of Prunsrv
set PATH_PRUNSRV=%MYPATH%

rem location of jarfile
set PATH_JAR=%MYPATH%

set PRUNSRV=%PATH_PRUNSRV%prunsrv
echo Installing %SERVICE% if necessary
%PRUNSRV% //IS//%SERVICE% --Install %PATH_PRUNSRV%prunsrv.exe

echo Setting the parameters for %SERVICE%
%PRUNSRV% //US//%SERVICE% --Jvm=auto --StdOutput auto --StdError auto ^
--Classpath=%PATH_JAR%ProcrunService.jar ^
--StartMode=jvm --StartClass=ProcrunService --StartParams=start ^
 --StopMode=jvm  --StopClass=ProcrunService  --StopParams=stop

echo Installation of %SERVICE% is complete