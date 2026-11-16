/*
   Licensed to the Apache Software Foundation (ASF) under one or more
   contributor license agreements.  See the NOTICE file distributed with
   this work for additional information regarding copyright ownership.
   The ASF licenses this file to You under the Apache License, Version 2.0
   (the "License"); you may not use this file except in compliance with
   the License.  You may obtain a copy of the License at

       https://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include "jsvc.h"
#include <errno.h>

/* These checks must remain active even when NDEBUG is defined. */
#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: %s failed\n", __FILE__, __LINE__, #condition); \
        exit(1); \
    } \
} while (0)

/* Create parent directories for a fixture file inside the temporary home. */
static FILE *create_file(char *path)
{
    char *p;
    FILE *file;

    for (p = path + 1; *p != '\0'; p++) {
        if (*p == '/') {
            *p = '\0';
            CHECK(mkdir(path, 0700) == 0 || errno == EEXIST);
            *p = '/';
        }
    }
    file = fopen(path, "w");
    CHECK(file != NULL);
    return file;
}

static void library_path(char *path, char *java_home, char *name)
{
    char temp[1024];

    CHECK(replace(temp, sizeof(temp), location_jvm_configured[0], "$JAVA_HOME", java_home) == 0);
    CHECK(replace(path, 1024, temp, "$VM_NAME", name) == 0);
}

int main(int argc, char *argv[])
{
    char cfg[1024];
    char path[1024];
    char name[32];
    FILE *file;
    home_data *data;
    int count, duplicates, i;

    CHECK(argc == 4);
    count = atoi(argv[2]);
    duplicates = atoi(argv[3]);
    CHECK(count >= 0 && count <= 257);
    CHECK(location_jvm_cfg[0] != NULL);
    CHECK(location_jvm_configured[0] != NULL);
    CHECK(replace(cfg, sizeof(cfg), location_jvm_cfg[0], "$JAVA_HOME", argv[1]) == 0);
    file = create_file(cfg);
    CHECK(fputs("# Test configuration\n\n", file) >= 0);
    for (i = 0; i < count; i++) {
        FILE *library;

        sprintf(name, "vm%d", duplicates ? 0 : i);
        library_path(path, argv[1], name);
        library = create_file(path);
        CHECK(fclose(library) == 0);
        CHECK(fprintf(file, "-missing KNOWN\n-%s KNOWN\n", name) > 0);
    }
    /* Unresolvable definitions must not consume the last available slot. */
    CHECK(fputs("-missing KNOWN\n# trailing comment\n\n", file) >= 0);
    CHECK(fclose(file) == 0);

    data = home(argv[1]);
    if (count > 255) {
        /* Reject the whole configuration, not a truncated VM table. */
        CHECK(data == NULL);
    }
    else {
        CHECK(data != NULL);
        CHECK(data->jnum == count);
        CHECK(data->jvms != NULL);
        CHECK(data->jvms[count] == NULL);
        for (i = 0; i < count; i++) {
            sprintf(name, "vm%d", duplicates ? 0 : i);
            library_path(path, argv[1], name);
            CHECK(data->jvms[i] != NULL);
            CHECK(strcmp(data->jvms[i]->name, name) == 0);
            CHECK(strcmp(data->jvms[i]->libr, path) == 0);
            free(data->jvms[i]->name);
            free(data->jvms[i]->libr);
            free(data->jvms[i]);
        }
        free(data->jvms);
        free(data->cfgf);
        free(data->path);
        free(data);
    }
    printf("PASS: %d resolved entries, duplicates=%d\n", count, duplicates);
    return 0;
}
