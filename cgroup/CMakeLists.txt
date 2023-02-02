add_library(cgroup_objs OBJECT
            cgroup_private.cc
            cgroup.cc
            ${Platform_SOURCE_DIR}/include/cgroup/cgroup.h
            cgroup_private.h)
target_include_directories(cgroup_objs SYSTEM PUBLIC
                           ${Platform_SOURCE_DIR}/include)
target_link_libraries(cgroup_objs PRIVATE Boost::filesystem fmt::fmt)
set_property(TARGET cgroup_objs PROPERTY POSITION_INDEPENDENT_CODE 1)
cb_enable_unity_build(cgroup_objs)

add_library(cgroup STATIC
            ${Platform_SOURCE_DIR}/src/loadfile.cc
            ${Platform_SOURCE_DIR}/src/split_string.cc
            $<TARGET_OBJECTS:cgroup_objs>)
target_include_directories(cgroup SYSTEM PUBLIC ${Platform_SOURCE_DIR}/include)
target_link_libraries(cgroup PRIVATE Boost::filesystem fmt::fmt Folly::folly)
cb_enable_unity_build(cgroup)
set_property(TARGET cgroup PROPERTY POSITION_INDEPENDENT_CODE 1)

cb_add_test_executable(cgroup_test cgroup_tests.cc)
target_compile_definitions(cgroup_test PRIVATE SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(cgroup_test
                      platform
                      Folly::folly
                      GTest::gtest
                      GTest::gtest_main)
add_test(cgroup_test cgroup_test)
add_executable(cgroup_trace cgroup_trace.cc)
target_link_libraries(cgroup_trace platform)
