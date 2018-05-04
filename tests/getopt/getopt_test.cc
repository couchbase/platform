/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <gtest/gtest.h>
#include <platform/cb_malloc.h>
#include <platform/cbassert.h>
#include <platform/getopt.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

char **vec2array(const std::vector<std::string> &vec) {
    auto** arr = new char*[vec.size()];
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

class GetoptTest : public ::testing::Test {
protected:
    void SetUp() override {
        cb::getopt::reset();
    }
};

TEST_F(GetoptTest, NormalWithOneUnknownProvided) {
    getoptvec vec;
    vec.push_back("program");
    vec.push_back("-a");
    vec.push_back("-b");
    auto argc = (int)vec.size();
    auto** argv = vec2array(vec);
    ASSERT_EQ(1, cb::getopt::optind);
    ASSERT_EQ('a', cb::getopt::getopt(argc, argv, "a"));
    ASSERT_EQ(2, cb::getopt::optind);
    ASSERT_EQ('?', cb::getopt::getopt(argc, argv, "a"));
    ASSERT_EQ(3, cb::getopt::optind);

    release(argv, vec.size());
}

TEST_F(GetoptTest, NormalWithTermination) {
    getoptvec vec;
    vec.push_back("program");
    vec.push_back("-a");
    vec.push_back("--");
    vec.push_back("-b");
    auto argc = (int)vec.size();
    auto** argv = vec2array(vec);
    ASSERT_EQ('a', cb::getopt::getopt(argc, argv, "a"));
    ASSERT_EQ(-1, cb::getopt::getopt(argc, argv, "a"));
    ASSERT_EQ(3, cb::getopt::optind);

    release(argv, vec.size());
}

TEST_F(GetoptTest, RegressionTestFromEpEngine) {
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

    auto argc = (int)vec.size();
    auto** argv = vec2array(vec);

    ASSERT_EQ('E', cb::getopt::getopt(argc, argv, "E:T:e:vC:s"));
    ASSERT_STREQ(argv[2], cb::getopt::optarg);
    ASSERT_EQ('T', cb::getopt::getopt(argc, argv, "E:T:e:vC:s"));
    ASSERT_STREQ(argv[4], cb::getopt::optarg);
    ASSERT_EQ('e', cb::getopt::getopt(argc, argv, "E:T:e:vC:s"));
    ASSERT_STREQ(argv[6], cb::getopt::optarg);
    ASSERT_EQ('v', cb::getopt::getopt(argc, argv, "E:T:e:vC:s"));
    ASSERT_EQ('C', cb::getopt::getopt(argc, argv, "E:T:e:vC:s"));
    ASSERT_STREQ(argv[9], cb::getopt::optarg);
    ASSERT_EQ('s', cb::getopt::getopt(argc, argv, "E:T:e:vC:s"));
    ASSERT_EQ(-1, cb::getopt::getopt(argc, argv, "E:T:e:vC:s"));
    ASSERT_EQ(11, cb::getopt::optind);

    release(argv, vec.size());
}

TEST_F(GetoptTest, TestLongOptions) {
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

    auto argc = (int)vec.size();
    auto** argv = vec2array(vec);

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
    EXPECT_TRUE(first) << "--first not found";
    EXPECT_TRUE(second) << "--second not found";
    EXPECT_TRUE(third) << "--third not found";

    release(argv, vec.size());
}
