/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2020 Couchbase, Inc
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
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
        "narenas:1,"
        /* Start with profiling enabled but inactive; this allows us to
           turn it on/off at runtime. */
        "prof:true,prof_active:false";