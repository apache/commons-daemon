/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _SECURITY_H_INCLUDED_
#define _SECURITY_H_INCLUDED_

__APXBEGIN_DECLS

#define DEFAULT_SERVICE_USER    L"NT AUTHORITY\\LocalService"

#define STAT_SERVICE            L"NT AUTHORITY\\LocalService"
#define STAT_NET_SERVICE        L"NT AUTHORITY\\NetworkService"
#define STAT_SYSTEM             L"LocalSystem"

#define STAT_SYSTEM_WITH_DOMAIN L"NT AUTHORITY\\System"

DWORD
apxSecurityGrantFileAccessToUser(
    LPCWSTR szPath,
    LPCWSTR szUser);

__APXEND_DECLS

#endif /* _SECURITY_H_INCLUDED_ */
