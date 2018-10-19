/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2018 Couchbase, Inc.
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
#include <cctype>
#include <stdexcept>

std::string cb::time2text(std::chrono::nanoseconds time2convert) {
    const char* const extensions[] = {" ns", " us", " ms", " s", nullptr};
    int id = 0;
    auto time = time2convert.count();

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

std::chrono::nanoseconds cb::text2time(const std::string& text) {
    std::size_t pos = 0;
    auto value = std::stoi(text, &pos);

    // trim off whitespace between the number and the textual description
    while (std::isspace(text[pos])) {
        ++pos;
    }

    std::string specifier{text.substr(pos)};
    // Trim off trailing whitespace
    pos = specifier.find(' ');
    if (pos != std::string::npos) {
        specifier.resize(pos);
    }

    if (specifier.empty()) {
        return std::chrono::milliseconds(value);
    }

    if (specifier == "ns" || specifier == "nanoseconds") {
        return std::chrono::nanoseconds(value);
    }

    if (specifier == "us" || specifier == "microseconds") {
        return std::chrono::microseconds(value);
    }

    if (specifier == "ms" || specifier == "milliseconds") {
        return std::chrono::milliseconds(value);
    }

    if (specifier == "s" || specifier == "seconds") {
        return std::chrono::seconds(value);
    }

    if (specifier == "m" || specifier == "minutes") {
        return std::chrono::minutes(value);
    }

    if (specifier == "h" || specifier == "hours") {
        return std::chrono::hours(value);
    }

    throw std::invalid_argument("cb::text2time: Invalid format: " + text);
}
