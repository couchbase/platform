# Disable the test for Windows due to the fact that the event is
# sent to the entire process group causing the python interpreter
# to exit with the same error code (keyboard interrupt)
if (NOT WIN32)
   add_executable(platform-interrupt-test
                  ${Platform_SOURCE_DIR}/include/platform/interrupt.h
                  ${Platform_SOURCE_DIR}/include/platform/platform.h
                  interrupt_test.cc)
   target_link_libraries(platform-interrupt-test platform)
   add_test(platform-interrupt-test
            python
            ${Platform_SOURCE_DIR}/tests/interrupt/interrupt_test.py
            ${Platform_BINARY_DIR}/tests/interrupt/platform-interrupt-test)
endif (NOT WIN32)
