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
 * as Cygwin does not support lockf, jsvc uses fcntl to emulate it.
 */
#ifdef OS_CYGWIN
#include "jsvc.h"
#include <sys/fcntl.h>

/*
 * File locking routine
 */
int lockf(int fildes, int function, off_t size)
{
    struct flock buf;

    switch (function) {
        case F_LOCK:
            buf.l_type = F_WRLCK;
            break;
        case F_ULOCK:
            buf.l_type = F_UNLCK;
            break;
        default:
            return -1;
    }
    buf.l_whence = 0;
    buf.l_start = 0;
    buf.l_len = size;

    return fcntl(fildes, F_SETLK, &buf);
}
#else
const char __unused_locks_c[] = __FILE__;
#endif
