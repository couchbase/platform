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
#include <string>

/**
 * Get a textual string of the current system error code (GetLastError
 * on windows and errno on mac/linux/unix)
 */
PLATFORM_PUBLIC_API
std::string cb_strerror();

#ifdef WIN32
#define cb_os_error_t DWORD
#else
#define cb_os_error_t int
#endif

/**
 * Get a textual string representation of the specified error code.
 * On windows system this is a DWORD returned by GetLastError or
 * WSAGetLastError, and on unix systems this is an integer (normally
 * the value set by errno)
 *
 * @param error The error code to look up
 * @return a textual representation of the error
 */
PLATFORM_PUBLIC_API
std::string cb_strerror(cb_os_error_t error);
