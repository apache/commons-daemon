/*
 *  Copyright 2002-2004 The Apache Software Foundation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/* ====================================================================
 * testchild.c
 *
 * Contributed by Mladen Turk <mturk@mappingsoft.com>
 *
 * 05 Aug 2002
 * ==================================================================== 
 */

#ifndef STRICT
#define STRICT
#endif
#ifndef OEMRESOURCE
#define OEMRESOURCE
#endif

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <time.h>
#include <stdarg.h>

BOOL WINAPI ControlHandler ( DWORD dwCtrlType )
{
   switch (dwCtrlType) {
   case CTRL_BREAK_EVENT:
      printf("got CTRL+BREAK Event\n");
      exit(1);
      return TRUE;
   case CTRL_C_EVENT:
      printf("got CTRL+C Event\n");
      exit(2);
      return TRUE;
      break;

   }
   return FALSE;
}

//int main(int argc, char **argv, char **envp)
void __cdecl main(int argc, char **argv)

{
    int i;
    char **envp = _environ;
    SetConsoleCtrlHandler(ControlHandler, TRUE);

    printf("Testing child\n");
    printf("ARGV:\n");
    for (i = 0; i < argc; i++)
        printf("\t%d %s\n", i, argv[i]);
    printf("ENVP:\n");
    for (i = 0; envp[i]; i++)
        printf("\t%d %s\n", i, envp[i]);
    
    printf("\nChild done...\n");
    fflush(stdout);
    for (i = 0; i < 1000; i++)
        Sleep(100);
}
