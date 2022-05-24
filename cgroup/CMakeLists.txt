add_library(cgroup_objs OBJECT
            cgroup_private.cc
            cgroup.cc
            ${Platform_SOURCE_DIR}/include/cgroup/cgroup.h
            cgroup_private.h)
target_include_directories(cgroup_objs SYSTEM PRIVATE ${BOOST_INCLUDE_DIR})
target_include_directories(cgroup_objs SYSTEM PUBLIC ${Platform_SOURCE_DIR}/include)
target_link_libraries(cgroup_objs PRIVATE ${Boost_FILESYSTEM_LIBRARY})
set_property(TARGET cgroup_objs PROPERTY POSITION_INDEPENDENT_CODE 1)
cb_enable_unity_build(cgroup_objs)
cb_add_test_executable(cgroup_test cgroup_tests.cc)
target_link_libraries(cgroup_test
                      platform
                      Folly::folly
                      GTest::gtest
                      GTest::gtest_main)
add_test(cgroup_test cgroup_test)
