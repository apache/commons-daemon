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
 
#ifndef _CMDLINE_H_INCLUDED_
#define _CMDLINE_H_INCLUDED_

__APXBEGIN_DECLS

#define APXCMDOPT_NIL   0x00000000  /* Argopt value not needed */
#define APXCMDOPT_INT   0x00000001  /* Argopt value is unsigned integer */
#define APXCMDOPT_STR   0x00000002  /* Argopt value is string */
#define APXCMDOPT_STE   0x00000006  /* Argopt value is expandable string */
#define APXCMDOPT_MSZ   0x00000010  /* Multiline string '#' separated */
#define APXCMDOPT_BIN   0x00000020  /* Encrypted binary */

#define APXCMDOPT_REG   0x00000100  /* Save to registry */
#define APXCMDOPT_SRV   0x00000200  /* Save to service registry */
#define APXCMDOPT_USR   0x00000400  /* Save to user registry */

#define APXCMDOPT_FOUND 0x00001000  /* The option is present in cmdline as -- */
#define APXCMDOPT_ADD   0x00002000  /* The option is present in cmdline as ++ */


typedef struct APXCMDLINEOPT APXCMDLINEOPT;

struct APXCMDLINEOPT {
    LPWSTR          szName;         /* Long Argument Name */
    LPWSTR          szRegistry;     /* Registry Association */
    LPWSTR          szSubkey;       /* Registry Association */
    DWORD           dwType;         /* Argument type (string, number, multistring */
    LPWSTR          szValue;        /* Return string value  */
    DWORD           dwValue;        /* Return numeric value or present if NIL */
};

typedef struct APXCMDLINE {
    APXCMDLINEOPT       *lpOptions;
    LPWSTR              szArgv0;
    LPWSTR              szExecutable;   /* Parsed argv0 */
    LPWSTR              szExePath;      /* Parsed argv0 */
    LPWSTR              szApplication;  /* Fist string after //CMD// */
    DWORD               dwCmdIndex;     /* Command index */
    LPWSTR              *lpArgvw;
    DWORD               dwArgc;
    APXHANDLE           hPool;
    
} APXCMDLINE, *LPAPXCMDLINE;

LPAPXCMDLINE apxCmdlineParse(
    APXHANDLE hPool,
    APXCMDLINEOPT   *lpOptions,
    LPCWSTR         *lpszCommands,
    LPCWSTR         *lpszAltcmds
);

void apxCmdlineLoadEnvVars(
    LPAPXCMDLINE lpCmdline
);

void apxCmdlineFree(
    LPAPXCMDLINE lpCmdline
);


__APXEND_DECLS

#endif /* _CMDLINE_H_INCLUDED_ */
