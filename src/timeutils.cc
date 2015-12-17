/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015 Couchbase, Inc.
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

#include <platform/timeutils.h>

std::string Couchbase::hrtime2text(hrtime_t time) {
    const char* const extensions[] = {" ns", " us", " ms", " s", nullptr};
    int id = 0;

    while (time > 9999) {
        ++id;
        time /= 1000;
        if (extensions[id + 1] == NULL) {
            break;
        }
    }

    std::string ret;
    if (extensions[id + 1] == NULL && time > 599) {
        int hour = static_cast<int>(time / 3600);
        time -= hour * 3600;
        int min = static_cast<int>(time / 60);
        time -= min * 60;

        if (hour > 0) {
            ret = std::to_string(hour) + "h:" + std::to_string(min) + "m:" +
                  std::to_string(time) + "s";
        } else {
            ret = std::to_string(min) + "m:" + std::to_string(time) + "s";
        }
    } else {
        ret = std::to_string(time) + extensions[id];
    }

    return ret;
}
