ADD_EXECUTABLE(platform-getopt-test getopt_test.cc)
TARGET_LINK_LIBRARIES(platform-getopt-test platform)

ADD_TEST(platform-getopt-test-0 platform-getopt-test 0)
ADD_TEST(platform-getopt-test-1 platform-getopt-test 1)
ADD_TEST(platform-getopt-test-2 platform-getopt-test 2)
