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

#ifndef IOHUB_LOG_H
#define IOHUB_LOG_H

#include <stdio.h> // for printf

#undef DEBUG_ENABLED

#ifdef DEBUG_ENABLED
/**
 * Print a debug message.
 */
#define DEBUG(x, ...) printf(x, __VA_ARGS__);
#else 
#define DEBUG(x, ...) ; 
#endif 

/**
 * A thread-safe strerror alternative.
 *
 * @param errnum        The POSIX error number.
 * @return              A statically allocated error string.
 */
const char *terror(int err);

#endif

// vim: ts=4:sw=4:et
