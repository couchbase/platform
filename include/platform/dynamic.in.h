/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#pragma once

#cmakedefine CB_DONT_NEED_BYTEORDER 1
#cmakedefine CB_DONT_NEED_GETHRTIME 1

/* Function attribute to be used where the return value should not be
 * ignored by the caller. For example: allocation functions where the
 * caller should free the result after use.
 */
#cmakedefine HAVE_ATTR_WARN_UNUSED_RESULT
#if defined(HAVE_ATTR_WARN_UNUSED_RESULT)
#define CB_MUST_USE_RESULT __attribute__((warn_unused_result))
#else
#define CB_MUST_USE_RESULT
#endif

/* Function attribute to be used for printf-style format checking
 */
#cmakedefine HAVE_ATTR_FORMAT
#if defined(HAVE_ATTR_FORMAT)
#define CB_FORMAT_PRINTF(FMT_IDX, STR_IDX) __attribute__((format(printf, FMT_IDX, STR_IDX)))
#else
#define CB_FORMAT_PRINTF(FMT_IDX, STR_IDX)
#endif
