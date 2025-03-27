@ECHO OFF
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
if %errorlevel% neq 0 (
  echo "delete failed"
)
echo "cleaned"


rem install service with notimeout and no wait
echo ""
echo "install service with notimeout and no wait"
ECHO %myserv%
ECHO %myjar%
ECHO %mypath%
echo ""
%myserv% //IS//TestService --Description="JFC tests" --DisplayName="Test Service" --Install=%myserv% --StartMode=exe --StartPath=%mypath% --StartImage=cmd.exe ++StartParams="/c java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon" --StopMode=exe --StopPath=%mypath% --StopImage=cmd.exe ++StopParams="/c java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon 1" --LogPath=%mypath% --LogLevel=Debug --StdOutput=auto --StdError=auto
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
echo "install service with timeout 10 and 60 sec wait"
ECHO %myserv%
ECHO %myjar%
ECHO %mypath%
echo ""
%myserv% //IS//TestService --Description="JFC tests" --DisplayName="Test Service" --Install=%myserv% --StartMode=exe --StartPath=%mypath% --StartImage=cmd.exe ++StartParams="/c java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon" --StopMode=exe --StopPath=%mypath% --StopImage=cmd.exe ++StopParams="/c java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon 2" --LogPath=%mypath% --LogLevel=Debug --StdOutput=auto --StdError=auto --StopTimeout 10
if %errorlevel% neq 0 (
  echo "install failed"
  exit 1
)
call :startservice
call :testservice
%myserv% //SS//TestService
if %errorlevel% neq 0 (
  echo "timeout 10 and wait 60 failed"
  exit 1
)
call :deleteservice


rem install service with timeout 10 and 60+60 sec wait
rem the client will take 60 sec to stop the server
echo ""
echo "install service with timeout 10 and 60+60 sec wait"
ECHO %myserv%
ECHO %myjar%
ECHO %mypath%
echo ""
%myserv% //IS//TestService --Description="JFC tests" --DisplayName="Test Service" --Install=%myserv% --StartMode=exe --StartPath=%mypath% --StartImage=cmd.exe ++StartParams="/c java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon" --StopMode=exe --StopPath=%mypath% --StopImage=cmd.exe ++StopParams="/c java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon 3" --LogPath=%mypath% --LogLevel=Debug --StdOutput=auto --StdError=auto --StopTimeout 10
if %errorlevel% neq 0 (
  echo "install failed"
  exit 1
)
call :startservice
call :testservice
call :mybanner stopping
%myserv% //SS//TestService
if %errorlevel% equ 0 (
  echo "timeout 10 and wait 60 should have failed"
  exit 1
)
call :deleteservice

call :mybanner Done!!!
exit 0

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
java  -cp %myjar% org.apache.commons.daemon.ProcrunDaemon 0
if %errorlevel% neq 0 (
  echo "Java part of the srvice doesn't answer"
  exit 1
)
EXIT /B 0
