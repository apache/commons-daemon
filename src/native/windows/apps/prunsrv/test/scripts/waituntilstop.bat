@ECHO OFF
rem Licensed to the Apache Software Foundation (ASF) under one or more
rem contributor license agreements.  See the NOTICE file distributed with
rem this work for additional information regarding copyright ownership.
rem The ASF licenses this file to You under the Apache License, Version 2.0
rem (the "License"); you may not use this file except in compliance with
rem the License.  You may obtain a copy of the License at
rem
rem     https://www.apache.org/licenses/LICENSE-2.0
rem
rem Unless required by applicable law or agreed to in writing, software
rem distributed under the License is distributed on an "AS IS" BASIS,
rem WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
rem See the License for the specific language governing permissions and
rem limitations under the License.

SET result=0
SETLOCAL ENABLEDELAYEDEXPANSION
for /l %%x in (1, 1, 100) do (
  echo "testing service %%x"
  java -cp %myjar% org.apache.commons.daemon.ProcrunDaemon 1 > nul 2>&1
  findstr Exception client.txt > nul 2>&1
  IF !ERRORLEVEL! EQU 0 (
    echo "Found"
    SET result=1
    goto :EndLoop
  ) ELSE (
    echo "NOT Found"
  )
  timeout /t 10 > nul
)

:EndLoop
ENDLOCAL
echo "Done result: %result%"
IF %result% EQU 0 (
  exit /b 0
)
exit /b 1
