/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#pragma once

#cmakedefine HAVE_BACKTRACE 1
#cmakedefine HAVE_DLADDR 1
#cmakedefine HAVE_PTHREAD_SETNAME_NP 1
#cmakedefine HAVE_PTHREAD_GETNAME_NP 1
#cmakedefine HAVE_SCHED_GETAFFINITY 1
#cmakedefine HAVE_SCHED_GETCPU 1
#cmakedefine HAVE_CPUID_H 1

#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS
#endif
