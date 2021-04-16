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

#include <breakpad_wrapper/breakpad_wrapper.h>

#if defined(HAVE_BREAKPAD)
#  if defined(WIN32)
/*
 * For some unknown reason, Folly has decided to undefine some Windows types
 * that are required by "dbghelp.h", which is included in Breakpad's Windows
 * "exception_handler.h". Nobody bothered to document why exactly they were
 * undefined, and it looks as though they are not, and never were, used within
 * the Folly codebase. So, if we have already pulled in Folly's "Windows.h" at
 * this point, then we need to re-define OUT and IN. To avoid any potential
 * issues with Folly, we'll undefine them after we've included Breakpad's
 * "exception_handler.h". If we haven't pulled in Folly's "Windows.h", then
 * Breakpad will pull in it's own, but it won't undefine OUT and IN so there
 * won't be an issue.
 */
#ifdef _WINDOWS_
#ifndef OUT
#define UNDEFOUT 1
#define OUT
#endif // OUT

#ifndef IN
#define UNDEFIN 1
#define IN
#endif // IN
#endif // _WINOWS_
#    include "client/windows/handler/exception_handler.h"
#ifdef UNDEFOUT
#undef OUT
#endif

#ifdef UNDEFIN
#undef IN
#endif
#  elif defined(linux)
#    include "client/linux/handler/exception_handler.h"
#  else
#    error Unsupported platform for breakpad, cannot compile.
#  endif

using namespace google_breakpad;

ExceptionHandler* handler;

#if defined(WIN32)
/* Called by Breakpad when an exception triggers a dump. */
static bool dumpCallback(const wchar_t* dump_path, const wchar_t* minidump_id,
                         void* context, EXCEPTION_POINTERS* exinfo,
                         MDRawAssertionInfo* assertion, bool succeeded) {
    fprintf(stderr, "Breakpad caught crash in process. Writing crash dump to "
            "%S\\%S.dmp\n", dump_path, minidump_id);

    return succeeded;
}
#endif

#if defined(linux)
/* Called by Breakpad when an exception triggers a dump. */
static bool dumpCallback(const MinidumpDescriptor& descriptor,
                         void* context, bool succeeded) {
    fprintf(stderr, "Breakpad caught crash in process. Writing crash dump to "
            "%s\n", descriptor.path());

    return succeeded;
}
#endif

void breakpad_initialize(const char* minidump_dir) {
#if defined(WIN32)
    // Takes a wchar_t* on Windows. Isn't the Breakpad API nice and
    // consistent? ;)
    size_t len = strlen(minidump_dir) + 1;
    wchar_t * wc_minidump_dir = new wchar_t[len];
    size_t wlen = 0;
    mbstowcs_s(&wlen, wc_minidump_dir, len, minidump_dir, _TRUNCATE);

    if (handler != nullptr) {
        delete handler;
    }
    handler = new ExceptionHandler(wc_minidump_dir, /*filter*/NULL,
                                   dumpCallback,
                                   /*callback-context*/NULL,
                                   ExceptionHandler::HANDLER_NONE,
                                   MiniDumpNormal, /*pipe*/(wchar_t*)NULL,
                                   /*custom_info*/NULL);
    delete[] wc_minidump_dir;

#elif defined(linux)
    MinidumpDescriptor descriptor(minidump_dir);

    if (handler != nullptr) {
        delete handler;
    }
    handler = new ExceptionHandler(descriptor, /*filter*/nullptr,
                                   dumpCallback,
                                   /*callback-context*/nullptr,
                                   /*install_handler*/false, /*server_fd*/-1);
#endif /* defined({OS}) */
}

bool breakpad_write_minidump(void) {
    return handler->WriteMinidump();
}

uintptr_t breakpad_get_write_minidump_addr() {
    return (uintptr_t)breakpad_write_minidump;
}

#else /* defined(HAVE_BREAKPAD) */

void breakpad_initialize(const char* minidump_dir) {
    // do nothing
}

bool breakpad_write_minidump(void) {
    return false;
}

uintptr_t breakpad_get_write_minidump_addr() {
    return 0;
}

#endif /* defined(HAVE_BREAKPAD) */
