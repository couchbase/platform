ADD_EXECUTABLE(platform-getopt-test getopt_test.cc)
TARGET_LINK_LIBRARIES(platform-getopt-test platform gtest gtest_main)

ADD_TEST(platform-getopt-test platform-getopt-test)
