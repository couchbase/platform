CMAKE_MINIMUM_REQUIRED(VERSION 3.2)
PROJECT(Platform LANGUAGES C CXX)

INCLUDE(CheckFunctionExists)
INCLUDE(CheckIncludeFile)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(CheckSymbolExists)
INCLUDE(CTest)
INCLUDE(GenerateExportHeader)
INCLUDE(check_unit_test_enabled)

OPTION(PLATFORM_ENABLE_CBSOCKET "Build cbsocket support" ON)

check_unit_test_enabled(platform COUCHBASE_PLATFORM_BUILD_UNIT_TESTS)
if (COUCHBASE_PLATFORM_BUILD_UNIT_TESTS)
    ENABLE_CODE_COVERAGE_REPORT()
endif (COUCHBASE_PLATFORM_BUILD_UNIT_TESTS)

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
CHECK_SYMBOL_EXISTS(pthread_getname_np pthread.h HAVE_PTHREAD_GETNAME_NP)
CHECK_SYMBOL_EXISTS(sched_getaffinity sched.h HAVE_SCHED_GETAFFINITY)
CHECK_SYMBOL_EXISTS(sched_getcpu sched.h HAVE_SCHED_GETCPU)
CMAKE_POP_CHECK_STATE()

CMAKE_PUSH_CHECK_STATE(RESET)

CHECK_SYMBOL_EXISTS(htonll arpa/inet.h CB_DONT_NEED_BYTEORDER)
CHECK_INCLUDE_FILE(cpuid.h HAVE_CPUID_H)

if (HAVE_BACKTRACE)
  add_definitions(-DHAVE_BACKTRACE=1)
endif()

if (HAVE_DLADDR)
  add_definitions(-DHAVE_DLADDR=1)
endif()

if (HAVE_PTHREAD_SETNAME_NP)
  add_definitions(-DHAVE_PTHREAD_SETNAME_NP=1)
endif()

if (HAVE_PTHREAD_GETNAME_NP)
  add_definitions(-DHAVE_PTHREAD_GETNAME_NP=1)
endif()

if (HAVE_SCHED_GETAFFINITY)
  add_definitions(-DHAVE_SCHED_GETAFFINITY=1)
endif()

if (HAVE_SCHED_GETCPU)
  add_definitions(-DHAVE_SCHED_GETCPU=1)
endif()

if (HAVE_CPUID_H)
  add_definitions(-DHAVE_CPUID_H=1)
endif()

CONFIGURE_FILE (${CMAKE_CURRENT_SOURCE_DIR}/include/platform/dynamic.in.h
                ${CMAKE_CURRENT_BINARY_DIR}/include/platform/dynamic.h)

#
# Set the include path
#
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/external)

IF (MEMORY_ALLOCATOR)
  INCLUDE_DIRECTORIES(AFTER ${MALLOC_INCLUDE_DIR})
ENDIF (MEMORY_ALLOCATOR)

ADD_LIBRARY(JSON_checker SHARED src/JSON_checker.cc include/JSON_checker.h)
SET_TARGET_PROPERTIES(JSON_checker PROPERTIES SOVERSION 1.0.0)
TARGET_LINK_LIBRARIES(JSON_checker PRIVATE platform_headers)
GENERATE_EXPORT_HEADER(JSON_checker
                       EXPORT_MACRO_NAME JSON_CHECKER_PUBLIC_API
                       EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/platform/JSON_checker-visibility.h)

IF (WIN32)
   INCLUDE_DIRECTORIES(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/include/win32)
   ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
   SET(PLATFORM_FILES src/sockets.c
                      src/cb_win32.cc
                      src/crc32c_sse4_2.cc
                      include/win32/getopt.h
                      include/win32/strings.h
                      include/win32/unistd.h)
   ADD_DEFINITIONS(-DHAVE_BACKTRACE_SUPPORT=1)
ELSE (WIN32)
   SET(PLATFORM_FILES src/cb_pthreads.cc)
   # We don't know the output of uname -p on all of our supported platforms
   # so check if it's PPC64
   if (NOT CMAKE_SYSTEM_PROCESSOR EQUAL "^ppc64")
      list(APPEND PLATFORM_FILES src/crc32c_sse4_2.cc)
      set_source_files_properties(src/crc32c_sse4_2.cc
                                  PROPERTIES COMPILE_FLAGS -msse4.2)
   endif()
   LIST(APPEND PLATFORM_LIBRARIES "pthread")

   IF (NOT APPLE)
      LIST(APPEND PLATFORM_LIBRARIES "rt")
   ENDIF(NOT APPLE)

   SET(LIB_M m)
   LIST(REMOVE_DUPLICATES PLATFORM_LIBRARIES)
ENDIF (WIN32)

IF (NOT DEFINED COUCHBASE_NETWORK_LIBS)
   IF (WIN32)
      SET(COUCHBASE_NETWORK_LIBS "Ws2_32")
   ENDIF (WIN32)
