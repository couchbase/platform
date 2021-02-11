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

if (NOT WIN32)
    # Backtrace support on !win32 use backtrace() from execinfo.h and dladdr()
    # from dlfcn.h. Verify that they're there
    CMAKE_PUSH_CHECK_STATE(RESET)
    FIND_LIBRARY(EXECINFO_LIBRARY NAMES execinfo)
    IF (EXECINFO_LIBRARY)
        SET(CMAKE_REQUIRED_LIBRARIES "${EXECINFO_LIBRARY}")
        LIST(APPEND PLATFORM_LIBRARIES "${EXECINFO_LIBRARY}")
    ENDIF (EXECINFO_LIBRARY)
    CHECK_SYMBOL_EXISTS(backtrace execinfo.h HAVE_BACKTRACE)
    if (NOT HAVE_BACKTRACE)
        message(FATAL_ERROR "backtrace not found in execinfo.h and is needed for backtrace support")
    endif ()
    CMAKE_POP_CHECK_STATE()

    CMAKE_PUSH_CHECK_STATE(RESET)
    SET(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
    FIND_LIBRARY(DL_LIBRARY NAMES dl)
    IF (DL_LIBRARY)
        SET(CMAKE_REQUIRED_LIBRARIES "${DL_LIBRARY}")
        LIST(APPEND PLATFORM_LIBRARIES "${DL_LIBRARY}")
    ENDIF (DL_LIBRARY)
    CHECK_SYMBOL_EXISTS(dladdr dlfcn.h HAVE_DLADDR)
    if (NOT HAVE_DLADDR)
        message(FATAL_ERROR "dladdr not found in dlfcn.h and is needed for backtrace support")
    endif (NOT HAVE_DLADDR)
    CMAKE_POP_CHECK_STATE()
endif (NOT WIN32)

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
include_directories(BEFORE SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/external)

IF (MEMORY_ALLOCATOR)
  include_directories(AFTER SYSTEM ${MALLOC_INCLUDE_DIR})
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
   SET(PLATFORM_FILES src/sockets.cc
                      src/cb_win32.cc
                      src/crc32c_sse4_2.cc
                      include/win32/getopt.h
                      include/win32/unistd.h)
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

cmake_push_check_state(RESET)
# On macOS, aligned_alloc() is only available when CMAKE_OSX_DEPLOYMENT_TARGET
# is 10.15 (Catalina) upwards; but confusingly the symbol _is_ available if
# compiling on 10.15 even when CMAKE_OSX_DEPLOYMENT_TARGET is less than 10.15,
# and hence CHECK_SYMBOL_EXISTS() check will pass, with only a warning:
#
#     warning: 'aligned_alloc' is only available on macOS 10.15 or newer [-Wunguarded-availability-new]
#
# Address this by enabling warnings-as-errors for macOS and hence make
# aligned_alloc check fail in this scenario.
if (APPLE)
    set(CMAKE_REQUIRED_FLAGS -Werror)
endif()
check_symbol_exists(aligned_alloc stdlib.h HAVE_ALIGNED_ALLOC)
if(HAVE_ALIGNED_ALLOC)
    add_definitions(-DHAVE_ALIGNED_ALLOC)
endif()
cmake_pop_check_state()

check_symbol_exists(posix_memalign stdlib.h HAVE_POSIX_MEMALIGN)
if(HAVE_POSIX_MEMALIGN)
    add_definitions(-DHAVE_POSIX_MEMALIGN)
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

if (WIN32)
  # WIN2 platform.dll:
  #  provide cb_malloc directly (that uses arena tracking with jemalloc/malloc
  #  depending on the build configuration)
  set(CB_MALLOC_IMPL src/cb_malloc_arena.cc)
elseif (UNIX)
  # UNIX platform.so:
  #  provide a weakly defined cb_malloc that uses plain je_malloc (or malloc)
  #  Targets desiring the tracking allocator will need to also link to
  #  platform_cb_malloc.
  set(CB_MALLOC_IMPL src/cb_malloc_default.cc)
else ()
  message(FATAL_ERROR "Unsupported system")
endif (WIN32)

LIST(APPEND PLATFORM_FILES
  src/base64.cc
  src/dirutils.cc
  src/loadfile.cc
  src/random.cc
  src/byte_buffer_dump.cc
  src/cb_arena_malloc.cc
  src/cb_time.cc
  src/checked_snprintf.cc
  src/crc32c.cc
  src/crc32c_private.h
  src/getopt.cc
  src/global_new_replacement.cc
  src/histogram.cc
  src/interrupt.cc
  src/processclock.cc
  src/sized_buffer.cc
  src/strerror.cc
  src/string_hex.cc
  src/sysinfo.cc
  src/thread.cc
  src/timeutils.cc
  src/uuid.cc
  ${CB_MALLOC_IMPL}
  include/platform/atomic_duration.h
  include/platform/base64.h
  include/platform/bitset.h
  include/platform/byte_buffer_dump.h
  include/platform/cb_malloc.h
  include/platform/checked_snprintf.h
  include/platform/corestore.h
  include/platform/crc32c.h
  include/platform/dirutils.h
  include/platform/getopt.h
  include/platform/interrupt.h
  include/platform/non_negative_counter.h
  include/platform/platform_socket.h
  include/platform/platform_thread.h
  include/platform/platform_time.h
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

# Build a separate static library for cbassert/backtrace
LIST(APPEND CBASSERT_FILES
  src/backtrace.cc
  src/cbassert.cc
  include/platform/backtrace.h
  include/platform/cbassert.h)

add_library(platform_cbassert STATIC)

# If the build has sanitizers enabled generate two libraries
# If the build does not, build one library but use an alias to make it look like
# two.
if(CB_SANITIZERS)
  add_library(platform_cbassert_unsanitized STATIC)
  remove_sanitizers(platform_cbassert_unsanitized)
  set(platform_cbassert_VARIANTS
      platform_cbassert
      platform_cbassert_unsanitized)
else()
  add_library(platform_cbassert_unsanitized ALIAS platform_cbassert)
  set(platform_cbassert_VARIANTS platform_cbassert)
endif(CB_SANITIZERS)

foreach(platform_cbassert_variant ${platform_cbassert_VARIANTS})
  target_sources(${platform_cbassert_variant} PRIVATE ${CBASSERT_FILES})
  target_include_directories(${platform_cbassert_variant} SYSTEM PRIVATE ${FOLLY_INCLUDE_DIR})
  target_include_directories(${platform_cbassert_variant} PRIVATE include)
  set_target_properties(${platform_cbassert_variant}
                        PROPERTIES
                        POSITION_INDEPENDENT_CODE
                        true)
  if (WIN32)
    target_link_libraries(${platform_cbassert_variant} PRIVATE DbgHelp.lib)
  else()
    target_link_libraries(${platform_cbassert_variant} PRIVATE ${CMAKE_DL_LIBS})
  endif()
endforeach()

if (MEMORY_ALLOCATOR STREQUAL "jemalloc")
LIST(APPEND PLATFORM_FILES src/je_arena_corelocal_tracker.cc)
LIST(APPEND PLATFORM_FILES src/je_arena_malloc.cc)
LIST(APPEND PLATFORM_FILES src/je_arena_malloc_stats.cc)
endif()
# Always build the system_arena code for simpler concurrent development
LIST(APPEND PLATFORM_FILES src/system_arena_malloc.cc)

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
  target_link_libraries(platform_unsanitized_so
      PRIVATE
          phosphor_unsanitized
          ${FOLLY_LIBRARIES_UNSANITIZED})
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

  if(platform MATCHES "unsanitized")
    set(cbassert_lib platform_cbassert_unsanitized)
  else()
    set(cbassert_lib platform_cbassert)
  endif()

  # Link publically with COUCHBASE_NETWORK_LIBS - strictly speaking this
  # shouldn't be necessary, however a number of downstream components
  # (e.g. couchstore targets) rely on platform to make available
  # htons/htonl (via Ws2_32) etc when building on Windows.
  # Until all such targets are updated to link directly to Ws2_32, we
  # link publically.
  TARGET_LINK_LIBRARIES(${platform_so} PUBLIC
    ${COUCHBASE_NETWORK_LIBS}
    ${cbassert_lib})

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

if (WIN32)
  # Windows: create an empty interface library so targets can specify they link
  # to platform_cb_malloc_arena, but it is a no-op on WIN32.
  add_library(platform_cb_malloc_arena INTERFACE)
elseif (UNIX)
  # On UNIX any targets that want the full memory tracking allocator and/or
  # JeMalloc must link against this extra library. This library will give
  # the target a strong symbol for cb_malloc, overriding the weak symbol within
  # platform_so.so
  LIST(APPEND PLATFORM_CB_MALLOC_ARENA_FILES src/cb_malloc_arena.cc)
  if (MEMORY_ALLOCATOR STREQUAL "jemalloc")
    LIST(APPEND PLATFORM_CB_MALLOC_ARENA_FILES src/je_malloc_conf.cc)
  endif()
  add_library(platform_cb_malloc_arena STATIC ${PLATFORM_CB_MALLOC_ARENA_FILES})
  target_link_libraries(platform_cb_malloc_arena platform ${MALLOC_LIBRARIES})
  set_target_properties(platform_cb_malloc_arena PROPERTIES
                                           POSITION_INDEPENDENT_CODE true)
  target_include_directories(platform_cb_malloc_arena PUBLIC include)
else ()
  message(FATAL_ERROR "Unsupported system")
endif (WIN32)

GENERATE_EXPORT_HEADER(platform_so
                       EXPORT_MACRO_NAME PLATFORM_PUBLIC_API
                       EXPORT_FILE_NAME ${Platform_BINARY_DIR}/include/platform/visibility.h)

# dirutils is moved to platform, but until we've fixed all references
# to the old library in the various projects we'll just keep it around
# as an alias
add_library(dirutils ALIAS platform)

# To simplify usage we always build breakpad_wrapper; it'll just
# do nothing on targets which don't have Breakpad.
add_library(breakpad_wrapper SHARED
            src/breakpad_wrapper.cc
            include/breakpad_wrapper/breakpad_wrapper.h)
set_target_properties(breakpad_wrapper PROPERTIES COMPILE_FLAGS "-Dplatform_so_EXPORTS")
if (BREAKPAD_FOUND)
   target_compile_definitions(breakpad_wrapper PRIVATE -DHAVE_BREAKPAD)
   target_include_directories(breakpad_wrapper SYSTEM PRIVATE ${BREAKPAD_INCLUDE_DIR})
endif (BREAKPAD_FOUND)

if (UNIX)
   # Breakpad has headers (which are dependencies for our breakpad integration
   # code) that don't compile cleanly as C++11, so the GNU standard is
   # used instead.
   set_source_files_properties(src/breakpad_wrapper.cc PROPERTIES COMPILE_FLAGS ${CB_GNU_CXX11_OPTION})
endif (UNIX)
target_link_libraries(breakpad_wrapper
                      ${BREAKPAD_LIBRARIES} platform_headers)

INSTALL(TARGETS breakpad_wrapper JSON_checker
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
