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

#ifndef __JSVC_REPLACE_H__
#define __JSVC_REPLACE_H__

/**
 * Replace all occurrences of mch in old with the new string rpl, and
 * stores the result in new, provided that its length (specified in len)
 * is enough.
 *
 * @param new The buffer where the result of the replace operation will be
 *            stored into.
 * @param len The length of the previous buffer.
 * @param old The string where occurrences of mtch must be searched.
 * @param mch The characters to match in old (and to be replaced)
 * @param rpl The characters that will be replaced in place of mch.
 * @return Zero on success, a value less than 0 if an error was encountered
 *         or a value greater than zero (indicating the required storage size
 *         for new) if the buffer was too short to hold the new string.
 */
int replace(char *new, int len, char *old, char *mch, char *rpl);

#endif /* ifndef __JSVC_REPLACE_H__ */

