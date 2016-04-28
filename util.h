/**
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

#ifndef IOHUB_UTIL_H
#define IOHUB_UTIL_H

#include <unistd.h> // for size_t

/**
 * Allocate a new region of memory and zero it, or die.
 */
void *xcalloc(size_t nmemb, size_t size);

/**
 * Like snprintf, but appends to a string that already exists.
 *
 * @param str       The string to append to
 * @param str_len   Maximum length allowed for str
 * @param fmt       Printf-style format string
 * @param ...       Printf-style arguments
 *
 * @return          0 on success; -ENAMETOOLONG if there was not enough
 *                      buffer space.
 */
int snappend(char *str, size_t str_len, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

/**
 * Convert a POSIX-open flags integer to a string.
 *
 * @param flags     The flags.
 * @param str       (out paramer) The buffer to write to.
 * @param max_len   Maximum length of the buffer to write to.
 *
 * @return          0 on success; -ENAMETOOLONG if there was not enough
 *                      buffer space.
 */
int open_flags_to_str(int flags, char *str, size_t max_len);

#endif

// vim: ts=4:sw=4:et
