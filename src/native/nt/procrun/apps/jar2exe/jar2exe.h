/* Copyright 2000-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
/* ====================================================================
 * jar2exe -- convert .jar file to WIN executable.
 * Contributed by Mladen Turk <mturk@apache.org>
 * 05 Aug 2003
 * ==================================================================== 
 */

#ifndef _JAR2EXE_H
#define _JAR2EXE_H

#undef  PRG_VERSION
#define PRG_VERSION    "1.0.0.0" 

#define ZIPOFF_CDIR_CLEN            20
#define ZIPOFF_CDIR_DISKNO          4
#define ZIPOFF_CDIR_FINALDISKNO     6
#define ZIPOFF_CDIR_ENTRIES         8
#define ZIPOFF_CDIR_TOTENTRIES      10
#define ZIPOFF_CDIR_SIZE            12
#define ZIPOFF_CDIR_OFFSET          16

#define ZIPOFF_FH_VERSION           4
#define ZIPOFF_FH_NEEDVERSION       6
#define ZIPOFF_FH_GPBF              8
#define ZIPOFF_FH_CM                10
#define ZIPOFF_FH_MTIME             12
#define ZIPOFF_FH_MDATE             14
#define ZIPOFF_FH_CRC32             16
#define ZIPOFF_FH_CSIZE             20
#define ZIPOFF_FH_USIZE             24
#define ZIPOFF_FH_FNLEN             28
#define ZIPOFF_FH_XFLEN             30
#define ZIPOFF_FH_FCLEN             32
#define ZIPOFF_FH_DISKNO            34
#define ZIPOFF_FH_IFATTR            36
#define ZIPOFF_FH_EFATTR            38
#define ZIPOFF_FH_LFHOFFSET         42
#define ZIPLEN_FH                   46

#define READ_32_BITS(base, offset)                \
        ((((unsigned char)(base)[offset + 0])) |       \
         (((unsigned char)(base)[offset + 1]) << 8)  | \
         (((unsigned char)(base)[offset + 2]) << 16) | \
         (((unsigned char)(base)[offset + 3]) << 24))
        
#define READ_16_BITS(base, offset)                \
        ((((unsigned char)(base)[offset])) |         \
         (((unsigned char)(base)[offset + 1]) << 8)) \

#define WRITE_32_BITS(base, offset, value) { \
    (base)[offset + 0] = (unsigned char)(value & 0xff);               \
    (base)[offset + 1] = (unsigned char)((value & 0xff00) >> 8);      \
    (base)[offset + 2] = (unsigned char)((value & 0xff0000) >> 16);   \
    (base)[offset + 3] = (unsigned char)((value & 0xff000000) >> 24); \
}

#define WRITE_16_BITS(base, offset, value) { \
    (base)[offset + 0] = (unsigned char)(value & 0xff);               \
    (base)[offset + 1] = (unsigned char)((value & 0xff00) >> 8);      \
}

/* shuld be faster then memcmp */
#define CHECK_4_MAGIC(base, magic) \
    ( ((base)[0] == (magic)[0]) && \
      ((base)[1] == (magic)[1]) && \
      ((base)[2] == (magic)[2]) && \
      ((base)[3] == (magic)[3]) )


#endif /* _JAR2EXE_H */

