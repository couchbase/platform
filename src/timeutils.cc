/*
 *     Copyright 2018-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <fmt/format.h>
#include <platform/split_string.h>
#include <platform/timeutils.h>
#include <cctype>
#include <charconv>
#include <cmath>
#include <stdexcept>
#include <thread>

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

static std::string_view trim_space(std::string_view text) {
    while (!text.empty() && text.front() == ' ') {
        text.remove_prefix(1);
    }

    while (!text.empty() && text.back() == ' ') {
        text.remove_suffix(1);
    }
    return text;
}

static std::chrono::nanoseconds text2nano(std::string_view text) {
    text = trim_space(text);

    int value{};
    const auto [ptr, ec]{
            std::from_chars(text.data(), text.data() + text.size(), value)};
    if (ec != std::errc()) {
        if (ec == std::errc::invalid_argument) {
            throw std::invalid_argument("text2nano: no conversion");
        }
        if (ec == std::errc::result_out_of_range) {
            throw std::out_of_range("text2nano: value exceeds integer");
        }
        throw std::system_error(std::make_error_code(ec));
    }

    // trim off what we've parsed
    text.remove_prefix(ptr - text.data());

    // trim off leading and trailing space
    text = trim_space(text);

    const auto specifier = text;
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

    throw std::invalid_argument(
            fmt::format("cb::text2time: Invalid format: {}", text));
}

std::chrono::nanoseconds cb::text2time(std::string_view text) {
    if (text.empty()) {
        throw std::invalid_argument(
                "cb::text2time: can't convert empty string");
    }
    auto pieces = cb::string::split(text, ':');
    std::chrono::nanoseconds ret{0};
    for (const auto& p : pieces) {
        ret += text2nano(std::string{p});
    }
    return ret;
}

std::chrono::microseconds cb::decayingSleep(
        std::chrono::microseconds uSeconds) {
    /* Max sleep time is slightly over a second */
    static const std::chrono::microseconds maxSleepTime(0x1 << 20);
    std::this_thread::sleep_for(uSeconds);
    return std::min(uSeconds * 2, maxSleepTime);
}

bool cb::waitForPredicateUntil(const std::function<bool()>& pred,
                               std::chrono::microseconds maxWaitTime) {
    using namespace std::chrono;
    auto deadline = steady_clock::now() + maxWaitTime;
    std::chrono::microseconds sleepTime(128);
    do {
        if (pred()) {
            return true;
        }
        sleepTime = decayingSleep(sleepTime);
    } while (steady_clock::now() < deadline);
    return false;
}

void cb::waitForPredicate(const std::function<bool()>& pred) {
    std::chrono::microseconds sleepTime(128);
    while (true) {
        if (pred()) {
            return;
        }
        sleepTime = decayingSleep(sleepTime);
    };
}
