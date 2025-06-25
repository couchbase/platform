cb_add_test_executable(process_monitor_child process_monitor_child.cc)
target_link_libraries(process_monitor_child PRIVATE platform)

cb_add_test_executable(platform_unit_tests
                       atomic_duration_test.cc
                       backtrace_test.cc
                       base64_test.cc
                       bifurcated_counter_test.cc
                       bitset_test.cc
                       byte_literals_test.cc
                       cb_getopt_test.cc
                       checked_snprintf_test.cc
                       command_line_options_parser_test.cc
                       corestore_test.cc
                       corestore_test.h
                       dirutils_test.cc
                       dirutils_test_2.cc
                       enum_class_bitmask_functions_test.cc
                       file_sink_tests.cc
                       getopt_test.cc
                       guarded_test.cc
                       hex_test.cc
                       json_checker_test.cc
                       json_log_test.cc
                       monotonic_test.cc
                       n_byte_integer_test.cc
                       non_blocking_mutex_test.cc
                       non_negative_counter_test.cc
                       optional_test.cc
                       ordered_map_test.cc
                       process_monitor_test.cc
                       processclock_test.cc
                       random_test.cc
                       relaxed_atomic_test.cc
                       save_file_test.cc
                       scope_timer_test.cc
                       semaphore_test.cc
                       simd_scan_test.cc
                       split_string_test.cc
                       static_clock_test.cc
                       string_utilities_test.cc
                       sysinfo_test.cc
                       thread_local_monotonic_resource_test.cc
                       thread_test.cc
                       timeutils_test.cc
                       unshared_test.cc
                       uuid_test.cc
                       )

# cb_getopt_test includes platform/getopt.h which is in conflict with
# <getopt.h> as the system getopt.h defines macros for something
# platform/getopt.h defines as const int ..
set_source_files_properties(cb_getopt_test.cc
        PROPERTIES SKIP_UNITY_BUILD_INCLUSION 1)
target_link_libraries(platform_unit_tests
        PRIVATE fmt::fmt GTest::gtest GTest::gtest_main platform platform_cb_malloc_arena JSON_checker)
platform_enable_pch(platform_unit_tests)
cb_enable_unity_build(platform_unit_tests)
add_test(platform_unit_tests platform_unit_tests)
add_dependencies(platform_unit_tests process_monitor_child)
