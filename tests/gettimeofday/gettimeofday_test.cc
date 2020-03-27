/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2019 Couchbase, Inc.
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

#include <platform/platform_time.h>

#include <iostream>
#ifdef _MSC_VER
#include <time.h>
#else
#include <sys/time.h>
#endif

int main()
{
    struct timeval tv;
    if (gettimeofday(&tv, nullptr) != 0) {
        std::cerr << "gettimeofday returned != 0" << std::endl;
        return 1;
    }

        time_t now = time(nullptr);
        if (tv.tv_sec > now) {
            std::cerr << "gettimeofday returned a date in the future " << std::endl
                << "time returns " << now << " and tv_sec " << tv.tv_sec << " tv_usec " << tv.tv_usec
                << std::endl;
            return 1;
        }

        if (now != tv.tv_sec && now != (tv.tv_sec + 1)) {
            // the test shouldn't take more than a sec to run, so it should be
            std::cerr << "gettimeofday returned a date too long ago " << std::endl
                << "time returns " << now << " and tv_sec " << tv.tv_sec << " tv_usec " << tv.tv_usec
                << std::endl;
            return 1;
        }

   return 0;
}
