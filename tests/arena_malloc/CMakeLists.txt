cb_add_test_executable(platform-arena_malloc-test arena_malloc_test.cc)
TARGET_LINK_LIBRARIES(platform-arena_malloc-test PRIVATE
        GTest::gtest
        GTest::gtest_main
        platform
        platform_cb_malloc_arena
        ${MALLOC_LIBRARIES})
platform_enable_pch(platform-arena_malloc-test)
ADD_TEST(platform-arena_malloc-test platform-arena_malloc-test)

if (MEMORY_ALLOCATOR STREQUAL "jemalloc")
  cb_add_test_executable(platform-arena_tracking_bench
                 arena_tracking_bench.cc)

  TARGET_LINK_LIBRARIES(platform-arena_tracking_bench PRIVATE
                        benchmark::benchmark
                        benchmark::benchmark_main
                        GTest::gtest
                        platform
                        platform_cb_malloc_arena)
  platform_enable_pch(platform-arena_tracking_bench)
endif()

if (UNIX)
  cb_add_test_executable(platform-cb_malloc-test cb_malloc_default.cc)
  # This test must not link against platform_cb_malloc
  TARGET_LINK_LIBRARIES(platform-cb_malloc-test PRIVATE
                        GTest::gtest
                        GTest::gtest_main
                        platform
                        ${MALLOC_LIBRARIES})
  platform_enable_pch(platform-cb_malloc-test)
  ADD_TEST(platform-cb_malloc-test platform-cb_malloc-test)
endif()
