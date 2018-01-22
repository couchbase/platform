ADD_EXECUTABLE(platform-corestore-test
               ${Platform_SOURCE_DIR}/include/platform/platform.h
               corestore_test.cc)
TARGET_LINK_LIBRARIES(platform-corestore-test platform  gtest gtest_main)

ADD_TEST(platform-corestore-test platform-corestore-test)
