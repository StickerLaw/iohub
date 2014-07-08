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

#ifndef IOHUB_THROTTLE_H
#define IOHUB_THROTTLE_H

#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>

#define UNKNOWN_UID 0xffffffff

struct uid_config {
    /** Next in linked list. */
    const struct uid_config *next;

    /** UID. */
    uint32_t uid;

    /** Minimum bytes per period. */
    uint64_t full;
};

/**
 * Initialize the throttling subsystem.
 *
 * Must be called before any other function here.
 *
 * @param list          Linked list of uid_config structures to configure the
 *                          throttler with.  Non-owned pointer.
 */
void throttle_init(const struct uid_config *list);

/**
 * Throttle the current thread.
 *
 * This will block until the system is ready to let us do the operation.
 *
 * @param uid           The current user ID.
 * @param amt           Size of the I/O operation we'd like to do.
 */
void throttle(uint32_t uid, uint64_t amt);

#endif

// vim: ts=4:sw=4:tw=79:et
