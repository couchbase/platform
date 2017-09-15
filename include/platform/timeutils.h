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
#pragma once

#include <platform/platform.h>
#include <chrono>
#include <string>


/* PLATFORM_PUBLIC_API
 *
 * Used for functions which are part of the public API of platform.
 * "Inside" platform (i.e. when compiling platform.so) they will export the
 * symbol
 * "Outside" platform (i.e. when compiling code which wants to link to
 * platform.so) they will allow the symbol to be imported from platform.so
 */
#if defined(platform_so_EXPORTS)
#define PLATFORM_PUBLIC_API EXPORT_SYMBOL
#else
#define PLATFORM_PUBLIC_API IMPORT_SYMBOL
#endif

namespace cb {
/**
 * Convert a time (in ns) to a human readable form...
 * @param time the time in nanoseconds
 * @return a string representation of the timestamp
 */
PLATFORM_PUBLIC_API
std::string time2text(const std::chrono::nanoseconds time);
}
