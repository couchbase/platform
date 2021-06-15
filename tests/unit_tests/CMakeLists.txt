cb_add_test_executable(process_monitor_child process_monitor_child.cc)
target_link_libraries(process_monitor_child platform)

cb_add_test_executable(platform_unit_tests
                       atomic_duration_test.cc
                       backtrace_test.cc
                       base64_test.cc
                       bitset_test.cc
                       cb_getopt_test.cc
                       checked_snprintf_test.cc
                       corestore_test.cc
                       corestore_test.h
                       dirutils_test.cc
                       dirutils_test_2.cc
                       getopt_test.cc
                       hex_test.cc
                       json_checker_test.cc
                       n_byte_integer_test.cc
                       non_negative_counter_test.cc
                       optional_test.cc
                       process_monitor_test.cc
                       processclock_test.cc
                       relaxed_atomic_test.cc
                       ring_buffer_test.cc
                       scope_timer_test.cc
                       semaphore_test.cc
                       sysinfo_test.cc
                       thread_test.cc
                       timeutils_test.cc
                       uuid_test.cc
                       )
target_link_libraries(platform_unit_tests gtest gtest_main platform JSON_checker)
platform_enable_pch(platform_unit_tests)
cb_enable_unity_build(platform_unit_tests)
add_test(platform_unit_tests platform_unit_tests)
add_dependencies(platform_unit_tests process_monitor_child)
