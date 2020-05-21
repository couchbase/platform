ADD_EXECUTABLE(platform-arena_malloc-test arena_malloc_test.cc)
TARGET_LINK_LIBRARIES(platform-arena_malloc-test PRIVATE
        gtest
        gtest_main
        platform
        platform_cb_malloc_arena
        ${MALLOC_LIBRARIES})
ADD_TEST(platform-arena_malloc-test platform-arena_malloc-test)

if (MEMORY_ALLOCATOR STREQUAL "jemalloc")
  ADD_EXECUTABLE(platform-arena_tracking_bench
                 arena_tracking_bench.cc)

  TARGET_LINK_LIBRARIES(platform-arena_tracking_bench PRIVATE
                        benchmark
                        gtest
                        platform
                        platform_cb_malloc_arena)

  TARGET_INCLUDE_DIRECTORIES(platform-arena_tracking_bench PUBLIC
                             ${benchmark_SOURCE_DIR}/include )
endif()

if (UNIX)
ADD_EXECUTABLE(platform-cb_malloc-test cb_malloc_default.cc)
# This test must not link against platform_cb_malloc
TARGET_LINK_LIBRARIES(platform-cb_malloc-test PRIVATE
        gtest
        gtest_main
        platform
        ${MALLOC_LIBRARIES})
ADD_TEST(platform-cb_malloc-test platform-cb_malloc-test)
endif()