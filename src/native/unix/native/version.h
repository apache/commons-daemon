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

#ifndef __VERSION_H__
#define __VERSION_H__

/**
 * Major API changes that could cause compatibility problems for older
 * programs such as structure size changes.  No binary compatibility is
 * possible across a change in the major version.
 */
#define JSVC_MAJOR_VERSION      1

/**
 * Minor API changes that do not cause binary compatibility problems.
 * Should be reset to 0 when upgrading JSVC_MAJOR_VERSION
 */
#define JSVC_MINOR_VERSION      2

/** patch level */
#define JSVC_PATCH_VERSION      2

/**
 *  This symbol is defined for internal, "development" copies of JSVC.
 *  This symbol will be #undef'd for releases.
 */
#define JSVC_IS_DEV_VERSION     0

/** Properly quote a value as a string in the C preprocessor */
#define JSVC_STRINGIFY(n) JSVC_STRINGIFY_HELPER(n)
/** Helper macro for JSVC_STRINGIFY */
#define JSVC_STRINGIFY_HELPER(n) #n


/** The formatted string of APU's version */
#define JSVC_VERSION_STRING \
     JSVC_STRINGIFY(JSVC_MAJOR_VERSION) "."   \
     JSVC_STRINGIFY(JSVC_MINOR_VERSION) "."   \
     JSVC_STRINGIFY(JSVC_PATCH_VERSION)       \
     JSVC_IS_DEV_STRING

/** Internal: string form of the "is dev" flag */
#if JSVC_IS_DEV_VERSION
#define JSVC_IS_DEV_STRING "-dev"
#else
#define JSVC_IS_DEV_STRING ""
#endif

#endif /* __VERSION_H__ */

