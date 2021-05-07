#pragma once

#cmakedefine CB_DONT_NEED_BYTEORDER 1

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

/**
 * Function attribute to be used to specify that a function won't return (like
 * exit/abort etc)
 */
#cmakedefine HAVE_ATTR_NORETURN
#if defined(HAVE_ATTR_NORETURN)
#define CB_ATTR_NORETURN __attribute__((noreturn))
#else
#define CB_ATTR_NORETURN
#endif

/**
 * Function attribute to specify that the given parameters MUST be set to
 * a non-null value.
 */
#cmakedefine HAVE_ATTR_NONNULL
#if defined(HAVE_ATTR_NONNULL)
#define CB_ATTR_NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
#else
#define CB_ATTR_NONNULL(...)
#endif

/**
 * Function attribute to specify that the function is deprecated
 */
#cmakedefine HAVE_ATTR_DEPRECATED
#if defined(HAVE_ATTR_DEPRECATED)
#define CB_ATTR_DEPRECATED __attribute__((deprecated))
#else
#define CB_ATTR_DEPRECATED
#endif
