ADD_EXECUTABLE(platform-mktemp-test
               ${Platform_SOURCE_DIR}/include/platform/platform.h
               cb_mktemp_test.c)
TARGET_LINK_LIBRARIES(platform-mktemp-test platform)

ADD_TEST(platform-mktemp-test platform-mktemp-test)
