/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void *xcalloc(size_t nmemb, size_t size)
{
    void *v;

    v = calloc(nmemb, size);
    if (!v) {
        fprintf(stderr, "calloc(%zd, %zd) failed: OOM\n", nmemb, size);
        abort();
    }
    return v;
}

int snappend(char *str, size_t str_len, const char *fmt, ...)
{
    va_list ap;
    size_t slen = strlen(str);
    if (slen >= str_len + 1) {
        return -ENAMETOOLONG;
    }
    va_start(ap, fmt);
    vsnprintf(str + slen, str_len - slen, fmt, ap);
    va_end(ap);
    return 0;
}

int open_flags_to_str(int flags, char *str, size_t max_len)
{
    const char *prefix = "";

    // TODO: optimize this a bit
    if (flags & O_RDONLY) {
        if (snappend(str, max_len, "%sO_RDONLY", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_WRONLY) {
        if (snappend(str, max_len, "%sO_WRONLY", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_RDWR) {
        if (snappend(str, max_len, "%sO_RDWR", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_CREAT) {
        if (snappend(str, max_len, "%sO_CREAT", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_EXCL) {
        if (snappend(str, max_len, "%sO_EXCL", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_NOCTTY) {
        if (snappend(str, max_len, "%sO_NOCTTY", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_TRUNC) {
        if (snappend(str, max_len, "%sO_TRUNC", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_APPEND) {
        if (snappend(str, max_len, "%sO_APPEND", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_NONBLOCK) {
        if (snappend(str, max_len, "%sO_NONBLOCK", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_DSYNC) {
        if (snappend(str, max_len, "%sO_DSYNC", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & FASYNC) {
        if (snappend(str, max_len, "%sO_FASYNC", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_DIRECT) {
        if (snappend(str, max_len, "%sO_DIRECT", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_LARGEFILE) {
        if (snappend(str, max_len, "%sO_LARGEFILE", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_DIRECTORY) {
        if (snappend(str, max_len, "%sO_DIRECTORY", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_NOFOLLOW) {
        if (snappend(str, max_len, "%sO_NOFOLLOW", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_NOATIME) {
        if (snappend(str, max_len, "%sO_NOATIME", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_CLOEXEC) {
        if (snappend(str, max_len, "%sO_CLOEXEC", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    return 0;
}

// vim: ts=4:sw=4:tw=79:et
