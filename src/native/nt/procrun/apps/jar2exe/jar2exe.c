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

/* Force the JNI vprintf functions */
#define _DEBUG_JNI  1
#include "apxwin.h"
#include <imagehlp.h>
#include "jar2exe.h"
#pragma comment( lib, "imagehlp.lib" )


#ifdef _DEBUG
#define LOG_MARK        _verbose, __FILE__, __LINE__
#else
#define LOG_MARK        _verbose, NULL, 0
#endif

#define EXE_SUFFIX      ".EXE"
#define EXE_SUFFIX_SZ   (sizeof(".EXE") - 1)
#define JAR_LINT        "jar2exe Copyright (c) 2000-2003 The Apache Software Foundation."

#define EMBED_SIZE      2048
#define EMBED_MAX       2044

typedef struct {
    char    s_signature[16];
    char    s_class[EMBED_SIZE];
    UINT32  s_flags[4];
    char    e_signature[16];
} st_config;

static char         _progname[MAX_PATH+1];
static char         _progpath[MAX_PATH+1];
static HANDLE       _hstub   = NULL;
static BOOL         _verbose = FALSE;
static BOOL         _makegui = FALSE;
static const char   *_lint   = JAR_LINT;
static st_config    *_config = NULL;

/* zip file header magic */
static BYTE zip_fh_magic[4] = { '\120', '\113', '\001', '\002'};
/* end of central directory signature */
static BYTE zip_ec_magic[4] = { '\120', '\113', '\005', '\006'}; 
/* lower case configuration block signature 
 * to enable binary searching
 */
static BYTE c_signature[] = {
    's', 't', 'a', 'r', 't', 'u', 's', 'e', 'r', 's', 'b', 'l', 'o', 'c', 'k', '\0' 
};

/* space in exe stub for user configuration */
static BYTE conf_in_exe[EMBED_SIZE + 48] = {
    'S', 'T', 'A', 'R', 'T', 'U', 'S', 'E', 'R', 'S', 'B', 'L', 'O', 'C', 'K', '\0',

    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',

    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    'E', 'N', 'D', 'O', 'F', 'U', 'S', 'E', 'R', 'S', 'B', 'L', 'O', 'C', 'K', '\0',
};

static LPCSTR _usage = JAR_LINT "\n"                                     \
    "Usage:\n"                                                           \
    "  jar2exe [options] <output.exe> <class> <embed.jar> [params ...]\n" \
    "  options:\n"                                          \
    "  -d\t\tdisplay error messages\n"                      \
    "  -g\t\tmake non-console application\n"                \
    "  -m<nn>\t\tInitial memory pool (nn MB)\n"             \
    "  -x<nn>\t\tMaximum memory size (nn MB)\n"             \
    "  -s<nn>\t\tThread stack size (nn Bytes)\n"            \
    "  output.exe\toutput executable\n"                     \
    "  class\t\tmain class name\n"                          \
    "  embed.jar\tjar file to embed in stub\n"              \
    "  params\t\tJava VM parameters\n";

/* Fix the ZIP central directory 
 * Find the central dir position and
 * add the stub length offset to each enty.
 */
static int fixjar(size_t slen)
{
    HANDLE hmap;
    BYTE *map, *cd, *hd, *mp;
    size_t len, off, cdir, cpos = 0;
    int i, nent, rc = 0;

    len = SetFilePointer(_hstub, 0, NULL, FILE_END);
    if ((hmap = CreateFileMapping(_hstub, NULL, 
                                  PAGE_READWRITE, 0, 0, NULL)) == NULL) {
        apxDisplayError(LOG_MARK, "%s unable to map the stub file\n",
                        _progname);
        return -1;
    }
    if ((map = MapViewOfFile(hmap, FILE_MAP_ALL_ACCESS, 0, 0, 0)) == NULL) {
        apxDisplayError(LOG_MARK, "%s unable to map the view of file\n",
                        _progname);
        CloseHandle(hmap);
    }
    off = len - ZIPOFF_CDIR_CLEN;
    cd  = map + off;
    mp  = CHECK_4_MAGIC(cd, zip_ec_magic) ? cd : NULL; 
    while ((len - off) < (64 * 1024)) { 
        i = 0;
        if (!mp) {
            off -= ZIPOFF_CDIR_CLEN;
            cd = map + off;

            if (!CHECK_4_MAGIC(cd, zip_ec_magic)) {
                for (; i < ZIPOFF_CDIR_CLEN; i++) {
                    if (CHECK_4_MAGIC((cd + i), zip_ec_magic)) {
                        mp = cd + i;
                        break;
                    }
                }
            }
            else
                mp = cd;
        }         

        if (mp) { 
            cpos = READ_32_BITS(mp, ZIPOFF_CDIR_OFFSET);
            nent = READ_16_BITS(mp, ZIPOFF_CDIR_TOTENTRIES);
            cdir = off + i;
            break;
        }

    }
    if (cpos) {
        hd = map + cpos + slen;
        for (i = 0; i < nent; i++) {
            size_t xl, cl, fl, sp;
            if (!CHECK_4_MAGIC(hd, zip_fh_magic)) {
                rc = -1;     
                apxDisplayError(LOG_MARK, "Wrong header at %d\n", hd - map);
                goto cleanup;
            }
            sp = READ_32_BITS(hd, ZIPOFF_FH_LFHOFFSET);
            fl = READ_16_BITS(hd, ZIPOFF_FH_FNLEN);
            xl = READ_16_BITS(hd, ZIPOFF_FH_XFLEN);
            cl = READ_16_BITS(hd, ZIPOFF_FH_FCLEN);
            if (i == 0 && sp != 0) {
                apxDisplayError(LOG_MARK, "The first entry is not zero\n");
                rc = -1;     
                goto cleanup;
            }
            WRITE_32_BITS(hd, ZIPOFF_FH_LFHOFFSET, sp + slen);            
            hd += (fl + xl + cl + ZIPLEN_FH);
        }
        /* repos to the end of central dir */
        mp = map + cdir;
        WRITE_32_BITS(mp, ZIPOFF_CDIR_OFFSET, cpos + slen);            
    }
cleanup:
    UnmapViewOfFile(map);
    CloseHandle(hmap);
    return rc;

}

