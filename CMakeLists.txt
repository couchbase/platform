PROJECT(Platform)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

IF (${CMAKE_MAJOR_VERSION} GREATER 2)
    CMAKE_POLICY(SET CMP0042 NEW)
ENDIF (${CMAKE_MAJOR_VERSION} GREATER 2)

INCLUDE(CheckIncludeFileCXX)
INCLUDE(CheckSymbolExists)
INCLUDE(CTest)

ENABLE_CODE_COVERAGE_REPORT()

CMAKE_PUSH_CHECK_STATE(RESET)
  FIND_LIBRARY(EXECINFO_LIBRARY NAMES execinfo)
  IF (EXECINFO_LIBRARY)
    SET(CMAKE_REQUIRED_LIBRARIES "${EXECINFO_LIBRARY}")
    LIST(APPEND PLATFORM_LIBRARIES "${EXECINFO_LIBRARY}")
  ENDIF(EXECINFO_LIBRARY)
  CHECK_SYMBOL_EXISTS(backtrace execinfo.h HAVE_BACKTRACE)
CMAKE_POP_CHECK_STATE()

CMAKE_PUSH_CHECK_STATE(RESET)
  SET(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
  FIND_LIBRARY(DL_LIBRARY NAMES dl)
  IF (DL_LIBRARY)
    SET(CMAKE_REQUIRED_LIBRARIES "${DL_LIBRARY}")
    LIST(APPEND PLATFORM_LIBRARIES "${DL_LIBRARY}")
  ENDIF (DL_LIBRARY)
  CHECK_SYMBOL_EXISTS(dladdr dlfcn.h HAVE_DLADDR)
CMAKE_POP_CHECK_STATE()

CMAKE_PUSH_CHECK_STATE(RESET)
SET(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
SET(CMAKE_REQUIRED_LIBRARIES "pthread")
CHECK_SYMBOL_EXISTS(pthread_setname_np pthread.h HAVE_PTHREAD_SETNAME_NP)
IF (HAVE_PTHREAD_SETNAME_NP)
  MESSAGE(STATUS "pthread_setname_np is available")
ELSE (HAVE_PTHREAD_SETNAME_NP)
  MESSAGE(STATUS "pthread_setname_np is not available")
ENDIF (HAVE_PTHREAD_SETNAME_NP)
CMAKE_POP_CHECK_STATE()


CHECK_SYMBOL_EXISTS(gethrtime sys/time.h CB_DONT_NEED_GETHRTIME)
CHECK_SYMBOL_EXISTS(htonll arpa/inet.h CB_DONT_NEED_BYTEORDER)

CONFIGURE_FILE (${CMAKE_CURRENT_SOURCE_DIR}/src/config.cmake.h
                ${CMAKE_CURRENT_BINARY_DIR}/src/config.h)

CONFIGURE_FILE (${CMAKE_CURRENT_SOURCE_DIR}/include/platform/dynamic.in.h
                ${CMAKE_CURRENT_BINARY_DIR}/include/platform/dynamic.h)

#
# Set the include path
#
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/include
                           ${CMAKE_CURRENT_BINARY_DIR}/src)

ADD_LIBRARY(cJSON SHARED src/cJSON.c include/cJSON.h)
SET_TARGET_PROPERTIES(cJSON PROPERTIES SOVERSION 1.0.0)
SET_TARGET_PROPERTIES(cJSON PROPERTIES COMPILE_FLAGS -DBUILDING_CJSON=1)

ADD_LIBRARY(JSON_checker SHARED src/JSON_checker.cc include/JSON_checker.h)
SET_TARGET_PROPERTIES(JSON_checker PROPERTIES SOVERSION 1.0.0)

IF (WIN32)
   INCLUDE_DIRECTORIES(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/include/win32)
   ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
   SET(PLATFORM_FILES src/getopt.c
                      src/sockets.c
                      src/strings.c
                      src/cb_win32.cc
                      src/winrandom.c
                      src/memorymap_win32.cc
                      include/win32/getopt.h
                      include/win32/strings.h
                      include/win32/unistd.h)
   INCLUDE(FindCouchbaseDbgHelp)
   LIST(APPEND PLATFORM_LIBRARIES "${DBGHELP_LIBRARY}")
   INSTALL(FILES ${DBGHELP_DLL} DESTINATION bin)
ELSE (WIN32)
   SET(PLATFORM_FILES src/cb_pthreads.c src/urandom.c src/memorymap_posix.cc)
   SET_SOURCE_FILES_PROPERTIES(src/crc32c_sse4_2.cc PROPERTIES COMPILE_FLAGS -msse4.2)
   LIST(APPEND PLATFORM_LIBRARIES "pthread")

   IF (NOT APPLE)
      LIST(APPEND PLATFORM_LIBRARIES "rt")
   ENDIF(NOT APPLE)

   TARGET_LINK_LIBRARIES(cJSON m)
ENDIF (WIN32)

IF (NOT DEFINED COUCHBASE_NETWORK_LIBS)
   IF (WIN32)
      SET(COUCHBASE_NETWORK_LIBS "Ws2_32")
   ELSEIF ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
      SET(COUCHBASE_NETWORK_LIBS socket nsl)
   ENDIF (WIN32)
ENDIF (NOT DEFINED COUCHBASE_NETWORK_LIBS)

IF ("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD")
    # The implementation you get with chrono on a default FreeBSD 10
    # is dead slow and considered useless...
    SET(HAVE_DEAD_SLOW_CHRONO true)
ENDIF ("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD")

IF (CB_DONT_NEED_GETHRTIME OR HAVE_DEAD_SLOW_CHRONO)
   SET(PLATFORM_FILES ${PLATFORM_FILES} src/gethrtime.c)
ELSE (CB_DONT_NEED_GETHRTIME OR HAVE_DEAD_SLOW_CHRONO)
   CHECK_INCLUDE_FILE_CXX(chrono HAVE_CHRONO)
   IF (NOT HAVE_CHRONO)
      MESSAGE(FATAL_ERROR "You need chrono to build a high resolution timer")
   ENDIF (NOT HAVE_CHRONO)
   SET(PLATFORM_FILES ${PLATFORM_FILES} src/cxxtime.cc)
ENDIF (CB_DONT_NEED_GETHRTIME OR HAVE_DEAD_SLOW_CHRONO)

#
# Add all of the libraries
#
ADD_LIBRARY(platform SHARED ${PLATFORM_FILES}
                            ${CMAKE_CURRENT_BINARY_DIR}/src/config.h
                            src/getpid.c
                            src/random.cc
                            src/backtrace.c
                            src/byteorder.c
                            src/cb_time.c
                            src/cb_mktemp.c
                            src/cbassert.c
                            src/crc32c.cc
                            src/crc32c_sse4_2.cc
                            src/crc32c_private.h
                            src/strerror.cc
                            include/platform/crc32c.h
                            include/platform/memorymap.h
                            include/platform/platform.h
                            include/platform/random.h
                            include/platform/strerror.h
                            include/platform/visibility.h)


LIST(REMOVE_DUPLICATES PLATFORM_LIBRARIES)
TARGET_LINK_LIBRARIES(platform ${COUCHBASE_NETWORK_LIBS} ${PLATFORM_LIBRARIES})
SET_TARGET_PROPERTIES(platform PROPERTIES SOVERSION 0.1.0)

ADD_LIBRARY(dirutils SHARED src/dirutils.cc include/platform/dirutils.h)
SET_TARGET_PROPERTIES(dirutils PROPERTIES SOVERSION 0.1.0)

IF (BREAKPAD_FOUND)
   ADD_DEFINITIONS(-DHAVE_BREAKPAD)
   INCLUDE_DIRECTORIES(AFTER ${BREAKPAD_INCLUDE_DIR})
   # Breakpad has headers (which are dependencies for our breakpad integration
   # code) that don't compile cleanly as C++11, so the GNU standard is
   # used instead.
   IF (UNIX)
      set_source_files_properties(src/breakpad_wrapper.cc PROPERTIES COMPILE_FLAGS ${CB_GNU_CXX11_OPTION})
   ENDIF (UNIX)

   ADD_LIBRARY(breakpad_wrapper SHARED
               src/breakpad_wrapper.cc
               include/breakpad_wrapper/breakpad_wrapper.h)

   TARGET_LINK_LIBRARIES(breakpad_wrapper
                         ${BREAKPAD_LIBRARIES})

   INSTALL(TARGETS breakpad_wrapper
           RUNTIME DESTINATION bin
           LIBRARY DESTINATION lib
           ARCHIVE DESTINATION lib)
ENDIF (BREAKPAD_FOUND)

ADD_EXECUTABLE(platform-dirutils-test tests/dirutils_test.cc)
TARGET_LINK_LIBRARIES(platform-dirutils-test dirutils)

ADD_EXECUTABLE(platform-gettimeofday-test tests/gettimeofday_test.cc)
TARGET_LINK_LIBRARIES(platform-gettimeofday-test platform)

ADD_EXECUTABLE(platform-crc32c-test tests/crc32c_test.cc)
TARGET_LINK_LIBRARIES(platform-crc32c-test platform)

ADD_EXECUTABLE(platform-crc32c-sw_hw-test tests/crc32c_test.cc
                                          src/crc32c.cc
                                          src/crc32c_sse4_2.cc)
SET_TARGET_PROPERTIES(platform-crc32c-sw_hw-test PROPERTIES COMPILE_FLAGS "-DCRC32C_UNIT_TEST")
TARGET_LINK_LIBRARIES(platform-crc32c-sw_hw-test ${PLATFORM_LIBRARIES})

ADD_EXECUTABLE(platform-crc32c-bench tests/crc32c_bench.cc
                                     src/crc32c.cc
                                     src/crc32c_sse4_2.cc)

SET_TARGET_PROPERTIES(platform-crc32c-bench PROPERTIES COMPILE_FLAGS "-DCRC32C_UNIT_TEST")
TARGET_LINK_LIBRARIES(platform-crc32c-bench platform)

INSTALL(TARGETS cJSON JSON_checker platform dirutils
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

#
# Test targets
#
ADD_EXECUTABLE(platform-gethrtime-test
               tests/gethrtime_test.c
               include/platform/platform.h)
TARGET_LINK_LIBRARIES(platform-gethrtime-test platform)

ADD_EXECUTABLE(platform-getopt-test tests/getopt_test.cc)
TARGET_LINK_LIBRARIES(platform-getopt-test platform)

ADD_EXECUTABLE(platform-random-test
               tests/random_test.cc
               include/platform/random.h)
TARGET_LINK_LIBRARIES(platform-random-test platform)

ADD_EXECUTABLE(platform-mktemp-test
               tests/cb_mktemp_test.c
               include/platform/platform.h)
TARGET_LINK_LIBRARIES(platform-mktemp-test platform)

ADD_TEST(platform-dirutils-test platform-dirutils-test)
ADD_TEST(platform-gettimeofday-test platform-gettimeofday-test)
ADD_TEST(platform-gethrtime-test platform-gethrtime-test)
ADD_TEST(platform-getopt-test-0 platform-getopt-test 0)
ADD_TEST(platform-getopt-test-1 platform-getopt-test 1)
ADD_TEST(platform-getopt-test-2 platform-getopt-test 2)
ADD_TEST(platform-random-test platform-random-test)
ADD_TEST(platform-mktemp-test platform-mktemp-test)
ADD_TEST(platform-crc32c-test platform-crc32c-test)
ADD_TEST(platform-crc32c-sw_hw-test platform-crc32c-sw_hw-test)

ADD_EXECUTABLE(platform-cjson-test include/cJSON.h tests/cjson_test.c)
TARGET_LINK_LIBRARIES(platform-cjson-test cJSON)
ADD_TEST(platform-cjson-test platform-cjson-test)

ADD_EXECUTABLE(platform-cjson-parse-test
               include/cJSON.h
               tests/cjson_parse_test.c)
TARGET_LINK_LIBRARIES(platform-cjson-parse-test cJSON platform)
ADD_TEST(platform-cjson-parse-test
         platform-cjson-parse-test -f ${PROJECT_SOURCE_DIR}/tests/testdata.json
                                   -n 1)
ADD_EXECUTABLE(platform-strings-test tests/strings_test.c)
TARGET_LINK_LIBRARIES(platform-strings-test platform)
ADD_TEST(platform-strings-test platform-strings-test)

ADD_EXECUTABLE(platform-backtrace-test tests/backtrace_test.c)
TARGET_LINK_LIBRARIES(platform-backtrace-test platform)
ADD_TEST(platform-backtrace-test platform-backtrace-test)

ADD_EXECUTABLE(platform-memorymap-test tests/memorymap_test.cc)
TARGET_LINK_LIBRARIES(platform-memorymap-test platform)
ADD_TEST(platform-memorymap-test platform-memorymap-test)

ADD_SUBDIRECTORY(tests)

IF (${CMAKE_MAJOR_VERSION} LESS 3)
   SET_TARGET_PROPERTIES(cJSON PROPERTIES INSTALL_NAME_DIR
                         ${CMAKE_INSTALL_PREFIX}/lib)
   SET_TARGET_PROPERTIES(JSON_checker PROPERTIES INSTALL_NAME_DIR
                         ${CMAKE_INSTALL_PREFIX}/lib)
   SET_TARGET_PROPERTIES(platform PROPERTIES INSTALL_NAME_DIR
                         ${CMAKE_INSTALL_PREFIX}/lib)
   SET_TARGET_PROPERTIES(dirutils PROPERTIES INSTALL_NAME_DIR
                         ${CMAKE_INSTALL_PREFIX}/lib)
ENDIF (${CMAKE_MAJOR_VERSION} LESS 3)
