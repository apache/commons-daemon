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

#ifndef __JSVC_LOCKS_H__
#define __JSVC_LOCKS_H__

/*
 * as Cygwin does not support locks, jsvc use NT API to emulate them.
 */
#ifdef OS_CYGWIN

#define F_ULOCK 0               /* Unlock a previously locked region */
#define F_LOCK  1               /* Lock a region for exclusive use */

/*
 * allow a file to be locked
 * @param fildes an open file descriptor
 * @param function a control value that specifies  the action to be taken
 * @param size number of bytes to lock
 * @return Zero on success, a value less than 0 if an error was encountered
 */
int lockf(int fildes, int function, off_t size);

#endif
#endif /* __JSVC_LOCKS_H__ */

