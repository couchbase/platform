/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
#include "config.h"

#include <platform/cb_malloc.h>
#include <platform/strerror.h>
#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <vector>

struct thread_execute {
    cb_thread_main_func func;
    void *argument;
};

static DWORD WINAPI platform_thread_wrap(LPVOID arg)
{
    auto *ctx = reinterpret_cast<struct thread_execute*>(arg);
    assert(ctx);
    ctx->func(ctx->argument);
    delete ctx;
    return 0;
}


__declspec(dllexport)
int cb_create_thread(cb_thread_t *id,
                     void (*func)(void *arg),
                     void *arg,
                     int detached)
{
    HANDLE handle;

    struct thread_execute *ctx;
    try {
        ctx = new struct thread_execute;
    } catch (std::bad_alloc) {
        return -1;
    }

    ctx->func = func;
    ctx->argument = arg;

    handle = CreateThread(NULL, 0, platform_thread_wrap, ctx, 0, id);
    if (handle == NULL) {
        delete ctx;
        return -1;
    } else {
        if (detached) {
            CloseHandle(handle);
        }
    }

    return 0;
}

__declspec(dllexport)
int cb_create_named_thread(cb_thread_t *id, void (*func)(void *arg), void *arg,
                     int detached, const char* name)
{
    // Thread naming not supported on WIN32, just call down to non-named
    // variant.
    return cb_create_thread(id, func, arg, detached);
}

__declspec(dllexport)
int cb_join_thread(cb_thread_t id)
{
    HANDLE handle = OpenThread(SYNCHRONIZE, FALSE, id);
    if (handle == NULL) {
        return -1;
    }
    WaitForSingleObject(handle, INFINITE);
    CloseHandle(handle);
    return 0;
}

__declspec(dllexport)
cb_thread_t cb_thread_self(void)
{
    return GetCurrentThreadId();
}

__declspec(dllexport)
int cb_thread_equal(const cb_thread_t a, const cb_thread_t b)
{
    return a == b;
}

__declspec(dllexport)
int cb_set_thread_name(const char*)
{
    // Not implemented on WIN32
    return -1;
}

__declspec(dllexport)
int cb_get_thread_name(char*, size_t)
{
    return -1;
}

__declspec(dllexport)
bool is_thread_name_supported(void)
{
    return false;
}

__declspec(dllexport)
void cb_mutex_initialize(cb_mutex_t *mutex)
{
    InitializeCriticalSection(mutex);
}

__declspec(dllexport)
void cb_mutex_destroy(cb_mutex_t *mutex)
{
    DeleteCriticalSection(mutex);
}

__declspec(dllexport)
void cb_mutex_enter(cb_mutex_t *mutex)
{
    EnterCriticalSection(mutex);
}

__declspec(dllexport)
int cb_mutex_try_enter(cb_mutex_t *mutex)
{
    return TryEnterCriticalSection(mutex) ? 0 : -1;
}

__declspec(dllexport)
void cb_mutex_exit(cb_mutex_t *mutex)
{
    LeaveCriticalSection(mutex);
}

__declspec(dllexport)
void cb_cond_initialize(cb_cond_t *cond)
{
    InitializeConditionVariable(cond);
}

__declspec(dllexport)
void cb_cond_destroy(cb_cond_t *cond)
{
    (void)cond;
}

__declspec(dllexport)
void cb_cond_wait(cb_cond_t *cond, cb_mutex_t *mutex)
{
    SleepConditionVariableCS(cond, mutex, INFINITE);
}

__declspec(dllexport)
void cb_cond_timedwait(cb_cond_t *cond, cb_mutex_t *mutex, unsigned int msec) {
    SleepConditionVariableCS(cond, mutex, msec);
}

__declspec(dllexport)
void cb_cond_signal(cb_cond_t *cond)
{
    WakeConditionVariable(cond);
}

__declspec(dllexport)
void cb_cond_broadcast(cb_cond_t *cond)
{
    WakeAllConditionVariable(cond);
}

static const char *get_dll_name(const char *path, char *buffer)
{
    if (strstr(path, ".dll") != nullptr) {
        return path;
    }

    strcpy(buffer, path);
    char *ptr = strstr(buffer, ".so");
    if (ptr != NULL) {
        sprintf(ptr, ".dll");
        return buffer;
    }

    strcat(buffer, ".dll");
    return buffer;
}

__declspec(dllexport)
cb_dlhandle_t cb_dlopen(const char *library, char **errmsg)
{
    cb_dlhandle_t handle;

    if (library == NULL) {
        if (errmsg != NULL) {
            *errmsg = cb_strdup("Open self is not supported");
        }
        return NULL;
    }

    std::vector<char> buffer(strlen(library) + 20, 0);

    handle = LoadLibrary(get_dll_name(library, buffer.data()));
    if (handle == NULL && errmsg != NULL) {
        std::string reason = cb_strerror();
        *errmsg = cb_strdup(reason.c_str());
    }

    return handle;
}

__declspec(dllexport)
void *cb_dlsym(cb_dlhandle_t handle, const char *symbol, char **errmsg)
{
    void *ret = GetProcAddress(reinterpret_cast<HMODULE>(handle), symbol);
    if (ret == NULL && errmsg) {
        std::string reason = cb_strerror();
        *errmsg = cb_strdup(reason.c_str());
    }

    return ret;
}

__declspec(dllexport)
void cb_dlclose(cb_dlhandle_t handle)
{
    FreeLibrary(reinterpret_cast<HMODULE>(handle));
}

__declspec(dllexport)
void usleep(unsigned int useconds)
{
    unsigned int msec = useconds / 1000;
    if (msec == 0) {
        msec = 1;
    }
    Sleep(msec);
}

__declspec(dllexport)
int gettimeofday(struct timeval *tv, void *tz)
{
    FILETIME ft;
    uint64_t usecs;
    LARGE_INTEGER li;
    uint64_t secs;

    assert(tz == NULL); /* I don't support that right now */
    assert(tv != NULL); /* I don't support that right now */

    GetSystemTimeAsFileTime(&ft);
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    usecs = li.QuadPart;

    // FILETIME is 100 nanosecs from from 1. jan 1601
    // convert it to usecs..
    usecs /= 10;

    secs = usecs / 1000000;
    tv->tv_usec = usecs % 1000000;

    // gettimeofday use 1st january 1970, subtract the secs
    // between the dates
    secs -= 11644473600;
    tv->tv_sec = (unsigned long)secs;

    return 0;
}

__declspec(dllexport)
int platform_set_binary_mode(FILE *fp)
{
    return _setmode(_fileno(fp), _O_BINARY);
}

__declspec(dllexport)
void cb_rw_lock_initialize(cb_rwlock_t *rw)
{
    InitializeSRWLock(rw);
}

__declspec(dllexport)
void cb_rw_lock_destroy(cb_rwlock_t *rw)
{
    (void)rw;
    // Nothing todo on windows
}

__declspec(dllexport)
int cb_rw_reader_enter(cb_rwlock_t *rw)
{
    AcquireSRWLockShared(rw);
    return 0;
}

__declspec(dllexport)
int cb_rw_reader_exit(cb_rwlock_t *rw)
{
    ReleaseSRWLockShared(rw);
    return 0;
}

__declspec(dllexport)
int cb_rw_writer_enter(cb_rwlock_t *rw)
{
    AcquireSRWLockExclusive(rw);
    return 0;
}

__declspec(dllexport)
int cb_rw_writer_exit(cb_rwlock_t *rw)
{
    ReleaseSRWLockExclusive(rw);
    return 0;
}