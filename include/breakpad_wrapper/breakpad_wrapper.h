/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

/*
* A set of "extern C" functions to allow non-C++ applications to invoke
* Breakpad.
*/

#pragma once


#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Setup Breakpad, specifying the directory where any minidumps should be
 * written. Should be invoked as part of program initialization, to reserve
 * memory for Breakpad's operation. *MUST* have been called before attempting
 * to write a minidump with breakpad_write_minidump().
 *
 * Note: Breakpad is configured to *NOT* automatically handle crashes and
 *       write a minidump file, users must call breakpad_write_minidump() when
 *       they wish to create a dump file.
 *
 * @param minidump_dir Path to an existing, writable directory which
 *        minidumps will be written to.
 */
void breakpad_initialize(const char* minidump_dir);

/* Writes a minidump of the current application state, to the directory
 * previously specified to breakpad_initialize().
 * @return True if the minidump was successfully written, else false.
 */
bool breakpad_write_minidump();

/* Returns the address of breakpad_write_minidump() function.
 * Provided to facilite passing that symbol into foreign environments
 * (e.g. Golang) for later use as a C function pointer.
 */
uintptr_t breakpad_get_write_minidump_addr();

#ifdef __cplusplus
} // extern "C"
#endif
