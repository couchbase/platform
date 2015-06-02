#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <errno.h>

struct thread_execute {
    cb_thread_main_func func;
    const char* name;
    void *argument;
};

static void *platform_thread_wrap(void *arg)
{
    struct thread_execute *ctx = arg;
    assert(arg);
    if (ctx->name != NULL) {
        cb_set_thread_name(ctx->name);
    }
    ctx->func(ctx->argument);
    free((void*)ctx->name);
    free(ctx);
    return NULL;
}

int cb_create_thread(cb_thread_t *id,
                     cb_thread_main_func func,
                     void *arg,
                     int detached)
{
    // Implemented in terms of cb_create_named_thread; with a NULL name.
    return cb_create_named_thread(id, func, arg, detached, NULL);
}

int cb_create_named_thread(cb_thread_t *id, cb_thread_main_func func, void *arg,
                           int detached, const char* name)
{
    int ret;
    struct thread_execute *ctx = malloc(sizeof(struct thread_execute));
    if (ctx == NULL) {
        return -1;
    }

    ctx->func = func;
    ctx->argument = arg;
    if (name != NULL) {
        if (strlen(name) > 15) {
            return 0;
        }
        ctx->name = strdup(name);
    } else {
        ctx->name = NULL;
    }

    if (detached) {
        pthread_attr_t attr;

        if (pthread_attr_init(&attr) != 0 ||
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
            return -1;
        }

        ret = pthread_create(id, &attr, platform_thread_wrap, ctx);
    } else {
        ret = pthread_create(id, NULL, platform_thread_wrap, ctx);
    }

    if (ret != 0) {
        free(ctx);
    }

    return ret;
}

int cb_join_thread(cb_thread_t id)
{
    return pthread_join(id, NULL);
}

cb_thread_t cb_thread_self(void)
{
    return pthread_self();
}

int cb_thread_equal(const cb_thread_t a, const cb_thread_t b)
{
    return pthread_equal(a, b);
}

void cb_set_thread_name(const char* name)
{
#if defined(__APPLE__)
        // No thread argument (implicit current thread).
        pthread_setname_np(name);
#else
        pthread_setname_np(pthread_self(), name);
#endif
}

void cb_mutex_initialize(cb_mutex_t *mutex)
{
    pthread_mutex_init(mutex, NULL);
}

void cb_mutex_destroy(cb_mutex_t *mutex)
{
    pthread_mutex_destroy(mutex);
}

void cb_mutex_enter(cb_mutex_t *mutex)
{
    int rv = pthread_mutex_lock(mutex);
    if (rv != 0) {
        fprintf(stderr, "FATAL: Failed to lock mutex: %d %s",
                rv, strerror(rv));
        abort();
    }
}

int cb_mutex_try_enter(cb_mutex_t *mutex) {
    return pthread_mutex_trylock(mutex) == 0 ? 0 : -1;
}

void cb_mutex_exit(cb_mutex_t *mutex)
{
    int rv = pthread_mutex_unlock(mutex);
    if (rv != 0) {
        fprintf(stderr, "FATAL: Failed to release mutex: %d %s",
                rv, strerror(rv));
        abort();
    }
}

void cb_cond_initialize(cb_cond_t *cond)
{
    pthread_cond_init(cond, NULL);
}

void cb_cond_destroy(cb_cond_t *cond)
{
    pthread_cond_destroy(cond);
}

void cb_cond_wait(cb_cond_t *cond, cb_mutex_t *mutex)
{
    pthread_cond_wait(cond, mutex);
}

void cb_cond_signal(cb_cond_t *cond)
{
    pthread_cond_signal(cond);
}

void cb_cond_broadcast(cb_cond_t *cond)
{
    pthread_cond_broadcast(cond);
}

void cb_cond_timedwait(cb_cond_t *cond, cb_mutex_t *mutex, unsigned int ms)
{
    struct timespec ts;
    struct timeval tp;
    int ret;
    uint64_t wakeup;

    memset(&ts, 0, sizeof(ts));

    /*
     * Unfortunately pthreads don't support relative sleeps so we need
     * to convert back to an absolute time...
     */
    gettimeofday(&tp, NULL);
    wakeup = ((uint64_t)(tp.tv_sec) * 1000) + (tp.tv_usec / 1000) + ms;
    /* Round up for sub ms */
    if ((tp.tv_usec % 1000) > 499) {
        ++wakeup;
    }

    ts.tv_sec = wakeup / 1000;
    wakeup %= 1000;
    ts.tv_nsec = wakeup * 1000000;

    ret = pthread_cond_timedwait(cond, mutex, &ts);
    switch (ret) {
    case EINVAL:
    case EPERM:
        fprintf(stderr, "FATAL: pthread_cond_timewait: %s\n",
                strerror(ret));
        abort();
    default:
        ;
    }
}

#ifdef __APPLE__
static const char *get_dll_name(const char *path, char *buffer)
{
    char *ptr = strstr(path, ".dylib");
    if (ptr != NULL) {
        return path;
    }

    strcpy(buffer, path);

    ptr = strstr(buffer, ".so");
    if (ptr != NULL) {
        sprintf(ptr, ".dylib");
        return buffer;
    }

    strcat(buffer, ".dylib");
    return buffer;
}
#else
static const char *get_dll_name(const char *path, char *buffer)
{
    char *ptr = strstr(path, ".so");
    if (ptr != NULL) {
        return path;
    }

    strcpy(buffer, path);
    strcat(buffer, ".so");
    return buffer;
}
#endif

cb_dlhandle_t cb_dlopen(const char *library, char **errmsg)
{
    cb_dlhandle_t handle;
    char *buffer = NULL;

    if (library == NULL) {
        handle = dlopen(NULL, RTLD_NOW | RTLD_LOCAL);
    } else {
        handle = dlopen(library, RTLD_NOW | RTLD_LOCAL);
        if (handle == NULL) {
            buffer = malloc(strlen(library) + 20);
            if (buffer == NULL) {
                if (*errmsg) {
                    *errmsg = strdup("Failed to allocate memory");
                }
                return NULL;
            }

            handle = dlopen(get_dll_name(library, buffer),
                            RTLD_NOW | RTLD_LOCAL);
            free(buffer);
        }
    }

    if (handle == NULL && errmsg != NULL) {
        *errmsg = strdup(dlerror());
    }

    return handle;
}

void *cb_dlsym(cb_dlhandle_t handle, const char *symbol, char **errmsg)
{
    void *ret = dlsym(handle, symbol);
    if (ret == NULL && errmsg) {
        *errmsg = strdup(dlerror());
    }
    return ret;
}

void cb_dlclose(cb_dlhandle_t handle)
{
    dlclose(handle);
}

int platform_set_binary_mode(FILE *fp)
{
    (void)fp;
    return 0;
}