/* Merge the stub with jar file
 * Fix the userblock bounded code.
 */
static int merge(LPCSTR jar, size_t slen)
{
    HANDLE hjar, hmap;
    char buff[4096];
    BYTE bmatch[16];

    DWORD rd, wr;
    BYTE *map, *ss;
    int i;
    if ((hmap = CreateFileMapping(_hstub, NULL, 
                                  PAGE_READWRITE, 0, 0, NULL)) == NULL) {
        apxDisplayError(LOG_MARK, "%s unable to map the stub file\n",
                        _progname);
        return -1;
    }
    if ((hjar = CreateFile(jar, GENERIC_READ, 0, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
        apxDisplayError(LOG_MARK, "%s unable to open jar file : %s\n",
                        _progname, jar);
        return -1;
    }
    
    if ((map = MapViewOfFile(hmap, FILE_MAP_ALL_ACCESS, 0, 0, 0)) == NULL) {
        apxDisplayError(LOG_MARK, "%s unable to map the view of file\n",
                        _progname);
        CloseHandle(hmap);
        return -1;
    }
    /* convert to upper case */
    for (i = 0; i < 15; i++)
        bmatch[i] = c_signature[i] - 32;
    bmatch[15] = 0;
    if ((ss = ApcMemSearch(map, bmatch, 16, slen)) != NULL) {
        st_config cfg;
        AplCopyMemory(&cfg, ss, sizeof(st_config));
        AplCopyMemory(ss, _config, sizeof(st_config));
    }
    UnmapViewOfFile(map);
    CloseHandle(hmap);
    SetFilePointer(_hstub, 0, NULL, FILE_END);
    while (ReadFile(hjar, buff, 4096, &rd, NULL)) {
        if (rd)
            WriteFile(_hstub, buff, rd, &wr, NULL);
        else
            break;
        if (rd != wr) {
            apxDisplayError(LOG_MARK, "%s mismatch read/write\n(Readed : %d Written %d)\n",
                            _progname, rd, wr);
            CloseHandle(hjar);
            return -1;
        }
    }
    CloseHandle(hjar);
    return 0;
}

static int
jar2exe(int argc, char *argv[])
{
    int i = 1, l;
    BY_HANDLE_FILE_INFORMATION stub_inf;
    char *p;
    LOADED_IMAGE li;

    while (argc > 1 && *argv[1] == '-') {
        p = argv[1];
        /* Check if verbose mode is on */
        while (*p) {
            if (*p == 'm') {
                _config->s_flags[0] = atoi(p + 1);
                break;
            }
            else if (*p == 'x') {
                _config->s_flags[1] = atoi(p + 1);
                break;
            }
            else if (*p == 's') {
                _config->s_flags[2] = atoi(p + 1);
                break;
            }
            else if (*p == 'd')
                _verbose = TRUE;
            else if (*p == 'g')
                _makegui = TRUE;
            ++p;
        }
        --argc;
        ++argv;
    }
    if (argc < 4) {
        SetLastError(ERROR_SUCCESS);
        apxDisplayError(TRUE, NULL, 0, _usage);
        return -1;
    }
    AplZeroMemory(_config->s_class, EMBED_SIZE);
    lstrcpyA(_config->s_class, argv[2]);
    l = lstrlenA(_config->s_class);
    for (i = 0; i < l; i++) {
        if (_config->s_class[i] == '.')
            _config->s_class[i] = '/';
    }
    p = (char *)(&(_config->s_class[l+1]));
    *p++ = '\0';
    if (argc > 4) {
        --p;
        for (i = 4; i < argc; i++) {
            l += lstrlenA(argv[i]);
            if (l > EMBED_MAX) {
                apxDisplayError(LOG_MARK, "%s args are too long (max %d are allowed)\n",
                                _progname, EMBED_MAX);
                return -1;
            }
            lstrcpyA(p, argv[i]);
            p += lstrlenA(argv[i]);
            *p++ = '\0';
        }
    }
    *p++ = '\0';
    DeleteFile(argv[1]);
    CopyFile(_progpath, argv[1], TRUE);
    if ((_hstub = CreateFile(argv[1], GENERIC_READ | GENERIC_WRITE, 0, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
        apxDisplayError(LOG_MARK, "%s unable to open stub %s\n",
                        _progname, argv[1]);
        DeleteFile(argv[1]);
        return -1;
    }
    /* Get original stub size */
    GetFileInformationByHandle(_hstub, &stub_inf);
    
    if (merge(argv[3], stub_inf.nFileSizeLow))
        goto cleanup;
    if (fixjar(stub_inf.nFileSizeLow))
        goto cleanup;
    CloseHandle(_hstub);
    if (_makegui) {
        /* Change a portable executable (PE) image Subsytem to GUI.
         * It will also calculate a new Checksum.
         */
        if (MapAndLoad(argv[1], NULL, &li, FALSE, FALSE)) {
            if (li.FileHeader)
                li.FileHeader->OptionalHeader.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
            UnMapAndLoad(&li);
        }
        else
            apxDisplayError(LOG_MARK, "%s MapAndLoad %s\n",
                            _progname, argv[1]);
    }
    return 0;
cleanup:
    CloseHandle(_hstub);
    DeleteFile(argv[1]);
    return -1;
}

/* Run the embedded jar file
 *
 */
int run(int argc, char *argv[])
{
    HANDLE hPool, hJava;
    DWORD  rv = 0;
    apxHandleManagerInitialize();
    hPool = apxPoolCreate(NULL, 0);
    /* Use default JVM */
    hJava = apxCreateJava(hPool, NULL);
    if (!IS_INVALID_HANDLE(hJava)) {
        LPSTR lpCmd;
        if (!apxJavaInitialize(hJava, _progpath,
                              _config->s_class + 
                              strlen(_config->s_class) + 1,
                              _config->s_flags[0],
                              _config->s_flags[1],
                              _config->s_flags[2])) {            
            rv = 1;
            goto cleanup;
        }
        lpCmd = apxArrayToMultiSzA(hPool, argc - 1, &argv[1]);
        if (!apxJavaLoadMainClass(hJava, _config->s_class, NULL, lpCmd)) {
            rv = 2;
            goto cleanup;
        }
        if (!apxJavaStart(hJava)) {
            rv = 3;
        }
        else
            apxJavaWait(hJava, INFINITE, FALSE);
cleanup:
        apxCloseHandle(hJava);
    }
    else {
        rv = 4;
    }

    apxHandleManagerDestroy();
    ExitProcess(rv);
    return 0;
}

int main(int argc, char *argv[])
{
    char *p;
    char path[MAX_PATH+1];
    GetModuleFileName(GetModuleHandle(NULL), path, MAX_PATH);
    lstrcpyA(_progpath, path);
    
    _config = (st_config *)conf_in_exe;
    if (lstrcmpiA(_config->s_signature, c_signature)) {
        apxDisplayError(LOG_MARK, "%s\nWrong user block signature\n", _lint);
        ExitProcess(1);
    }
    /* remove the path and extension from module name */
    if ((p = AplRindexA(path, '\\')))
        *p++ = '\0';
    else
        p = &path[0];
    lstrcpyA(_progname, p);
    if (lstrlenA(_progname) > EXE_SUFFIX_SZ && ((p = AplRindexA(_progname, '.')) != NULL)) {
        if (!lstrcmpiA(p, EXE_SUFFIX))
            *p = '\0';
    }
    
    if (lstrcmpiA(_progname, "jar2exe") == 0)
        return jar2exe(argc, argv);
    else if (lstrcmpiA(_progname, "jar2exew") == 0) {
        /* default is to make gui app */
        _makegui = TRUE;
        return jar2exe(argc, argv);
    }
    else /* run the embedded jar */
        return run(argc, argv);
    
    /* NOT REACHED */
    return 0;
}