ENDIF (NOT DEFINED COUCHBASE_NETWORK_LIBS)

CHECK_FUNCTION_EXISTS(malloc_usable_size HAVE_MALLOC_USABLE_SIZE)
if(HAVE_MALLOC_USABLE_SIZE)
  ADD_DEFINITIONS(-DHAVE_MALLOC_USABLE_SIZE)
endif()
#
# Add all of the libraries
#

# 'platform_headers' - header-only part of platform.
#
# This should typically only be used for tests / benchmarks which want the
# definitions of platform functions; but don't want to link against the
# complete library. For example, a unit test might want to just link against
# the single .cc file for the code it is testing, but needs the prototype(s)
# of that .cc file.
ADD_LIBRARY(platform_headers INTERFACE)
TARGET_INCLUDE_DIRECTORIES(platform_headers SYSTEM INTERFACE
                           ${FOLLY_INCLUDE_DIR})

TARGET_INCLUDE_DIRECTORIES(platform_headers INTERFACE
        include
        ${Platform_BINARY_DIR}/include)

LIST(APPEND PLATFORM_FILES
  src/base64.cc
  src/random.cc
  src/byte_buffer_dump.cc
  src/byteorder.c
  src/cb_malloc.cc
  src/cb_time.cc
  src/checked_snprintf.cc
  src/crc32c.cc
  src/crc32c_private.h
  src/getopt.cc
  src/global_new_replacement.cc
  src/histogram.cc
  src/interrupt.cc
  src/memorymap.cc
  src/pipe.cc
  src/processclock.cc
  src/sized_buffer.cc
  src/strerror.cc
  src/string_hex.cc
  src/sysinfo.cc
  src/thread.cc
  src/timeutils.cc
  src/uuid.cc
  include/platform/atomic_duration.h
  include/platform/base64.h
  include/platform/bitset.h
  include/platform/byte_buffer_dump.h
  include/platform/cb_malloc.h
  include/platform/checked_snprintf.h
  include/platform/corestore.h
  include/platform/crc32c.h
  include/platform/getopt.h
  include/platform/interrupt.h
  include/platform/memorymap.h
  include/platform/non_negative_counter.h
  include/platform/platform_socket.h
  include/platform/platform_thread.h
  include/platform/platform_time.h
  include/platform/pipe.h
  include/platform/processclock.h
  include/platform/random.h
  include/platform/ring_buffer.h
  include/platform/rwlock.h
  include/platform/sized_buffer.h
  include/platform/strerror.h
  include/platform/string_hex.h
  include/platform/sysinfo.h
  include/platform/thread.h
  include/platform/timeutils.h
  include/platform/uuid.h
  ${Platform_BINARY_DIR}/include/platform/visibility.h
)

add_library(platform_cbassert STATIC
            src/backtrace.c
            src/cbassert.c
            include/platform/backtrace.h
            include/platform/cbassert.h)
target_include_directories(platform_cbassert PRIVATE include)
set_target_properties(platform_cbassert PROPERTIES POSITION_INDEPENDENT_CODE true)
if (WIN32)
target_link_libraries(platform_cbassert PRIVATE DbgHelp.lib)
else()
target_link_libraries(platform_cbassert PRIVATE ${CMAKE_DL_LIBS})
endif()

# platform_so - the 'real' platform shared library containing our various
# platform abstraction functionality.
# Note that in general external users should link 'platform' and not
# 'platform_so' - see below.
add_library(platform_so SHARED)
target_link_libraries(platform_so PUBLIC ${FOLLY_LIBRARIES})
target_link_libraries(platform_so PRIVATE phosphor)

# 'platform' - the library clients should normally link to.
#
# 'platform' is a static library, which itself links to the main platform_so
# shared library. This is to ensure that any clients of platform always link
# in the C++ global operator new/delete replacements if they use platform at
# all. If they didn't then this would cause problems:
# 1) Memory usage from the client will not be tracked.
# 2) If a C++ object allocated in platform (using cbmalloc via new) is
#    delete'd in the client (using default free) then we have a mismatched
#    new/delete pair.
# By giving all clients of platform the global new/delete automatically we
# avoid these problems.
add_library(platform STATIC)

if(CB_SANITIZERS)
  # Define an additional variant of platform libs which has the
  # sanitizers disabled.  This allows targets which require platform,
  # but which are incompatible with the sanitizers (e.g. Erlang NIFs)
  # to still be built and run when we are building the rest of the
  # Server with Sanitizer support.
  add_library(platform_unsanitized_so SHARED)
  target_link_libraries(platform_unsanitized_so PRIVATE phosphor_unsanitized)
  remove_sanitizers(platform_unsanitized_so)

  add_library(platform_unsanitized STATIC)
  remove_sanitizers(platform_unsanitized)

  set(platform_VARIANTS platform platform_unsanitized)
else()
  # For convenience define a 'platform_unsanitized' alias even when
  # the sanitizers are disabled (so targets which need an unsanitized
  # platform can unconditionally link against this).
  add_library(platform_unsanitized ALIAS platform)

  set(platform_VARIANTS platform)
