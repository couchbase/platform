IF (NOT WIN32)
    SET_SOURCE_FILES_PROPERTIES(${Platform_SOURCE_DIR}/src/crc32c_sse4_2.cc
                                PROPERTIES COMPILE_FLAGS -msse4.2)
ENDIF(NOT WIN32)

ADD_EXECUTABLE(platform-crc32c-test crc32c_test.cc)
TARGET_LINK_LIBRARIES(platform-crc32c-test platform)

ADD_EXECUTABLE(platform-crc32c-sw_hw-test
               ${Platform_SOURCE_DIR}/src/crc32c.cc
               ${Platform_SOURCE_DIR}/src/crc32c_sse4_2.cc
               crc32c_test.cc)
SET_TARGET_PROPERTIES(platform-crc32c-sw_hw-test PROPERTIES COMPILE_FLAGS "-DCRC32C_UNIT_TEST")
TARGET_LINK_LIBRARIES(platform-crc32c-sw_hw-test ${PLATFORM_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(platform-crc32c-sw_hw-test PRIVATE
        ${Platform_SOURCE_DIR}/include
        ${FOLLY_INCLUDE_DIR})

ADD_EXECUTABLE(platform-crc32c-bench
               ${Platform_SOURCE_DIR}/src/crc32c.cc
               ${Platform_SOURCE_DIR}/src/crc32c_sse4_2.cc
               crc32c_bench.cc)
SET_TARGET_PROPERTIES(platform-crc32c-bench PROPERTIES COMPILE_FLAGS "-DCRC32C_UNIT_TEST")
TARGET_LINK_LIBRARIES(platform-crc32c-bench platform)

ADD_TEST(platform-crc32c-test platform-crc32c-test)
ADD_TEST(platform-crc32c-sw_hw-test platform-crc32c-sw_hw-test)
