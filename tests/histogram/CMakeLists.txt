# We use "_USE_MATH_DEFINES" which is some windows shenanigans in
# histogram_test.cc. This creates some macros that are not created if you
# have already included <cmath> already via another file. The precompiled header
# is including <cmath> via something and appears to be included before any other
# includes so the PCH is incompatible with this file on windows.
set_source_files_properties(histogram_test.cc
        PROPERTIES
        SKIP_PRECOMPILE_HEADERS ON)

cb_add_test_executable(platform-histogram-test
               ${Platform_SOURCE_DIR}/include/platform/histogram.h
               ${Platform_SOURCE_DIR}/src/histogram.cc
               histogram_test.cc)
target_link_libraries(platform-histogram-test PRIVATE platform GTest::gtest GTest::gtest_main)
platform_enable_pch(platform-histogram-test)
add_test(platform-histogram-test platform-histogram-test)
