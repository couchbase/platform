ADD_EXECUTABLE(platform-corestore-test
               corestore_test.cc
               corestore_test.h)
TARGET_LINK_LIBRARIES(platform-corestore-test platform  gtest gtest_main)

ADD_TEST(platform-corestore-test platform-corestore-test)
