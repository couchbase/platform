/*
 *     Copyright 2018-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/timeutils.h>
#include <cctype>
#include <cmath>
#include <stdexcept>

std::string cb::time2text(std::chrono::nanoseconds time2convert) {
    const char* const extensions[] = {" ns", " us", " ms", " s", nullptr};
    int id = 0;
    double time = time2convert.count();

    while (time > 9999) {
        ++id;
        time /= 1000;
        if (extensions[id + 1] == nullptr) {
            break;
        }
    }

    std::string ret;
    if (extensions[id + 1] == nullptr && time > 599) {
        int hour = static_cast<int>(time / 3600);
        time -= hour * 3600;
        int min = static_cast<int>(time / 60);
        time -= min * 60;

        if (hour > 0) {
            ret = std::to_string(hour) + "h:" + std::to_string(min) + "m:" +
                  std::to_string(int(round(time))) + "s";
        } else {
            ret = std::to_string(min) + "m:" + std::to_string(int(round(time))) + "s";
        }
    } else {
        ret = std::to_string(int(round(time))) + extensions[id];
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
