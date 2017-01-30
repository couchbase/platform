/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#pragma once

#cmakedefine HAVE_BACKTRACE 1
#cmakedefine HAVE_DLADDR 1
#cmakedefine HAVE_PTHREAD_SETNAME_NP 1
#cmakedefine HAVE_PTHREAD_GETNAME_NP 1
#cmakedefine HAVE_SCHED_GETAFFINITY 1

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#else

#if !defined(__cplusplus) && !defined(linux) && !defined(__GNUC__)
typedef unsigned long long uint64_t;
typedef long long int64_t;
#endif

#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS
#endif

#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <pwd.h>
#include <sys/time.h>
#include <signal.h>
#include <inttypes.h>

#endif // WIN32

/* Common section */
#include <stdlib.h>
#include <sys/types.h>

#include <platform/platform.h>
