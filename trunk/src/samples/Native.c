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
 * Native routine to core JVM
 */
#include <jni.h>

#ifndef _Included_Native
#define _Included_Native
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     Native
 * Method:    toto
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_SimpleDaemon_toto
  (JNIEnv *env, jclass class) {
    int i;
    i = -1;
    memcpy(&i, &i, i);
    memset(&i, ' ', 1024);
}

#ifdef __cplusplus
}
#endif
#endif
