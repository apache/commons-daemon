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

#ifndef __JSVC_JAVA_H__
#define __JSVC_JAVA_H__

#define LOADER "org/apache/commons/daemon/support/DaemonLoader"

char *java_library(arg_data *args, home_data *data);
bool java_init(arg_data *args, home_data *data);
bool java_destroy(void);
bool java_load(arg_data *args);
bool java_signal(void);
bool java_start(void);
bool java_stop(void);
bool java_version(void);
bool java_check(arg_data *args);
bool JVM_destroy(int exit);

#endif /* __JSVC_JAVA_H__ */

