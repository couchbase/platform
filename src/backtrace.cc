/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <platform/backtrace.h>
#include <boost/stacktrace/stacktrace.hpp>
#include <cinttypes>

#if defined(WIN32)
#include <windows.h>
#include <Dbghelp.h>
#else
#include <dlfcn.h> // for dladdr()
#include <execinfo.h> // for backtrace()
#endif

// Maximum number of frames that will be printed.
#define MAX_FRAMES 50

/**
 * Populates buf with a description of the given address in the program.
 **/
static void describe_address(char* msg, size_t len, int frame_num,
                             const void* addr) {
    // Prefix with frame number.
    auto prefix_len = snprintf(msg, 8, "#%-2d ", frame_num);
    msg += prefix_len;
    len -= prefix_len;

#if defined(WIN32)
    // Get module information
    IMAGEHLP_MODULE64 module_info;
    module_info.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
    SymGetModuleInfo64(GetCurrentProcess(), (DWORD64)addr, &module_info);

    // Get symbol information.
    DWORD64 displacement = 0;
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    PSYMBOL_INFO sym_info = (PSYMBOL_INFO)buffer;
    sym_info->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym_info->MaxNameLen = MAX_SYM_NAME;

    if (SymFromAddr(
                GetCurrentProcess(), (DWORD64)addr, &displacement, sym_info)) {
        snprintf(msg,
                 len,
                 "%s(%s+%lld) [0x%p]",
                 module_info.ImageName ? module_info.ImageName : "",
                 sym_info->Name,
                 displacement,
                 addr);
    } else {
        // No symbol found.
        snprintf(msg, len, "[0x%p]", addr);
    }
#else // !WIN32
    Dl_info info;
    int status = dladdr(addr, &info);

    if (status != 0) {
        ptrdiff_t image_offset = (char*)addr - (char*)info.dli_fbase;
        if (info.dli_fname != nullptr && info.dli_fname[0] != '\0') {
            // Found a nearest symbol - print it.
            if (info.dli_saddr == 0) {
                // No function offset calculation possible.
                snprintf(msg,
                         len,
                         "%s(%s) [%p+0x%" PRIx64 "]",
                         info.dli_fname,
                         info.dli_sname ? info.dli_sname : "",
                         info.dli_fbase,
                         (uint64_t)image_offset);
            } else {
                char sign;
                ptrdiff_t offset;
                if (addr >= info.dli_saddr) {
                    sign = '+';
                    offset = (char*)addr - (char*)info.dli_saddr;
                } else {
                    sign = '-';
                    offset = (char*)info.dli_saddr - (char*)addr;
                }
                snprintf(msg,
                         len,
                         "%s(%s%c%#tx) [%p+0x%" PRIx64 "]",
                         info.dli_fname,
                         info.dli_sname ? info.dli_sname : "",
                         sign,
                         offset,
                         info.dli_fbase,
                         (uint64_t)image_offset);
            }
        } else {
            // No function found; just print library name and offset.
            snprintf(msg,
                     len,
                     "%s [%p+0x%" PRIx64 "]",
                     info.dli_fname,
                     info.dli_fbase,
                     (uint64_t)image_offset);
        }
    } else {
        // dladdr failed.
        snprintf(msg, len, "[%p]", addr);
    }
#endif // WIN32
}

void print_backtrace(write_cb_t write_cb, void* context) {
    void* frames[MAX_FRAMES];
#if defined(WIN32)
    int active_frames = CaptureStackBackTrace(0, MAX_FRAMES, frames, NULL);
#else
    int active_frames = backtrace(frames, MAX_FRAMES);
#endif

    // Note we start from 1 to skip our own frame.
    for (int ii = 1; ii < active_frames; ii++) {
        // Fixed-sized buffer; possible that description will be cropped.
        char msg[300];
        describe_address(msg, sizeof(msg), ii - 1, frames[ii]);
        write_cb(context, msg);
    }
    if (active_frames == MAX_FRAMES) {
        write_cb(context, "<frame limit reached, possible truncation>");
    }
}

void print_backtrace_frames(const boost::stacktrace::stacktrace& frames,
                            std::function<void(const char* frame)> callback) {
    for (size_t ii = 0; ii < frames.size(); ii++) {
        // Fixed-sized buffer; possible that description will be cropped.
        char msg[300];
        describe_address(msg, sizeof(msg), ii, frames[ii].address());
        callback(msg);
    }
}

static void print_to_file_cb(void* ctx, const char* frame) {
    auto* stream = reinterpret_cast<FILE*>(ctx);
    fprintf(stream, "\t%s\n", frame);
}

void print_backtrace_to_file(FILE* stream) {
    print_backtrace(print_to_file_cb, stream);
}

struct context {
    const char* indent;
    char* buffer;
    size_t size;
    size_t offset;
    bool error;
};

static void memory_cb(void* ctx, const char* frame) {
    auto* c = reinterpret_cast<context*>(ctx);

    if (!c->error) {
        int length = snprintf(c->buffer + c->offset,
                              c->size - c->offset,
                              "%s%s\n",
                              c->indent,
                              frame);

        if ((length < 0) || (size_t(length) >= (c->size - c->offset))) {
            c->error = true;
        } else {
            c->offset += length;
        }
    }
}

bool print_backtrace_to_buffer(const char* indent, char* buffer, size_t size) {
    context c{indent, buffer, size, 0, false};
    print_backtrace(memory_cb, &c);
    return !c.error;
}

void cb::backtrace::initialize() {
#ifdef WIN32
    // We can only call SymInitialize once per process; if it's already been
    // called do nothing.
    static bool initialized = false;
    if (initialized) {
        return;
    }
    initialized = true;
    if (!SymInitialize(GetCurrentProcess(), NULL, TRUE)) {
        throw std::system_error(
                int(GetLastError()), std::system_category(), "SymInitialize()");
    }
#endif
}
