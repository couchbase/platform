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

  cb_add_test_executable(platform-jemalloc_thread_shutdown_test
                         jemalloc_thread_shutdown_test.cc)
  target_link_libraries(platform-jemalloc_thread_shutdown_test PRIVATE
                        ${MALLOC_LIBRARIES})
  ADD_TEST(platform-jemalloc_thread_shutdown_test
           platform-jemalloc_thread_shutdown_test)
  SET_TESTS_PROPERTIES(platform-jemalloc_thread_shutdown_test PROPERTIES
    # Print some stats and config details. Also disable the built-in
    # junk-filling since we do that ourselves.
    ENVIRONMENT "JE_MALLOC_CONF=confirm_conf:true,abort_conf:true,stats_print:true,junk:false"
    # Lower the timeout since the test hangs on failure on Windows
    TIMEOUT 30)
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
