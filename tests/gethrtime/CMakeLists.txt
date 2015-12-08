ADD_EXECUTABLE(platform-gethrtime-test
               ${Platform_SOURCE_DIR}/include/platform/platform.h
               gethrtime_test.c)
TARGET_LINK_LIBRARIES(platform-gethrtime-test platform)

ADD_TEST(platform-gethrtime-test platform-gethrtime-test)
