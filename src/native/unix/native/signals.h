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
#ifndef __JSVC_SIGNALS_H__
#define __JSVC_SIGNALS_H__

/*
 * as Windows does not support signal, jsvc use event to emulate them.
 * The supported signal is SIGTERM.
 */
#ifdef OS_CYGWIN
/*
 * set a routine handler for the signal
 * note that it cannot be used to change the signal handler
 * @param func The function to call on termination
 * @return Zero on success, a value less than 0 if an error was encountered
 */
int SetTerm(void (*func) (void));

#endif
#endif /* ifndef __JSVC_SIGNALS_H__ */
