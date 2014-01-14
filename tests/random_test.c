#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <platform/random.h>

int main(int argc, char **argv)
{
    cb_rand_t r;
    char buffer[1024];
    int ii;

    if (cb_rand_open(&r) == -1) {
        fprintf(stderr, "Failed to initialize random generator\n");
        return -1;
    }
    memset(buffer, 0, sizeof(buffer));
    if (cb_rand_get(r, buffer, sizeof(buffer)) == -1) {
        fprintf(stderr, "Failed to read random bytes\n");
        cb_rand_close(r);
        return -1;
    }

    /* In theory it may return 1k of 0 so we may have a false positive */
    for (ii = 0; ii < sizeof(buffer); ++ii) {
        if (buffer[ii] != 0) {
            break;
        }
    }
    if (ii == sizeof(buffer)) {
        fprintf(stderr, "I got 1k of 0 (or it isn't working)\n");
        cb_rand_close(r);
        return -1;
    }

    if (cb_rand_close(r) == -1) {
        fprintf(stderr, "rand close failed\n");
        return -1;
    }

    return 0;
}
