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

#ifndef __JSVC_H__
#define __JSVC_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Definitions for booleans */
#ifdef OS_DARWIN
#include <stdbool.h>
#else
typedef enum {
    false,
    true
} bool;
#endif

#include "version.h"
#include "debug.h"
#include "arguments.h"
#include "home.h"
#include "location.h"
#include "replace.h"
#include "dso.h"
#include "java.h"
#include "help.h"
#include "signals.h"
#include "locks.h"

int  main(int argc, char *argv[]);
void main_reload(void);
void main_shutdown(void);

#endif /* ifndef __JSVC_H__ */

