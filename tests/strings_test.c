#include <stdio.h>
#include <stdlib.h> /* for free */
#include <string.h> /* for strcmp */
#include <strings.h>

#include <platform/cbassert.h>

static void test_asprintf(void) {
    char *result = 0;
    (void)asprintf(&result, "test 1");
    cb_assert(strcmp(result, "test 1") == 0);
    free(result);

    (void)asprintf(&result, "test %d", 2);
    cb_assert(strcmp(result, "test 2") == 0);
    free(result);

    (void)asprintf(&result, "%c%c%c%c %d", 't', 'e', 's', 't', 3);
    cb_assert(strcmp(result, "test 3") == 0);
    free(result);
}

int main(void) {
    test_asprintf();
    return 0;
}
