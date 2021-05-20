CMAKE_MINIMUM_REQUIRED(VERSION 3.15)
PROJECT(Platform LANGUAGES C CXX)

INCLUDE(CheckFunctionExists)
INCLUDE(CheckIncludeFile)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(CheckSymbolExists)
INCLUDE(CTest)

OPTION(PLATFORM_ENABLE_CBSOCKET "Build cbsocket support" ON)

enable_code_coverage_report()

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

    # Used by sysinfo.cc in get_available_cpu_count
    if (HAVE_SCHED_GETAFFINITY)
        add_definitions(-DHAVE_SCHED_GETAFFINITY=1)
    endif()

    # The following checks is used by sysinfo.cc to get cpu index
    if (HAVE_SCHED_GETCPU)
        add_definitions(-DHAVE_SCHED_GETCPU=1)
    endif()

    if (HAVE_CPUID_H)
        add_definitions(-DHAVE_CPUID_H=1)
    endif()
endif (NOT WIN32)

CONFIGURE_FILE (${CMAKE_CURRENT_SOURCE_DIR}/include/platform/dynamic.in.h
                ${CMAKE_CURRENT_BINARY_DIR}/include/platform/dynamic.h)

#
# Set the include path
#
include_directories(BEFORE SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/external)

IF (MEMORY_ALLOCATOR)
  include_directories(AFTER SYSTEM ${MALLOC_INCLUDE_DIR})
ENDIF (MEMORY_ALLOCATOR)

ADD_LIBRARY(JSON_checker STATIC src/JSON_checker.cc include/JSON_checker.h)
TARGET_LINK_LIBRARIES(JSON_checker PRIVATE platform_headers)

IF (WIN32)
   INCLUDE_DIRECTORIES(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/include/win32)
   ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
   SET(PLATFORM_FILES src/sockets.cc
                      src/cb_win32.cc
                      src/crc32c_hw_accel.cc
                      include/win32/getopt.h)
