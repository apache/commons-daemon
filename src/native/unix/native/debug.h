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

#ifndef __JSVC_DEBUG_H__
#define __JSVC_DEBUG_H__

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Wether debugging is enabled or not.
 */
extern bool log_debug_flag;

/* Wether SYSLOG logging (for stderr) is enable or not. */
extern bool log_stderr_syslog_flag;

/* Wether SYSLOG logging (for stdout) is enable or not. */
extern bool log_stdout_syslog_flag;

/**
 * The name of the jsvc binary.
 */
extern char *log_prog;

/**
 * Helper macro to avoid NPEs in printf.
 */
#define PRINT_NULL(x) ((x) == NULL ? "null" : (x))

/**
 * Dump a debug message.
 *
 * @param fmt The printf style message format.
 * @param ... Any optional parameter for the message.
 */
void log_debug(const char *fmt, ...);

/**
 * Dump an error message.
 *
 * @param fmt The printf style message format.
 * @param ... Any optional parameter for the message.
 */
void log_error(const char *fmt, ...);

#ifdef __cplusplus
}
#endif
#endif                          /* ifndef __JSVC_DEBUG_H__ */

