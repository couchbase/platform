/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
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
