/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdlib.h>
#include <getopt.h>
#include <inttypes.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <platform/platform.h>
#include <cJSON.h>
#include <stdio.h>
#include <errno.h>

static int spool(FILE *fp, char *dest, size_t size)
{
    size_t offset = 0;

    clearerr(fp);
    while (offset < size) {
        offset += fread(dest + offset, 1, size - offset, fp);
        if (ferror(fp)) {
            return -1;
        }
    }

    return 0;
}

static char *load_file(const char *file)
{
    FILE *fp;
    struct stat st;
    char *data;

    if (stat(file, &st) == -1) {
        fprintf(stderr, "Failed to lookup test file %s: %s", file,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    fp = fopen(file, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open test file %s: %s", file,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    data = reinterpret_cast<char*>(malloc(st.st_size + 1));
    if (data == NULL) {
        fclose(fp);
        fprintf(stderr, "Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }

    if (spool(fp, data, st.st_size) == -1) {
        free(data);
        fclose(fp);
        fprintf(stderr, "Failed to open test file %s: %s", file,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    fclose(fp);
    data[st.st_size] = 0;
    return data;
}

static void report(hrtime_t time) {
   const char * const extensions[] = { " ns", " usec", " ms", " s", NULL };
   int id = 0;

   while (time > 9999) {
      ++id;
      time /= 1000;
      if (extensions[id + 1] == NULL) {
         break;
      }
   }

   assert(extensions[id] != NULL);
   fprintf(stderr, "Parsing took an average of %" PRIu64 "%s\n",
           (uint64_t)time, extensions[id]);
}

int main(int argc, char **argv) {
    char *data = NULL;
    const char *fname = "testdata.json";
    int num = 1;
    int cmd;
    int ii;
    hrtime_t start;
    hrtime_t delta;

    while ((cmd = getopt(argc, argv, "f:n:")) != -1) {
        switch (cmd) {
        case 'f' : fname = optarg; break;
        case 'n' : num = atoi(optarg); break;
        default:
            fprintf(stderr, "usage: %s [-f fname] [-n num]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    data = load_file(fname);

    start = gethrtime();
    for (ii = 0; ii < num; ++ii) {
        cJSON *ptr = cJSON_Parse(data);
        assert(ptr != NULL);
        cJSON_Delete(ptr);
    }
    delta = gethrtime() - start;

    report(delta / (hrtime_t)num);

    free(data);
    exit(EXIT_SUCCESS);
}
