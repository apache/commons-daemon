/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * as Windows does not support signal, jsvc uses events to emulate them.
 * The supported signal is SIGTERM.
 * The kills.c contains the kill logic.
 */
#ifdef OS_CYGWIN
#include <windows.h>
#include <stdio.h>
static void (*HandleTerm) (void) = NULL;        /* address of the handler routine. */

/*
 * Event handling routine
 */
void v_difthf(LPVOID par)
{
    HANDLE hevint;              /* make a local copy because the parameter is shared! */

    hevint = (HANDLE) par;

    for (;;) {
        if (WaitForSingleObject(hevint, INFINITE) == WAIT_FAILED) {
            /* something have gone wrong. */
            return;                    /* may be something more is needed. */
        }

        /* call the interrupt handler. */
        if (HandleTerm == NULL)
            return;
        HandleTerm();
    }
}

/*
 * set a routine handler for the signal
 * note that it cannot be used to change the signal handler
 */
int SetTerm(void (*func) (void))
{
    char Name[256];
    HANDLE hevint, hthread;
    DWORD ThreadId;
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;

    sprintf(Name, "TERM%ld", GetCurrentProcessId());

    /*
     * event cannot be inherited.
     * the event is reseted to nonsignaled after the waiting thread is released.
     * the start state is resetted.
     */

    /* Initialize the new security descriptor. */
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

    /* Add a NULL descriptor ACL to the security descriptor. */
    SetSecurityDescriptorDacl(&sd, TRUE, (PACL) NULL, FALSE);

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = TRUE;


    /*  It works also with NULL instead &sa!! */
    hevint = CreateEvent(&sa, FALSE, FALSE, Name);

    HandleTerm = func;

    if (hevint == NULL)
        return -1;                     /* failed */

    /* create the thread to wait for event */
    hthread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) v_difthf,
                           (LPVOID) hevint, 0, &ThreadId);
    if (hthread == NULL) {
        /* failed remove the event */
        CloseHandle(hevint);           /* windows will remove it. */
        return -1;
    }

    CloseHandle(hthread);              /* not needed */
    return 0;
}
#else
const char __unused_signals_c[] = __FILE__;
#endif
