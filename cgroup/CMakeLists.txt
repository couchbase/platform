add_library(cgroup STATIC
            cgroup_private.cc
            cgroup.cc
            ${Platform_SOURCE_DIR}/include/cgroup/cgroup.h
            cgroup_private.h)
target_include_directories(cgroup SYSTEM PRIVATE ${BOOST_INCLUDE_DIR})
target_include_directories(cgroup SYSTEM PUBLIC ${Platform_SOURCE_DIR}/include)
target_link_libraries(cgroup PRIVATE ${Boost_FILESYSTEM_LIBRARY})
set_property(TARGET cgroup PROPERTY POSITION_INDEPENDENT_CODE 1)
cb_enable_unity_build(cgroup)
cb_add_test_executable(cgroup_test cgroup_tests.cc)
target_link_libraries(cgroup_test cgroup
                      platform
                      Folly::folly
                      GTest::gtest
                      GTest::gtest_main)
add_test(cgroup_test cgroup_test)
