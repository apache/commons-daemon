/* ========================================================================= *
 *                                                                           *
 *                 The Apache Software License,  Version 1.1                 *
 *                                                                           *
 *          Copyright (c) 1999-2003 The Apache Software Foundation.          *
 *                           All rights reserved.                            *
 *                                                                           *
 * ========================================================================= *
 *                                                                           *
 * Redistribution and use in source and binary forms,  with or without modi- *
 * fication, are permitted provided that the following conditions are met:   *
 *                                                                           *
 * 1. Redistributions of source code  must retain the above copyright notice *
 *    notice, this list of conditions and the following disclaimer.          *
 *                                                                           *
 * 2. Redistributions  in binary  form  must  reproduce the  above copyright *
 *    notice,  this list of conditions  and the following  disclaimer in the *
 *    documentation and/or other materials provided with the distribution.   *
 *                                                                           *
 * 3. The end-user documentation  included with the redistribution,  if any, *
 *    must include the following acknowlegement:                             *
 *                                                                           *
 *       "This product includes  software developed  by the Apache  Software *
 *        Foundation <http://www.apache.org/>."                              *
 *                                                                           *
 *    Alternately, this acknowlegement may appear in the software itself, if *
 *    and wherever such third-party acknowlegements normally appear.         *
 *                                                                           *
 * 4. The names  "The  Jakarta  Project",  "WebApp",  and  "Apache  Software *
 *    Foundation"  must not be used  to endorse or promote  products derived *
 *    from this  software without  prior  written  permission.  For  written *
 *    permission, please contact <apache@apache.org>.                        *
 *                                                                           *
 * 5. Products derived from this software may not be called "Apache" nor may *
 *    "Apache" appear in their names without prior written permission of the *
 *    Apache Software Foundation.                                            *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES *
 * INCLUDING, BUT NOT LIMITED TO,  THE IMPLIED WARRANTIES OF MERCHANTABILITY *
 * AND FITNESS FOR  A PARTICULAR PURPOSE  ARE DISCLAIMED.  IN NO EVENT SHALL *
 * THE APACHE  SOFTWARE  FOUNDATION OR  ITS CONTRIBUTORS  BE LIABLE  FOR ANY *
 * DIRECT,  INDIRECT,   INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR  CONSEQUENTIAL *
 * DAMAGES (INCLUDING,  BUT NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE GOODS *
 * OR SERVICES;  LOSS OF USE,  DATA,  OR PROFITS;  OR BUSINESS INTERRUPTION) *
 * HOWEVER CAUSED AND  ON ANY  THEORY  OF  LIABILITY,  WHETHER IN  CONTRACT, *
 * STRICT LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN *
 * ANY  WAY  OUT OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF  ADVISED  OF THE *
 * POSSIBILITY OF SUCH DAMAGE.                                               *
 *                                                                           *
 * ========================================================================= *
 *                                                                           *
 * This software  consists of voluntary  contributions made  by many indivi- *
 * duals on behalf of the  Apache Software Foundation.  For more information *
 * on the Apache Software Foundation, please see <http://www.apache.org/>.   *
 *                                                                           *
 * ========================================================================= */

/* @version $Id: signals.c,v 1.1 2003/09/04 23:28:20 yoavs Exp $ */

/*
 * as Windows does not support signal, jsvc use event to emulate them.
 * The supported signal is SIGTERM.
 * The kills.c contains the kill logic.
 */
#ifdef OS_CYGWIN
#include <windows.h>
#include <stdio.h>
static void (*HandleTerm)()=NULL; /* address of the handler routine. */

/*
 * Event handling routine
 */
void v_difthf(LPVOID par)
{
HANDLE hevint; /* make a local copy because the parameter is shared! */

  hevint = (HANDLE) par;

  for (;;) {
    if (WaitForSingleObject(hevint,INFINITE) == WAIT_FAILED) {
      /* something have gone wrong. */
      return; /* may be something more is needed. */
      }

    /* call the interrupt handler. */
    if (HandleTerm==NULL) return;
    HandleTerm();
    }
}

/*
 * set a routine handler for the signal
 * note that it cannot be used to change the signal handler
 */
int SetTerm(void (*func)())
{
char Name[256];
HANDLE hevint, hthread;
DWORD ThreadId; 
SECURITY_ATTRIBUTES sa;
SECURITY_DESCRIPTOR sd;

  sprintf(Name,"TERM%ld",GetCurrentProcessId());

  /*
   * event cannot be inherited.
   * the event is reseted to nonsignaled after the waiting thread is released.
   * the start state is resetted.
   */

  /* Initialize the new security descriptor. */
  InitializeSecurityDescriptor (&sd, SECURITY_DESCRIPTOR_REVISION);

  /* Add a NULL descriptor ACL to the security descriptor. */
  SetSecurityDescriptorDacl (&sd, TRUE, (PACL)NULL, FALSE);

  sa.nLength = sizeof(sa);
  sa.lpSecurityDescriptor = &sd;
  sa.bInheritHandle = TRUE;


  /*  It works also with NULL instead &sa!! */
  hevint = CreateEvent(&sa,FALSE, FALSE,Name);

  HandleTerm = (int (*)()) func;

  if (hevint == NULL) return(-1); /* failed */

  /* create the thread to wait for event */
  hthread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE) v_difthf,
                         (LPVOID) hevint, 0, &ThreadId);
  if (hthread == NULL) {
    /* failed remove the event */
    CloseHandle(hevint); /* windows will remove it. */
    return(-1);
    }

  CloseHandle(hthread); /* not needed */
  return(0);
}
#endif