ELSE (WIN32)
   SET(PLATFORM_FILES src/cb_pthreads.cc)
   set(crc32_hw_archs "aarch64;AMD64;x86_64")
   if(CMAKE_SYSTEM_PROCESSOR IN_LIST crc32_hw_archs)
      list(APPEND PLATFORM_FILES src/crc32c_hw_accel.cc)
   endif()
   if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
      set_source_files_properties(src/crc32c_hw_accel.cc
                                  PROPERTIES COMPILE_FLAGS -msse4.2)
   endif()
   if (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
      set_source_files_properties(src/crc32c_hw_accel.cc
                                  PROPERTIES COMPILE_FLAGS -march=armv8-a+crc)
   endif()
   # To avoid feature (sse4.2) mismatch errors when compiling we should avoid
   # including the precompiled header in crc32c_sse4_2
   set_source_files_properties(src/crc32c_hw_accel.cc
                               PROPERTIES
                               SKIP_PRECOMPILE_HEADERS ON)
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
TARGET_LINK_LIBRARIES(platform_headers INTERFACE Folly::headers)
TARGET_INCLUDE_DIRECTORIES(platform_headers INTERFACE
        include
        ${Platform_BINARY_DIR}/include)

if (WIN32)
  # WIN2 platform library:
  #  provide cb_malloc directly (that uses arena tracking with jemalloc/malloc
  #  depending on the build configuration)
  set(CB_MALLOC_IMPL src/cb_malloc_arena.cc)
elseif (UNIX)
  # UNIX platform library:
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
  src/murmurhash3.cc
  src/processclock.cc
  src/process_monitor.cc
  src/semaphore.cc
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
  include/platform/process_monitor.h
  include/platform/random.h
  include/platform/ring_buffer.h
  include/platform/rwlock.h
  include/platform/semaphore.h
  include/platform/sized_buffer.h
  include/platform/strerror.h
  include/platform/string_hex.h
  include/platform/sysinfo.h
  include/platform/thread.h
  include/platform/timeutils.h
  include/platform/uuid.h
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
  target_link_libraries(${platform_cbassert_variant} PRIVATE Folly::headers)
endforeach()

if (MEMORY_ALLOCATOR STREQUAL "jemalloc")
LIST(APPEND PLATFORM_FILES src/je_arena_corelocal_tracker.cc)
LIST(APPEND PLATFORM_FILES src/je_arena_malloc.cc)
LIST(APPEND PLATFORM_FILES src/je_arena_malloc_stats.cc)
endif()
# Always build the system_arena code for simpler concurrent development
LIST(APPEND PLATFORM_FILES src/system_arena_malloc.cc)


if (CB_PCH)
    # Given consumers of precompiled headers must have the same compile flags,
    # we need two PCH targets - with and without position-independent code.
    foreach(pch_obj_lib platform_pch platform_pch_fpic)
        add_library(${pch_obj_lib} OBJECT precompiled_headers.cc)
        target_link_libraries(${pch_obj_lib} PUBLIC Folly::headers)
        target_include_directories(${pch_obj_lib} PUBLIC ${gtest_SOURCE_DIR}/include)
        target_precompile_headers(${pch_obj_lib} PRIVATE precompiled_headers.h)
    endforeach()
    set_property(TARGET platform_pch_fpic PROPERTY POSITION_INDEPENDENT_CODE 1)
endif()

function(platform_enable_pch target)
    if (CB_PCH)
        # ASan/TSan builds don't like a mix of position independent code so we
        # need to use the correct pch target
        get_property(fpic TARGET ${target} PROPERTY POSITION_INDEPENDENT_CODE)
        if (fpic)
            reuse_pch(${target} platform_pch_fpic)
        else()
            reuse_pch(${target} platform_pch)
        endif()
    endif()
endfunction()

# Skip using precompiled headers in global_new_replacement or we end up with
# redundant redeclarations of malloc_usable_size
set_source_files_properties(src/global_new_replacement.cc
        PROPERTIES
        SKIP_PRECOMPILE_HEADERS ON)

# platform - the main platform library containing our various
# platform abstraction functionality.
#
# Platform includes a C++ global operator new/delete replacement,
# which redirects to cb_malloc and friends. This is implemented using
# the selected COUCHBASE_MEMORY_ALLOCATOR (typically jemalloc in
# production builds). cb_malloc et al are defined as weak symbols in
# platform. This allows them to be overridden - for example using
# platform_cb_arena_malloc to also record per-arena memory usage.
#
# Note this is a static library - this is primary
# because it uses Facebook Folly, and we have encountered a number of
# bugs when Folly is used from multiple shared binaries in the same
# process - i.e. if both the main exe (memcached) and platform.so
# linked folly, we run in to problems - see MB-45378.
add_library(platform STATIC)
target_link_libraries(platform PUBLIC Folly::folly platform_headers)
target_link_libraries(platform PRIVATE
  phosphor
  gsl_lite
  ${MALLOC_LIBRARIES}
  ${PLATFORM_LIBRARIES})

target_sources(platform PRIVATE ${PLATFORM_FILES})

# Link publically with COUCHBASE_NETWORK_LIBS - strictly speaking this
# shouldn't be necessary, however a number of downstream components
# (e.g. couchstore targets) rely on platform to make available
# htons/htonl (via Ws2_32) etc when building on Windows.
# Until all such targets are updated to link directly to Ws2_32, we
# link publically.
TARGET_LINK_LIBRARIES(platform PUBLIC
  ${COUCHBASE_NETWORK_LIBS}
  platform_cbassert)

SET_TARGET_PROPERTIES(platform PROPERTIES POSITION_INDEPENDENT_CODE true)
platform_enable_pch(platform)
cb_enable_unity_build(platform)
# Exclude our defintions of new/delete. When included in a unity build this
# file has a number of issues to address. Compile failure due to differing
# defintions of malloc_usable_size on WIN32 and Linux. A second issue occurs
# on ASAN only where duplicate symbol errors are seen, all of our new/delete
# symbols now clash with the ASAN replacements).
set_source_files_properties(src/global_new_replacement.cc
                            PROPERTIES SKIP_UNITY_BUILD_INCLUSION 1)

# platform_cb_arena_malloc contains strong symbols for
# cb_malloc/cb_free et al; which override the default behavour in
# platform and call down to cb::ArenaMalloc. In addition to supporting
# different arenas which can be assigned per-thread, this also
# facilitates tracking the amount of memory allocated per arena.  The
# primary use-case for platform_cb_arena_malloc is memcached itself,
# to track memory usage at the per-Bucket level, by creating an Arena
# per bucket.
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
  # Note this must be an object library and not a static library. This
  # is because the strong symbols for cb_malloc et al will _not_
  # necessarily override the weak symbols from 'platform' - if
  # platform is linked first (which it could well be depending on link
  # order then the first definition is used (weak or
  # otherwise). However if we use an OBJECT library (which essentially
  # passes each .o file to the linker, the strong symbol _will_
  # correctly take presidence.
  add_library(platform_cb_malloc_arena OBJECT ${PLATFORM_CB_MALLOC_ARENA_FILES})
  target_link_libraries(platform_cb_malloc_arena platform ${MALLOC_LIBRARIES})
  set_target_properties(platform_cb_malloc_arena PROPERTIES
                                           POSITION_INDEPENDENT_CODE true)
  target_include_directories(platform_cb_malloc_arena PUBLIC include)
else ()
  message(FATAL_ERROR "Unsupported system")
endif (WIN32)

# To simplify usage we always build breakpad_wrapper; it'll just
# do nothing on targets which don't have Breakpad.
add_library(breakpad_wrapper SHARED
            src/breakpad_wrapper.cc
            include/breakpad_wrapper/breakpad_wrapper.h)
set_target_properties(breakpad_wrapper PROPERTIES COMPILE_FLAGS "-Dplatform_so_EXPORTS")
if (BREAKPAD_FOUND)
   target_compile_definitions(breakpad_wrapper PRIVATE -DHAVE_BREAKPAD)
   target_include_directories(breakpad_wrapper SYSTEM PRIVATE ${BREAKPAD_INCLUDE_DIR})
   target_link_libraries(breakpad_wrapper ${BREAKPAD_LIBRARIES})
endif (BREAKPAD_FOUND)
target_link_libraries(breakpad_wrapper platform_headers)

if (UNIX)
   # Breakpad has headers (which are dependencies for our breakpad integration
   # code) that don't compile cleanly as C++11, so the GNU standard is
   # used instead.
   set_source_files_properties(src/breakpad_wrapper.cc PROPERTIES COMPILE_FLAGS ${CB_GNU_CXX11_OPTION})
endif (UNIX)

INSTALL(TARGETS breakpad_wrapper
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

ADD_SUBDIRECTORY(cbcompress)

IF (PLATFORM_ENABLE_CBSOCKET)
   ADD_SUBDIRECTORY(cbsocket)
ENDIF (PLATFORM_ENABLE_CBSOCKET)

add_subdirectory(tests)
