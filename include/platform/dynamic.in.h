#pragma once

#cmakedefine CB_DONT_NEED_BYTEORDER 1

/* Function attribute to be used for printf-style format checking
 */
#cmakedefine HAVE_ATTR_FORMAT
#if defined(HAVE_ATTR_FORMAT)
#define CB_FORMAT_PRINTF(FMT_IDX, STR_IDX) __attribute__((format(printf, FMT_IDX, STR_IDX)))
#else
#define CB_FORMAT_PRINTF(FMT_IDX, STR_IDX)
#endif
