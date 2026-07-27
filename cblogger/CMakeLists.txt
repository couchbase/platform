add_library(cblogger STATIC
        custom_rotating_file_sink.cc
        custom_rotating_file_sink.h
        logger_config.cc
        prefix_logger.cc
        spdlogger.cc
        ${Platform_SOURCE_DIR}/include/cblogger/file_sink_encryption_config.h
        ${Platform_SOURCE_DIR}/include/cblogger/logger.h
        ${Platform_SOURCE_DIR}/include/cblogger/logger_config.h
        ${Platform_SOURCE_DIR}/include/cblogger/logger_iface.h
        ${Platform_SOURCE_DIR}/include/cblogger/logger_level.h
        ${Platform_SOURCE_DIR}/include/cblogger/prefix_logger.h)
set_property(TARGET cblogger PROPERTY POSITION_INDEPENDENT_CODE 1)
platform_enable_pch(cblogger)
cb_enable_unity_build(cblogger)
target_link_libraries(cblogger
        PUBLIC
        cbcrypto
        platform
        spdlog::spdlog
        nlohmann_json::nlohmann_json)
add_sanitizers(cblogger)

add_library(cblogger_test_fixture STATIC
        logger_test_fixture.cc
        logger_test_fixture.h)
set_property(TARGET cblogger_test_fixture PROPERTY POSITION_INDEPENDENT_CODE 1)
platform_enable_pch(cblogger_test_fixture)
target_include_directories(cblogger_test_fixture INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cblogger_test_fixture PUBLIC cblogger GTest::gtest)

cb_add_test_executable(cblogger_unit_test
        logger_config_test.cc
        logger_test.cc)
cb_enable_unity_build(cblogger_unit_test)
platform_enable_pch(cblogger_unit_test)
target_link_libraries(cblogger_unit_test
        PRIVATE
        cblogger
        cblogger_test_fixture
        GTest::gtest
        GTest::gtest_main
        platform)
add_test(NAME cblogger_unit_test
         WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
         COMMAND cblogger_unit_test)

cb_add_test_executable(cblogger_benchmark logger_benchmark.cc)
platform_enable_pch(cblogger_benchmark)
target_link_libraries(cblogger_benchmark PRIVATE benchmark::benchmark cblogger)
add_test(NAME cblogger_benchmark
         WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
         COMMAND cblogger_benchmark --benchmark_min_time=0.0000001s)
