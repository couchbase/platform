/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015 Couchbase, Inc
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
#include "config.h"

hrtime_t gethrtime_period(void)
{
    /* this isn't actually completely accurate, but who cares ;-) */
    hrtime_t start = gethrtime();
    hrtime_t end = gethrtime() - start;
    if (end == 0) {
       end = 1;
    }

    return end;
}

#ifndef __sun
hrtime_t gethrtime(void)
{
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME_FAST, &tp);
    hrtime_t ret = tp.tv_sec;
    ret *= 1000; // ms
    ret *= 1000; // us
    ret *= 1000; // ns
    ret += tp.tv_nsec;

    return ret;
}
#endif
