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

#ifdef DSO_DLFCN
#include <dlfcn.h>
#ifdef OS_LINUX
bool ld_library_path_set = false;
#endif /* ifdef OS_LINUX */

/* Initialize all DSO stuff */
bool dso_init(void)
{
    return true;
}

/* Attempt to link a library from a specified filename */
dso_handle dso_link(const char *path)
{
    log_debug("Attemtping to load library %s", path);

    return ((void *)dlopen(path, RTLD_GLOBAL | RTLD_NOW));
}

/* Attempt to unload a library */
bool dso_unlink(dso_handle libr)
{
    if (dlclose(libr) == 0)
        return true;
    else
        return false;
}

/* Get the address for a specifed symbol */
void *dso_symbol(dso_handle hdl, const char *nam)
{
    return dlsym(hdl, nam);
}

/* Return the error message from dlopen */
char *dso_error(void)
{
    return (dlerror());
}

#endif /* ifdef DSO_DLFCN */
