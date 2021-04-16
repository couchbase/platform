/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
*     Copyright 2015 Couchbase, Inc
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
