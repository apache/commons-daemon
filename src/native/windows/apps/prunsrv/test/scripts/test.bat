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
rem to run it you need:
rem 1 - the commons-daemon-*-tests.jar
rem 2 - procrun.exe in a subdirectory (usually something like WIN10_X64_EXE_RELEASE\prunsrv.exe)
rem 3 - use cmd/c test.bat in the command Prompt (cmd)
rem the test are OK once test.bat Done!!! is displayed at the test of the bat script.

SETLOCAL ENABLEEXTENSIONS
SETLOCAL EnableDelayedExpansion
SET mypath=%cd%
FOR /F "tokens=3 USEBACKQ" %%A IN (`ATTRIB /s ..\..\prunsrv.exe`) DO (
  SET myserv=%%A
)
FOR /F "tokens=3 USEBACKQ" %%A IN (`Attrib ..\..\..\..\..\..\..\target\*-tests.jar`) DO (
  SET myjar=%%A
)
ECHO %myserv%
ECHO %myjar%
ECHO %mypath%

echo "cleaning..."
call mybanner stopping
%myserv% //SS//TestService
if %errorlevel% neq 0 (
  echo "Stop service failed"
)
call mybanner deleting
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
call mybanner "install service with notimeout and no wait"
echo ""
%myserv% //IS//TestService --Description="Procrun tests" --DisplayName="Test Service" --Install=%myserv% --StartMode=exe --StartPath=%mypath% --StartImage=cmd.exe ++StartParams="/c java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon" --StopMode=exe --StopPath=%mypath% --StopImage=cmd.exe ++StopParams="/c java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon 1" --LogPath=%mypath% --LogLevel=Debug --StdOutput=auto --StdError=auto
if %errorlevel% neq 0 (
  echo "install failed"
  exit 1
)

call startservice
call testservice
call mybanner stopping
%myserv% //SS//TestService
if %errorlevel% neq 0 (
  echo "No timeout No wait stop failed"
  exit 1
)

rem start the service and make the java part to stop
call startservice
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
call stopservice
call deleteservice

rem install service with timeout 10 and 60 sec wait
echo ""
call mybanner "install service with timeout 10 and 60 sec wait"
echo ""
%myserv% //IS//TestService --Description="Procrun tests" --DisplayName="Test Service" --Install=%myserv% --StartMode=exe --StartPath=%mypath% --StartImage=cmd.exe ++StartParams="/c java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon" --StopMode=exe --StopPath=%mypath% --StopImage=cmd.exe ++StopParams="/c java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon 2" --LogPath=%mypath% --LogLevel=Debug --StdOutput=auto --StdError=auto --StopTimeout 10
if %errorlevel% neq 0 (
  echo "install failed"
  exit 1
)
call startservice
call testservice
call mybanner "stopping timeout 10 and wait 60"
%myserv% //SS//TestService
if %errorlevel% neq 0 (
  echo "timeout 10 and wait 60 failed"
  exit 1
)
call deleteservice

rem install service with timeout 10 and 60+60 sec wait
rem the client will take 60 sec to stop the server
echo ""
call mybanner "install service with timeout 10 and 60+60 sec wait"
echo ""
%myserv% //IS//TestService --Description="Procrun tests" --DisplayName="Test Service" --Install=%myserv% --StartMode=exe --StartPath=%mypath% --StartImage=cmd.exe ++StartParams="/c java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon" --StopMode=exe --StopPath=%mypath% --StopImage=cmd.exe ++StopParams="/c java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon 4" --LogPath=%mypath% --LogLevel=Debug --StdOutput=auto --StdError=auto --StopTimeout 10
if %errorlevel% neq 0 (
  echo "install service with timeout 10 and 60+60 sec wait failed"
  exit 1
)
call startservice
call testservice
call mybanner "stopping service with timeout 10 and 60+60 sec wait"
%myserv% //SS//TestService
if %errorlevel% equ 0 (
  echo "timeout 10 and wait 60+60 should have failed"
  exit 1
)

rem the service is still running wait for it.
rem procrun kills the child processes
call waituntilstop
if %errorlevel% neq 0 (
  echo "Not stopped"
  exit 1
)

call mybanner deleting
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
call mybanner "install java service with timeout 10 and 60 sec wait"
echo ""
%myserv% //IS//TestService --Description="Procrun tests" --DisplayName="Test Service" --Install=%myserv% --StartMode=Java --StartPath=%mypath% --Classpath=%myjar% --StartClass=org.apache.commons.daemon.ProcrunDaemon --StartMethod=main --StopMode=Java --StopPath=%mypath% --StopClass=org.apache.commons.daemon.ProcrunDaemon --StopMethod=main ++StopParams="2" --LogPath=%mypath% --LogLevel=Debug --StdOutput=auto --StdError=auto --StopTimeout 10
if %errorlevel% neq 0 (
  echo "install failed"
  exit 1
)

call startservice
call testservice
call mybanner stopping
%myserv% //SS//TestService
if %errorlevel% neq 0 (
  echo "java service tests timeout 10 and wait 60 failed"
  exit 1
)
call deleteservice

rem install jvm service with notimeout and no wait
echo ""
echo "install jvm service with notimeout and no wait"
echo ""
%myserv% //IS//TestService --Description="Procrun jvm tests" --DisplayName="Test Service" --Install=%myserv% --StartMode=jvm --StartPath=%mypath% --StartClass=org.apache.commons.daemon.ProcrunDaemon --StartMethod=start ++StartParams=procstart --StopMode=jvm --StopClass=org.apache.commons.daemon.ProcrunDaemon --StopMethod=stop ++StopParams 1 --Classpath=%myjar% --LogPath=%mypath% --LogLevel=Debug --StdOutput=auto --StdError=auto
if %errorlevel% neq 0 (
  echo "install failed"
  exit 1
)
call startservice
call testservice
call mybanner stopping
%myserv% //SS//TestService
if %errorlevel% neq 0 (
  echo "jvm service tests notimeout and no wait failed"
  exit 1
)
call deleteservice

rem install jvm service with 10 timeout and 60 wait
echo ""
echo "install jvm service with 10 timeout and 60 wait"
echo ""
%myserv% //IS//TestService --Description="Procrun jvm tests" --DisplayName="Test Service" --Install=%myserv% --StartMode=jvm --StartPath=%mypath% --StartClass=org.apache.commons.daemon.ProcrunDaemon --StartMethod=start ++StartParams=procstart --StopMode=jvm --StopClass=org.apache.commons.daemon.ProcrunDaemon --StopMethod=stop ++StopParams 3 --Classpath=%myjar% --LogPath=%mypath% --LogLevel=Debug --StdOutput=auto --StdError=auto --StopTimeout=10
if %errorlevel% neq 0 (
  echo "install failed"
  exit 1
)
call startservice
call testservice
call mybanner stopping
%myserv% //SS//TestService
if %errorlevel% neq 0 (
  echo "jvm service tests 10 imeout and 60 wait failed"
  exit 1
)
call deleteservice

call mybanner "test.bat Done!!!"
