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



#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

HANDLE hGlobalEvent; 
#define EVENT_NAME "ServBatchExampleEvent"


int main(int argc, char **argv)
{
    int mode = 0;
    int i;

    for (i = 0; i < argc; i++)
        fprintf(stdout, "Command line param [%d] = %s\n", i, argv[i]);

    if (argc > 1 && stricmp(argv[1], "stop") == 0)
        mode = 1;
    
    if (mode) {
        fprintf(stdout, "Stopping service\n");
        hGlobalEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, EVENT_NAME);
        if (!hGlobalEvent)
            fprintf(stderr, "Unable to upen the Global Event\n");
        else {
            fprintf(stdout, "Signaling service event\n");
            SetEvent(hGlobalEvent);
        }

    }
    else {
        fprintf(stdout, "Starting service\n");
        hGlobalEvent = CreateEvent(NULL, FALSE, FALSE, EVENT_NAME);
        WaitForSingleObject(hGlobalEvent, INFINITE);
        fprintf(stdout, "Event Signaled\n");
    }
 
    fprintf(stdout, "Service mode %d finished\n", mode);
    return 0;
}
