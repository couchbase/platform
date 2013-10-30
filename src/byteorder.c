/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "config.h"

#if !defined(HAVE_NTOHLL) || !defined(HAVE_NTOHLL)
static uint64_t swap64(uint64_t in) {
#ifndef WORDS_BIGENDIAN
    /* Little endian, flip the bytes around until someone makes a faster/better
     * way to do this. */
    uint64_t rv = 0;
    int i = 0;
    for (i = 0; i<8; i++) {
        rv = (rv << 8) | (in & 0xff);
        in >>= 8;
    }
    return rv;
#else
    /* big-endian machines don't need byte swapping */
    return in;
#endif
}
#endif

#ifndef HAVE_NTOHLL
PLATFORM_PUBLIC_API
uint64_t ntohll(uint64_t val) {
    return swap64(val);
}
#endif

#ifndef HAVE_HTONLL
PLATFORM_PUBLIC_API
uint64_t htonll(uint64_t val) {
    return swap64(val);
}
#endif
