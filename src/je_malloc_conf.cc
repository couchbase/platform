/*
 *     Copyright 2020-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <jemalloc/jemalloc.h>

/* jemalloc checks for this symbol, and it's contents for the config to use. */
const char* je_malloc_conf =
/* Enable background worker thread for asynchronous purging.
 * Background threads are non-functional in jemalloc 5.1.0 on macOS due to
 * implementation discrepancies between the background threads and mutexes.
 */
#ifndef __APPLE__
        "background_thread:true,"
#endif
        /* Use just one arena, instead of the default based on number of CPUs.
           Helps to minimize heap fragmentation. */
        "narenas:1"
#ifdef __linux__
        /* Start with profiling enabled but inactive; this allows us to
           turn it on/off at runtime. */
        ",prof:true,prof_active:false"
#endif
        ;
