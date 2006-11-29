/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/* @version $Id$ */

/*
 * as Windows does not support signal, jsvc uses events to emulate them.
 * The supported signal is SIGTERM.
 * signals.c contains the signal handler logic.
 */
#include <windows.h>
#include <stdio.h>

/*
 * Send a clean termination signal to a process
 * it is like kill(pid,SIGTERM);
 */
int TermPid(long pid)
{
char Name[256];
HANDLE hevint;
BOOL  rc;

  sprintf(Name,"TERM%ld",pid);

  hevint = OpenEvent(EVENT_MODIFY_STATE,FALSE,Name);
  if (hevint == NULL) return(-1); /* failed */

  rc = SetEvent(hevint);
  CloseHandle(hevint);
  
  if (rc) return(0);
  return(-1);
}
