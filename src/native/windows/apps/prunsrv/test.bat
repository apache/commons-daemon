@ECHO OFF
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

rem ---------------------------------------------------------------------------
rem Test script for procrun
rem

SETLOCAL ENABLEEXTENSIONS
SET mypath=%cd%
FOR /F "tokens=3 USEBACKQ" %%A IN (`ATTRIB /s prunsrv.exe`) DO (
  SET myserv=%%A
)
FOR /F "tokens=3 USEBACKQ" %%A IN (`Attrib ..\..\..\..\..\target\*-tests.jar`) DO (
  SET myjar=%%A
)
ECHO %myserv%
ECHO %myjar%
ECHO %mypath%

echo "cleaning..."
call :mybanner stopping
%myserv% //SS//TestService
if %errorlevel% neq 0 (
  echo "Stop service failed"
)
call :mybanner deleting
%myserv% //DS//TestService
if %errorlevel% equ 9 (
  echo "delete failed, not installed"
) else (
  if %errorlevel% neq 0 (
    echo "delete failed"
    exit 1
  )
)
echo "cleaned"

rem install service with notimeout and no wait
echo ""
call :mybanner "install service with notimeout and no wait"
echo ""
%myserv% //IS//TestService --Description="Procrun tests" --DisplayName="Test Service" --Install=%myserv% --StartMode=exe --StartPath=%mypath% --StartImage=cmd.exe ++StartParams="/c java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon" --StopMode=exe --StopPath=%mypath% --StopImage=cmd.exe ++StopParams="/c java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon 1" --LogPath=%mypath% --LogLevel=Debug --StdOutput=auto --StdError=auto
if %errorlevel% neq 0 (
  echo "install failed"
  exit 1
)

call :startservice
call :testservice
call :mybanner stopping
%myserv% //SS//TestService
if %errorlevel% neq 0 (
  echo "No timeout No wait stop failed"
  exit 1
)

rem start the service and make the java part to stop
call :startservice
java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon 1
if %errorlevel% neq 0 (
  echo "sending command 1 to the java part failed"
  exit 1
)
java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon 0
if %errorlevel% equ 0 (
  echo "command 1 to the java part failed, service still there"
  exit 1
)
echo "Stop an exited service..."
call :stopservice
call :deleteservice

rem install service with timeout 10 and 60 sec wait
echo ""
call :mybanner "install service with timeout 10 and 60 sec wait"
echo ""
%myserv% //IS//TestService --Description="Procrun tests" --DisplayName="Test Service" --Install=%myserv% --StartMode=exe --StartPath=%mypath% --StartImage=cmd.exe ++StartParams="/c java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon" --StopMode=exe --StopPath=%mypath% --StopImage=cmd.exe ++StopParams="/c java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon 2" --LogPath=%mypath% --LogLevel=Debug --StdOutput=auto --StdError=auto --StopTimeout 10
if %errorlevel% neq 0 (
  echo "install failed"
  exit 1
)
call :startservice
call :testservice
call :mybanner "stopping timeout 10 and wait 60"
%myserv% //SS//TestService
if %errorlevel% neq 0 (
  echo "timeout 10 and wait 60 failed"
  exit 1
)

echo ""
%myserv% //PS//TestService
echo ""

call :deleteservice


rem install service with timeout 10 and 60+60 sec wait
rem the client will take 60 sec to stop the server
echo ""
call :mybanner "install service with timeout 10 and 60+60 sec wait"
echo ""
%myserv% //IS//TestService --Description="Procrun tests" --DisplayName="Test Service" --Install=%myserv% --StartMode=exe --StartPath=%mypath% --StartImage=cmd.exe ++StartParams="/c java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon" --StopMode=exe --StopPath=%mypath% --StopImage=cmd.exe ++StopParams="/c java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon 3" --LogPath=%mypath% --LogLevel=Debug --StdOutput=auto --StdError=auto --StopTimeout 10
if %errorlevel% neq 0 (
  echo "install failed"
  exit 1
)
call :startservice
call :testservice
call :mybanner stopping
%myserv% //SS//TestService
if %errorlevel% equ 0 (
  echo "timeout 10 and wait 60+60 should have failed"
  exit 1
)

rem the service is still running wait for it.
call :waituntilstop
if %errorlevel% neq 0 (
  echo "Not stopped"
  exit 1
)

call :mybanner deleting
%myserv% //DS//TestService
if %errorlevel% equ 9 (
  echo "delete failed, not installed"
) else (
  if %errorlevel% neq 0 (
    echo "delete failed"
    echo "%errorlevel%"
    exit 1
  )
)

rem java service tests, basically use java as exe
echo ""
call :mybanner "install java service with timeout 10 and 60 sec wait"
echo ""
%myserv% //IS//TestService --Description="Procrun tests" --DisplayName="Test Service" --Install=%myserv% --StartMode=Java --StartPath=%mypath% --Classpath=%myjar% --StartClass=org.apache.commons.daemon.ProcrunDaemon --StartMethod=main --StopMode=Java --StopPath=%mypath% --StopClass=org.apache.commons.daemon.ProcrunDaemon --StopMethod=main ++StopParams="2" --LogPath=%mypath% --LogLevel=Debug --StdOutput=auto --StdError=auto --StopTimeout 10
if %errorlevel% neq 0 (
  echo "install failed"
  exit 1
)
call :startservice
call :testservice
call :mybanner stopping
%myserv% //SS//TestService
if %errorlevel% neq 0 (
  echo "java service tests timeout 10 and wait 60 failed"
  exit 1
)
call :deleteservice

call :mybanner "test.bat Done!!!"
goto :EOF

rem functions

:mybanner
echo "*****************************************************"
echo  "%~1"
echo "*****************************************************"
EXIT /B 0

:startservice
call :mybanner starting
%myserv% //ES//TestService
if %errorlevel% neq 0 (
  echo "start failed"
  exit 1
)
EXIT /B 0

:stopservice
call :mybanner stopping
%myserv% //SS//TestService
if %errorlevel% neq 0 (
  echo "stop failed"
  exit 1
)
EXIT /B 0

:deleteservice
call :mybanner deleting
%myserv% //DS//TestService
if %errorlevel% neq 0 (
  echo "delete failed"
  exit 1
)
EXIT /B 0

:testservice
call :mybanner testing
java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon 0
if %errorlevel% neq 0 (
  echo "Java part of the service doesn't answer"
  exit 1
)
EXIT /B 0

:waituntilstop
SETLOCAL EnableDelayedExpansion
for /l %%x in (1, 1, 100) do (
  echo "testing service %%x"
  java -cp %myjar% org.apache.commons.daemon.ProcrunDaemon 1
  > nul find "Exception" client.txt && (
    echo "exception"
    exit /b 0
  )
  timeout /t 10 > nul
)
exit /b 1
