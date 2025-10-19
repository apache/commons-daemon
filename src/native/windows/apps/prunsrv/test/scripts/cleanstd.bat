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

rem the files:
rem rw-r--r-- 1 Administrator None      0 Apr 28 00:55 testservice-stderr.2025-04-28.log
rem rw-r--r-- 1 Administrator None      0 Apr 28 00:55 testservice-stdout.2025-04-28.log
rem needs to be removed between tests.

SET "full_date=%date:~4,10%"
SET "month=%full_date:~0,2%"
SET "day=%full_date:~3,2%"
SET "year=%full_date:~6,4%"

del "testservice-stdout.%year%-%month%-%day%.log" 2>nul

del "testservice-stderr.%year%-%month%-%day%.log" 2>nul
