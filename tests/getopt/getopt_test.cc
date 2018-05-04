/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <iostream>
#include <cstdlib>
#include <string.h>
#include <platform/getopt.h>
#include <vector>
#include <string>
#include <platform/cbassert.h>
#include <platform/cb_malloc.h>

char **vec2array(const std::vector<std::string> &vec) {
    char **arr = new char*[vec.size()];
    for (unsigned int ii = 0; ii < (unsigned int)vec.size(); ++ii) {
        arr[ii] = cb_strdup(vec[ii].c_str());
    }
    return arr;
}

static void release(char **arr, size_t size) {
    for (size_t ii = 0; ii < size; ++ii) {
        cb_free(arr[ii]);
    }
    delete []arr;
}

typedef std::vector<std::string> getoptvec;

static void getopt_test_0(void) {
    getoptvec vec;
    vec.push_back("program");
    vec.push_back("-a");
    vec.push_back("-b");
    int argc = (int)vec.size();
    char **argv = vec2array(vec);

    cb_assert(cb::getopt::optind == 1);
    cb_assert('a' == cb::getopt::getopt(argc, argv, "a"));
    cb_assert(cb::getopt::optind == 2);
    cb_assert('?' == cb::getopt::getopt(argc, argv, "a"));
    cb_assert(cb::getopt::optind == 3);

    release(argv, vec.size());
}

static void getopt_test_1(void) {
    getoptvec vec;
    vec.push_back("program");
    vec.push_back("-a");
    vec.push_back("--");
    vec.push_back("-b");
    int argc = (int)vec.size();
    char **argv = vec2array(vec);
    cb_assert('a' == cb::getopt::getopt(argc, argv, "a"));
    cb_assert(-1 == cb::getopt::getopt(argc, argv, "a"));
    cb_assert(cb::getopt::optind == 3);

    release(argv, vec.size());
}

static void getopt_test_2(void) {
    getoptvec vec;

    vec.push_back("..\\memcached\\engine_testapp");
    vec.push_back("-E");
    vec.push_back("ep.dll");
    vec.push_back("-T");
    vec.push_back("ep_testsuite.dll");
    vec.push_back("-e");
    vec.push_back("flushall_enabled=true;ht_size=13;ht_locks=7");
    vec.push_back("-v");
    vec.push_back("-C");
    vec.push_back("7");
    vec.push_back("-s");
    vec.push_back("foo");

    int argc = (int)vec.size();
    char **argv = vec2array(vec);

    cb_assert('E' == cb::getopt::getopt(argc, argv, "E:T:e:vC:s"));
    cb_assert(strcmp(argv[2], cb::getopt::optarg) == 0);
    cb_assert('T' == cb::getopt::getopt(argc, argv, "E:T:e:vC:s"));
    cb_assert(strcmp(argv[4], cb::getopt::optarg) == 0);
    cb_assert('e' == cb::getopt::getopt(argc, argv, "E:T:e:vC:s"));
    cb_assert(strcmp(argv[6], cb::getopt::optarg) == 0);
    cb_assert('v' == cb::getopt::getopt(argc, argv, "E:T:e:vC:s"));
    cb_assert('C' == cb::getopt::getopt(argc, argv, "E:T:e:vC:s"));
    cb_assert(strcmp(argv[9], cb::getopt::optarg) == 0);
    cb_assert('s' == cb::getopt::getopt(argc, argv, "E:T:e:vC:s"));
    cb_assert(-1 == cb::getopt::getopt(argc, argv, "E:T:e:vC:s"));
    cb_assert(cb::getopt::optind == 11);

    release(argv, vec.size());
}

static void getopt_long_test(void) {
    static cb::getopt::option long_options[] =
    {
        {"first",  cb::getopt::no_argument, 0, 'f'},
        {"second", cb::getopt::no_argument, 0, 's'},
        {"third",  cb::getopt::no_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    getoptvec vec;

    vec.push_back("getopt_long_test");
    vec.push_back("--first");
    vec.push_back("--wrong");
    vec.push_back("--second");
    vec.push_back("--third");

    int argc = (int)vec.size();
    char **argv = vec2array(vec);

    int option_index = 0;
    int c = 1;

    bool first, second, third;
    first = second = third = false;

    while ( (c = cb::getopt::getopt_long(argc, argv, "fst",
                             long_options, &option_index)) != -1 )  {
        switch (c) {
        case 'f':
            first = true;
            break;
        case 's':
            second = true;
            break;
        case 't':
            third = true;
            break;
        }
    }
    cb_assert(first);
    cb_assert(second);
    cb_assert(third);

    release(argv, vec.size());
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " [testcase]" << std::endl;
        return 1;
    }

    switch (atoi(argv[1])) {
    case 0:
        getopt_test_0();
        break;
    case 1:
        getopt_test_1();
        break;
    case 2:
        getopt_test_2();
        break;
    case 3:
        getopt_long_test();
        break;
    default:
        std::cerr << "Unknown test case" << std::endl;
        return 1;
    }

    return 0;
}
