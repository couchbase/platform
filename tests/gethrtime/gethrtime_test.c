#include <stdio.h>
#include <stdlib.h>

#include <platform/platform.h>

int main(int argc, char **argv)
{
    hrtime_t start = gethrtime();
    hrtime_t end;
    int ii = 0;
    int max = 2000000;

    if (argc > 1) {
        if ((max = atoi(argv[1])) == 0) {
            fprintf(stderr, "Invalid max count\n");
            return 1;
        }
    }

    for (ii = 0; ii < max; ++ii) {
        gethrtime();
    }

    end = gethrtime();
    fprintf(stdout, "Running %u iteratons gave an average of %luns\n",
            max, (unsigned long)((end - start) / max));
    return 0;
}
