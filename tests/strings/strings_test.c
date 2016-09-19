#include <stdio.h>
#include <stdlib.h> /* for free */
#include <string.h> /* for strcmp */
#include <strings.h>

#include <platform/cbassert.h>
#include <platform/cb_malloc.h>

static void test_asprintf(void) {
    char *result = 0;
    cb_assert(asprintf(&result, "test 1") > 0);
    cb_assert(strcmp(result, "test 1") == 0);
    free(result);

    cb_assert(asprintf(&result, "test %d", 2) > 0);
    cb_assert(strcmp(result, "test 2") == 0);
    free(result);

    cb_assert(asprintf(&result, "%c%c%c%c %d", 't', 'e', 's', 't', 3) > 0);
    cb_assert(strcmp(result, "test 3") == 0);
    free(result);
}

int main(void) {
    test_asprintf();
    return 0;
}
