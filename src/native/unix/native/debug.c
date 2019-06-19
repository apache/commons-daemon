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

#include "jsvc.h"
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

/* Wether debug is enabled or not */
bool log_debug_flag = false;

/* Wether SYSLOG logging (for stderr) is enable or not. */
bool log_stderr_syslog_flag = false;

/* Wether SYSLOG logging (for stdout) is enable or not. */
bool log_stdout_syslog_flag = false;

/* The name of the jsvc binary. */
char *log_prog = "jsvc";

/* Dump a debug trace message to stderr */
void log_debug(const char *fmt, ...)
{
    va_list ap;
    time_t now;
    struct tm *nowtm;
    char buff[80];

    if (log_debug_flag == false)
        return;
    if (fmt == NULL)
        return;

    now = time(NULL);
    nowtm = localtime(&now);
    strftime(buff, sizeof(buff), "%Y-%m-%d %T", nowtm);
    va_start(ap, fmt);
    if (log_stderr_syslog_flag)
        fprintf(stderr, "%s %d %s debug: ", buff, getpid(), log_prog);
#if defined(DEBUG) || defined(_DEBUG)
    else
        fprintf(stderr, "[debug] %s %d ", buff, getpid());
#endif
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    fflush(stderr);
    va_end(ap);
}

/* Dump an error message to stderr */
void log_error(const char *fmt, ...)
{
    va_list ap;
    time_t now;
    struct tm *nowtm;
    char buff[80];

    if (fmt == NULL)
        return;

    va_start(ap, fmt);
    if (log_stderr_syslog_flag) {
        now = time(NULL);
        nowtm = localtime(&now);
        strftime(buff, sizeof(buff), "%Y-%m-%d %T", nowtm);
        fprintf(stderr, "%s %d %s error: ", buff, getpid(), log_prog);
    }
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    fflush(stderr);
    va_end(ap);
}
