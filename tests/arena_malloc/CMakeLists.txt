ADD_EXECUTABLE(platform-arena_malloc-test arena_malloc_test.cc)
TARGET_LINK_LIBRARIES(platform-arena_malloc-test PRIVATE
        gtest
        gtest_main
        platform)
ADD_TEST(platform-arena_malloc-test platform-arena_malloc-test)

if (MEMORY_ALLOCATOR STREQUAL "jemalloc")
  ADD_EXECUTABLE(platform-arena_tracking_bench
                 arena_tracking_bench.cc)

  TARGET_LINK_LIBRARIES(platform-arena_tracking_bench PRIVATE
                        benchmark
                        gtest
                        platform)

  TARGET_INCLUDE_DIRECTORIES(platform-arena_tracking_bench PUBLIC
                             ${benchmark_SOURCE_DIR}/include )
endif()