endif()

foreach(platform ${platform_VARIANTS})
  set(platform_so ${platform}_so)
  target_sources(${platform_so} PRIVATE ${PLATFORM_FILES})

  TARGET_LINK_LIBRARIES(${platform_so} PRIVATE
    platform_headers
    gsl_lite
    ${MALLOC_LIBRARIES}
    ${PLATFORM_LIBRARIES})

  # Link publically with COUCHBASE_NETWORK_LIBS - strictly speaking this
  # shouldn't be necessary, however a number of downstream components
  # (e.g. couchstore targets) rely on platform to make available
  # htons/htonl (via Ws2_32) etc when building on Windows.
  # Until all such targets are updated to link directly to Ws2_32, we
  # link publically.
  TARGET_LINK_LIBRARIES(${platform_so} PUBLIC
    ${COUCHBASE_NETWORK_LIBS}
    platform_cbassert)


  TARGET_INCLUDE_DIRECTORIES(${platform_so} SYSTEM PUBLIC ${FOLLY_INCLUDE_DIR})
  TARGET_INCLUDE_DIRECTORIES(${platform_so} PUBLIC include)
  # TODO: Add a TARGET_INCLUDE_DIRECTORIES(PUBLIC) to phosphor; then the
  # following line can be removed:
  TARGET_INCLUDE_DIRECTORIES(${platform_so} PRIVATE ${phosphor_SOURCE_DIR}/include)

  # On OS X 10.11 we need to remove the default rpath settings in order to all
  # us to load the platform library from Python scripts that are run out of
  # /usr/bin/python. We can remove this once we ship our own Python.
  IF (APPLE)
    SET_TARGET_PROPERTIES(${platform_so}
      PROPERTIES
      INSTALL_RPATH "")
  ENDIF ()
  SET_TARGET_PROPERTIES(${platform_so} PROPERTIES SOVERSION 0.1.0)

  INSTALL(TARGETS ${platform_so}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)


  target_sources(${platform} PRIVATE src/global_new_replacement.cc)
  SET_TARGET_PROPERTIES(${platform} PROPERTIES POSITION_INDEPENDENT_CODE true)
  TARGET_LINK_LIBRARIES(${platform} platform_headers ${platform_so})
endforeach()

GENERATE_EXPORT_HEADER(platform_so
                       EXPORT_MACRO_NAME PLATFORM_PUBLIC_API
                       EXPORT_FILE_NAME ${Platform_BINARY_DIR}/include/platform/visibility.h)

# TODO: Given dirutils now links with platform (for cbmalloc &
# global_new_replacement) we can probably just merge it into platform.
ADD_LIBRARY(dirutils SHARED
            src/dirutils.cc
            include/platform/dirutils.h)
SET_TARGET_PROPERTIES(dirutils PROPERTIES SOVERSION 0.1.0)
TARGET_LINK_LIBRARIES(dirutils platform ${CMAKE_DL_LIBS})
GENERATE_EXPORT_HEADER(dirutils
                       EXPORT_MACRO_NAME DIRUTILS_PUBLIC_API
                       EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/platform/dirutils-visibility.h)

# To simplify usage we always build breakpad_wrapper; it'll just
# do nothing on targets which don't have Breakpad.
add_library(breakpad_wrapper SHARED
            src/breakpad_wrapper.cc
            include/breakpad_wrapper/breakpad_wrapper.h)
set_target_properties(breakpad_wrapper PROPERTIES COMPILE_FLAGS "-Dplatform_so_EXPORTS")
if (BREAKPAD_FOUND)
   target_compile_definitions(breakpad_wrapper PRIVATE -DHAVE_BREAKPAD)
   target_include_directories(breakpad_wrapper PRIVATE SYSTEM AFTER ${BREAKPAD_INCLUDE_DIR})
endif (BREAKPAD_FOUND)

if (UNIX)
   # Breakpad has headers (which are dependencies for our breakpad integration
   # code) that don't compile cleanly as C++11, so the GNU standard is
   # used instead.
   set_source_files_properties(src/breakpad_wrapper.cc PROPERTIES COMPILE_FLAGS ${CB_GNU_CXX11_OPTION})
endif (UNIX)
target_link_libraries(breakpad_wrapper
                      ${BREAKPAD_LIBRARIES} platform_headers)

INSTALL(TARGETS breakpad_wrapper JSON_checker dirutils
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

ADD_SUBDIRECTORY(cbcompress)

IF (PLATFORM_ENABLE_CBSOCKET)
   ADD_SUBDIRECTORY(cbsocket)
ENDIF (PLATFORM_ENABLE_CBSOCKET)

if (COUCHBASE_PLATFORM_BUILD_UNIT_TESTS)
    ADD_SUBDIRECTORY(tests)
endif (COUCHBASE_PLATFORM_BUILD_UNIT_TESTS)
