add_library(cbcrypto STATIC
        common.cc
        digest.cc
        random_gen.cc
        symmetric.cc
        ${Platform_SOURCE_DIR}/include/cbcrypto/common.h
        ${Platform_SOURCE_DIR}/include/cbcrypto/digest.h
        ${Platform_SOURCE_DIR}/include/cbcrypto/random_gen.h
        ${Platform_SOURCE_DIR}/include/cbcrypto/symmetric.h)
set_property(TARGET cbcrypto PROPERTY POSITION_INDEPENDENT_CODE 1)
platform_enable_pch(cbcrypto)
target_link_libraries(cbcrypto PRIVATE
                      OpenSSL::SSL nlohmann_json::nlohmann_json platform phosphor libsodium::libsodium fmt::fmt)

cb_add_test_executable(cbcrypto_unit_test cbcrypto_test.cc)
platform_enable_pch(cbcrypto_unit_test)
target_link_libraries(cbcrypto_unit_test
                     PRIVATE
                     cbcrypto
                     GTest::gtest
                     GTest::gtest_main
                     platform)
add_test(cbcrypto_unit_test cbcrypto_unit_test)
