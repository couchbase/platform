/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <string>

#ifdef WIN32
// Need DWORD
#ifndef WIN32_LEAN_AND_MEAN
#define DO_UNDEF_WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#ifdef DO_UNDEF_WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif
#endif

/**
 * Get a textual string of the current system error code (GetLastError
 * on windows and errno on mac/linux/unix)
 */
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
std::string cb_strerror(cb_os_error_t error);
