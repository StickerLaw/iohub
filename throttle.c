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

#include "htable.h"
#include "throttle.h"
#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/**
 * @file throttle.c
 *
 * This code implements I/O throttling for iohub.
 *
 * Time is broken up into "throttling periods" which are roughly 5 seconds
 * each.  During each period, each UID gets a fixed amount of bytes that it
 * can read or write.  Once it exhausts that fixed amount, it needs to go back
 * to the global pool for more bytes.
 *
 * To put some numbers to this, we might say that user foo (uid 1000) gets 20
 * MB/s minimum...  in other words, 5 * 20 * 1024 * 1024 = 104857600 bytes per
 * period.  The bandwidth of the drive might be 125 MB/s, or 655360000 bytes
 * per period.
 *
 * So foo can "steal" some bytes from the global pool once he exceeds his
 * minimum.  So can other users.
 */

/** Seconds per throttling period. */
#define SECS_PER_PERIOD 5

/** 
 * Number of bits we use to identify a throttling period.
 */
#define BITS_PER_PERIOD 20

/**
 * Bitmask for reading only bits related to the period number.
 */
#define PERIOD_MASK ((1 << BITS_PER_PERIOD) - 1)

struct uid_data {
    /** Allocation that this UID should get each period.  Immutable. */
    uint64_t full;

    /**
     * Top bits: remaining bytes in this period.
     * Bottom BITS_PER_PERIOD bits: current period.
     *
     * This must be accessed via atomic operations.
     */
    uint64_t cur;
};

/**
 * Table mapping UIDs to uid_data structures.
 *
 * We don't remove or insert anything from this table after throttle_init
 * completes, so we can safely access it without a lock.
 */
static struct htable *g_uid_table;

static uint32_t uid_hash_fun(const void *key, uint32_t capacity)
{
    uint32_t uid = (uint32_t)(uintptr_t)key;
    return uid % capacity;
}

static int uid_eq_fun(const void *a, const void *b)
{
    uint32_t ua = (uint32_t)(uintptr_t)a;
    uint32_t ub = (uint32_t)(uintptr_t)b;
    return ua == ub;
}

void throttle_init(const struct uid_config *list)
{
    const struct uid_config *conf;
    int ret, len = 0;
    struct uid_data *udata;

    for (conf = list; conf; conf = conf->next) {
        len++;
    }
    g_uid_table = htable_alloc(len * 4, uid_hash_fun, uid_eq_fun);
    if (!g_uid_table) {
        fprintf(stderr, "throttle_init: htable_alloc failed: out "
                "of memory.\n");
        abort();
    }
    for (conf = list; conf; conf = conf->next) {
        udata = xcalloc(1, sizeof(*udata));
        udata->full = conf->full;
        ret = htable_put(g_uid_table, (void*)(uintptr_t)conf->uid, udata);
        if (ret) {
            fprintf(stderr, "throttle_init: htable_put failed: error "
                    "%d (%s)\n", ret, strerror(ret));
            abort();
        }
    }
    if (!htable_get(g_uid_table, (void*)(uintptr_t)UNKNOWN_UID)) {
        fprintf(stderr, "throttle_init: you must specify an allocation "
                "for uid %d (all UIDs that we don't know about).\n",
                UNKNOWN_UID);
        abort();
    }
}

void throttle(uint32_t uid, uint64_t amt)
{
    struct uid_data *udata;
    int ret;
    uint32_t cur_period, prev_period;
    uint64_t avail, prev, next, nprev;
    struct timespec cur, delta;

    udata = htable_get(g_uid_table, (void*)(uintptr_t)uid);
    if (!udata) {
        udata = htable_get(g_uid_table, (void*)(uintptr_t)UNKNOWN_UID);
    }
    prev = __sync_fetch_and_or(&udata->cur, 0);
    while (1) {
        // Calculate the current period from the coarse monotonic time.
        // This should only take a few nanoseconds on modern Linux setups.
        ret = clock_gettime(CLOCK_MONOTONIC_RAW, &cur);
        if (ret) {
            fprintf(stderr, "clock_gettime failed with error %d (%s)\n",
                    ret, strerror(ret));
            abort();
        }
        cur_period = (cur.tv_sec / SECS_PER_PERIOD) & PERIOD_MASK;
        prev_period = prev & PERIOD_MASK;
        if (cur_period != prev_period) {
            // If the next period has rolled around, our allocation should have
            // renewed.
            avail = (udata->full << BITS_PER_PERIOD);
        } else {
            // See how many bytes are remaining for us in this period.
            avail = prev >> BITS_PER_PERIOD;
        }
        if (avail < amt) {
            if (amt > udata->full) {
                fprintf(stderr, "throttle: asked for more bytes than we can "
                        "source throughout an entire period.  full = %"PRId64
                        ", but we asked for %"PRId64"\n", udata->full, amt);
                abort();
            }
            // Try to sleep for the rest of the period.
            delta.tv_sec = ((cur_period + 1) * SECS_PER_PERIOD) - cur.tv_sec;
            delta.tv_nsec = 1000000000LL - cur.tv_nsec;
            nanosleep(&delta, NULL);
            prev = __sync_fetch_and_or(&udata->cur, 0);
            continue;
        }
        avail -= amt;
        next = cur_period | (avail << BITS_PER_PERIOD);
        nprev = __sync_val_compare_and_swap(&udata->cur, prev, next);
        if (nprev == prev) {
            // We have successfully claimed some bandwidth from the current
            // period.  We're done for now.
            break;
        }
        // Try, try again.
        prev = nprev;
    }
}

// vim: ts=4:sw=4:tw=79:et
