/*
 *     Copyright 2019-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
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
