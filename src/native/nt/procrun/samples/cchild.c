/* Copyright 2000-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
/* Simple console child */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <conio.h>
#include <time.h>
#include <process.h>    /* _beginthread, _endthread */
#include <fcntl.h>
#include <io.h>

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define STDO_MESSAGE "Simple STDOUT_FILENO message\n"
#define STDE_MESSAGE "Simple STDERR_FILENO message\n"

#define NLOOPS          5
#define NTHREADS       10

static int do_echo = 0;
BOOL WINAPI ControlHandler ( DWORD dwCtrlType )
{
   printf("\nCTRL Event %d ", dwCtrlType);
   switch (dwCtrlType) {
   case CTRL_BREAK_EVENT:
      printf("CTRL+BREAK\n");
      exit(1);
      return TRUE;
   case CTRL_C_EVENT:
      printf("CTRL+C");
      return TRUE;
      break;

   }
   putch('\n');
   return FALSE;
}

unsigned __stdcall threadfunc(void *args)
{
    int i, p , r;
    p = (int)(size_t)args;
    printf("Created thread %d %04x\n", p, GetCurrentThreadId());
    srand((unsigned)time(NULL) + p);
    for (i = 0; i < NLOOPS; i++) {
        r = rand() % 1000;
        Sleep(r);
        printf("Thread %d message %d\n", p, i);
    }
    printf("Quiting thread %d %04x\n", p, GetCurrentThreadId());
    _endthreadex(0);
    return 0;
}


int main(int argc, char *argv[])
{
    int i, conio = 0, threads = 0;
    char buf[256];
    HANDLE htrd[NTHREADS];

    OutputDebugString("cchild starting");
    fprintf(stdout, "cchild starting %s\n", argv[0]);
    fflush(stdout);
    SetConsoleCtrlHandler(ControlHandler, TRUE);
    Sleep(1000);
    
    for (i = 0; i < __argc; i++) {
        fprintf(stdout, "argv[%d] %s\n", i, __argv[i]);
        fflush(stdout);
        if (strcmp(__argv[i], "--") == 0)
            do_echo = 1;
        else if (strcmp(__argv[i], "-c") == 0)
            conio = 1;
        else if (strcmp(__argv[i], "-t") == 0)
            threads = 1;
    }
    fflush(stdout);
    fprintf(stderr, "Simple stderr message\n");
    fflush(stderr);
    write(STDOUT_FILENO, STDO_MESSAGE, sizeof(STDO_MESSAGE) - 1);
    write(STDERR_FILENO, STDE_MESSAGE, sizeof(STDE_MESSAGE) - 1);
    
    if (conio) {
        cputs("Type 'Y' when finished typing keys...");
        do {
            i = getch();
            i = toupper(i);
        } while (i != 'Y');
        putch('\n');
    }
    if (do_echo) {
        cputs("Going to echo loop...\n");
        while ((i = read(STDIN_FILENO, buf, 256)) > 0) {
            buf[i] = '\0';
            fputs(buf, stdout);
            if (strcmp(buf, "quit\n") == 0)
                break;
        }
    }
    if (threads) {
        for (i = 0; i < NTHREADS; i++) {
            unsigned id;
            htrd[i] = (HANDLE)_beginthreadex(NULL, 0, threadfunc, (void *)(size_t)i, 0, &id);
        }
        WaitForMultipleObjects(10, htrd, TRUE, INFINITE);
    }

    fprintf(stdout, "cchild finishing\n");
    fprintf(stdout, "cchild finished\n");
    Sleep(1000);
    OutputDebugString("cchild Ended");
    return 0;
}

