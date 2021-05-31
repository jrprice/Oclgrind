# CMakeLists.txt (Oclgrind)
# Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
# University of Bristol. All rights reserved.
#
# This program is provided under a three-clause BSD license. For full
# license terms please see the LICENSE file distributed with this
# source code.

cmake_minimum_required(VERSION 3.1)
project(Oclgrind)
set(Oclgrind_VERSION_MAJOR 19)
set(Oclgrind_VERSION_MINOR 10)

include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(CheckLibraryExists)
include(TestBigEndian)

# Enable C99 for GCC (required for tests)
if (CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
endif()

# pthreads
#find_package( Threads )
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# Require C++14 (hard requirement for LLVM >10)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable rpath on OS X
set(CMAKE_MACOSX_RPATH 1)

if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-strict-aliasing")
endif()

# Disable min/max macros on Windows
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  add_definitions(-DNOMINMAX)
endif()

# Suppress warnings from OpenCL runtime API headers
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-ignored-attributes")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gcc-compat")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-availability")
endif()

# Detect endianness
test_big_endian(IS_BIG_ENDIAN)

# Find LLVM
find_package(LLVM REQUIRED CONFIG NO_CMAKE_BUILDS_PATH)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Check LLVM version
if (${LLVM_PACKAGE_VERSION} VERSION_LESS "11.0")
  message(FATAL_ERROR "LLVM version must be >= 11.0")
endif()

# Add flags for LLVM
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

#to build against a shared version of LLVM:
exec_program("${CLANG_ROOT}/bin/llvm-config --libfiles"
    WORKING_DIRECTORY ${CLANG_ROOT}/bin/llvm-config
    OUTPUT_VARIABLE LLVM_LIBS)
separate_arguments(LLVM_LIBS)
add_compile_definitions(LLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING=1)
message("llvm libs are now: ${LLVM_LIBS}")
# Get LLVM libraries for linking
#list(FIND LLVM_AVAILABLE_LIBS LLVM _LLVM_SHARED_INDEX)
#if (${_LLVM_SHARED_INDEX} GREATER -1)
#  set(LLVM_LIBS LLVM)
#else()
#  llvm_map_components_to_libnames(LLVM_LIBS
#    bitreader bitwriter core coroutines coverage frontendopenmp instrumentation
#    ipo irreader linker lto mcparser objcarcopts support option target)
#endif()

# https://bugs.llvm.org/show_bug.cgi?id=44870
list(FIND LLVM_AVAILABLE_LIBS Polly _POLLY_INDEX)
if (${_POLLY_INDEX} GREATER -1)
  list(APPEND LLVM_LIBS Polly)
endif()

# Allow user to set path to Clang installation via CLANG_ROOT
set (CLANG_ROOT " " CACHE PATH "Root of Clang installation")
if (NOT ${CLANG_ROOT} STREQUAL " ")
  include_directories("${CLANG_ROOT}/include")
  link_directories("${CLANG_ROOT}/lib")
  set(CMAKE_REQUIRED_INCLUDES
      "${CMAKE_REQUIRED_INCLUDES};${CLANG_ROOT}/include")
endif()

set(CMAKE_REQUIRED_INCLUDES
    "${CMAKE_REQUIRED_INCLUDES};${LLVM_INCLUDE_DIRS}")
set(CMAKE_REQUIRED_DEFINITIONS
    "${CMAKE_REQUIRED_DEFINITIONS};${LLVM_DEFINITIONS}")

# Check for Clang headers
unset(CLANG_HEADER CACHE)
find_path(CLANG_HEADER "clang/CodeGen/CodeGenAction.h"
          PATHS "${CLANG_ROOT}/include" "${LLVM_INCLUDE_DIRS}"
          NO_DEFAULT_PATH)
find_path(CLANG_HEADER "clang/CodeGen/CodeGenAction.h")
if ("${CLANG_HEADER}" STREQUAL "CLANG_HEADER-NOTFOUND")
  message(FATAL_ERROR "Clang headers not found (set CLANG_ROOT)")
endif()

# Check for Clang libraries
unset(CLANG_LIB CACHE)
find_library(CLANG_LIB "clangFrontend"
             PATHS "${CLANG_ROOT}/lib" "${LLVM_LIBRARY_DIRS}"
             NO_DEFAULT_PATH)
find_library(CLANG_LIB "clangFrontend")
set(CLANG_LIBS
  clangCodeGen clangFrontend clangSerialization clangDriver
  clangParse clangSema clangAnalysis clangEdit clangAST clangASTMatchers
  clangLex clangBasic)

if ("${CLANG_LIB}" STREQUAL "CLANG_LIB-NOTFOUND")
  # https://releases.llvm.org/10.0.0/tools/clang/docs/ReleaseNotes.html#build-system-changes
  find_library(CLANG_LIB "clang-cpp"
               PATHS "${CLANG_ROOT}/lib" "${LLVM_LIBRARY_DIRS}"
               NO_DEFAULT_PATH)
  find_library(CLANG_LIB "clang-cpp")
  set(CLANG_LIBS "clang-cpp")

  if ("${CLANG_LIB}" STREQUAL "CLANG_LIB-NOTFOUND")
    message(FATAL_ERROR "Clang libraries not found (set CLANG_ROOT)")
  endif()
endif()

# Get path to Clang's opencl-c.h header
get_filename_component(CLANG_LIB_DIR "${CLANG_LIB}" DIRECTORY)
set(CLANG_FULL_VERSION
    "${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}.${LLVM_VERSION_PATCH}")
set(CLANG_OPENCL_C_H
    "${CLANG_LIB_DIR}/clang/${CLANG_FULL_VERSION}/include/opencl-c.h")
if (NOT EXISTS "${CLANG_OPENCL_C_H}")
  message(FATAL_ERROR "\nClang opencl-c.h not found:\n\t${CLANG_OPENCL_C_H}")
else()
  message(STATUS "Using opencl-c.h: ${CLANG_OPENCL_C_H}")
endif()

# Check for clang
find_program(CLANG clang
             PATHS "${CLANG_ROOT}/bin" "${LLVM_TOOLS_BINARY_DIR}"
             NO_DEFAULT_PATH)
find_program(CLANG clang)
if ("${CLANG}" STREQUAL "CLANG-NOTFOUND")
  message(FATAL_ERROR "Could not find clang binary")
endif()


# Check for GNU readline library
if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  set(READLINE_DIR "" CACHE PATH "Location of GNU readline library")

  set(CMAKE_REQUIRED_INCLUDES ${READLINE_DIR}/include)
  include_directories(${READLINE_DIR}/include)
  link_directories(${READLINE_DIR}/lib)

  check_include_files("stdio.h;readline/readline.h" HAVE_READLINE_H)
  check_include_files("stdio.h;readline/history.h" HAVE_HISTORY_H)
  check_library_exists(readline readline
                       "${READLINE_DIR}/lib" HAVE_READLINE_LIB)
  check_library_exists(readline add_history
                       "${READLINE_DIR}/lib" HAVE_HISTORY_LIB)
  if (HAVE_READLINE_H AND HAVE_HISTORY_H AND
      HAVE_READLINE_LIB AND HAVE_HISTORY_LIB)
    set(HAVE_READLINE 1)
    list(APPEND CORE_EXTRA_LIBS readline)
  else()
    set(HAVE_READLINE 0)
    message(WARNING "GNU readline library not found (set READLINE_DIR)\n"
                    "The interactive debugger will not have a command history.")
  endif()
else()
 set(HAVE_READLINE 0)
endif()

# Check for library directory suffixes
set(_LIBDIR_SUFFIX "")
get_property(USING_LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
if (USING_LIB64 AND NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
  set(_LIBDIR_SUFFIX "64")
endif()
set(LIBDIR_SUFFIX "${_LIBDIR_SUFFIX}"
    CACHE STRING "Suffix for installed library directory")

# Generate stringified opencl-c.h
add_custom_command(
  OUTPUT src/core/opencl-c.h.cpp
  COMMAND ${CMAKE_COMMAND} -DSOURCE_FILE=${CLANG_OPENCL_C_H}
    -P ${CMAKE_SOURCE_DIR}/src/core/gen_opencl-c.h.cmake
  DEPENDS ${CLANG_OPENCL_C_H} src/core/gen_opencl-c.h.cmake
)

include_directories("src/" "${PROJECT_BINARY_DIR}")

if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  set(CORE_LIB_TYPE "SHARED")
endif()

set(CORE_HEADERS
  src/core/common.h
  src/core/Context.h
  src/core/half.h
  src/core/Kernel.h
  src/core/KernelInvocation.h
  src/core/Memory.h
  src/core/Plugin.h
  src/core/Program.h
  src/core/Queue.h
  src/core/WorkItem.h
  src/core/WorkGroup.h)

add_library(oclgrind ${CORE_LIB_TYPE}
  ${CORE_HEADERS}
  src/core/opencl-c.h.cpp
  src/core/common.cpp
  src/core/Context.cpp
  src/core/half.cpp
  src/core/Kernel.cpp
  src/core/KernelInvocation.cpp
  src/core/Memory.cpp
  src/core/Plugin.cpp
  src/core/Program.cpp
  src/core/Queue.cpp
  src/core/WorkItem.cpp
  src/core/WorkItemBuiltins.cpp
  src/core/WorkGroup.cpp
  src/plugins/InstructionCounter.h
  src/plugins/InstructionCounter.cpp
  src/plugins/InteractiveDebugger.h
  src/plugins/InteractiveDebugger.cpp
  src/plugins/Logger.h
  src/plugins/Logger.cpp
  src/plugins/MemCheck.h
  src/plugins/MemCheck.cpp
  src/plugins/RaceDetector.h
  src/plugins/RaceDetector.cpp
  src/plugins/Uninitialized.h
  src/plugins/Uninitialized.cpp
  src/plugins/WorkloadCharacterisation.h
  src/plugins/WorkloadCharacterisation.cpp)
set_target_properties(oclgrind PROPERTIES INSTALL_RPATH ${CLANG_ROOT}/lib)
target_link_libraries(oclgrind PUBLIC ${CORE_EXTRA_LIBS} -Wl,-rpath,${CLANG_ROOT}/lib
  clangFrontend clangSerialization clangDriver clangCodeGen
  clangParse clangSema clangAnalysis clangEdit clangAST clangLex clangBasic
  "${LLVM_LIBS}" Threads::Threads dl)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  target_link_libraries(oclgrind PRIVATE Version)
endif()

# Sources for OpenCL runtime API frontend
set(RUNTIME_SOURCES
  src/runtime/async_queue.h
  src/runtime/async_queue.cpp
  src/runtime/icd.h
  src/runtime/runtime.cpp)

# Add ICD exports on Windows
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  set(DLL_EXPORTS src/runtime/icd.def)
endif()

add_library(oclgrind-rt-icd SHARED ${RUNTIME_SOURCES} ${DLL_EXPORTS})
set_target_properties(oclgrind-rt-icd PROPERTIES COMPILE_FLAGS "-DOCLGRIND_ICD")
set_target_properties(oclgrind-rt-icd PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
target_link_libraries(oclgrind-rt-icd oclgrind)

# Add runtime exports on Windows
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  set(DLL_EXPORTS src/runtime/runtime.def)
endif()

add_library(oclgrind-rt SHARED ${RUNTIME_SOURCES} ${DLL_EXPORTS})
set_target_properties(oclgrind-rt PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
target_link_libraries(oclgrind-rt oclgrind)

if (UNIX AND NOT APPLE)
  # Change the SONAME of the library so that it gets recognized by dlopen
  set_target_properties(oclgrind-rt PROPERTIES
                        NO_SONAME ON
                        LINK_FLAGS "-Wl,-soname,libOpenCL.so")
  set_target_properties(oclgrind-rt-icd PROPERTIES
                        NO_SONAME ON
                        LINK_FLAGS "-Wl,-soname,libOpenCL.so.1")
endif()

add_executable(oclgrind-exe src/runtime/oclgrind.cpp)
set_target_properties(oclgrind-exe PROPERTIES OUTPUT_NAME oclgrind)
target_compile_definitions(oclgrind-exe PRIVATE
                           "-DLIBDIR_SUFFIX=\"${LIBDIR_SUFFIX}\"")

add_executable(oclgrind-kernel
  src/kernel/oclgrind-kernel.cpp
  src/kernel/Simulation.h
  src/kernel/Simulation.cpp)
target_link_libraries(oclgrind-kernel oclgrind)

set(OPENCL_C_H
 ${CMAKE_BINARY_DIR}/include/oclgrind/opencl-c.h
 ${CMAKE_BINARY_DIR}/include/oclgrind/opencl-c-1.2-32.pch
 ${CMAKE_BINARY_DIR}/include/oclgrind/opencl-c-1.2-64.pch
 ${CMAKE_BINARY_DIR}/include/oclgrind/opencl-c-2.0-32.pch
 ${CMAKE_BINARY_DIR}/include/oclgrind/opencl-c-2.0-64.pch
)

add_custom_target(OPENCL_C_HEADERS ALL DEPENDS ${OPENCL_C_H})

add_custom_command(
  OUTPUT include/oclgrind/opencl-c.h
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E
    copy ${CLANG_OPENCL_C_H} include/oclgrind/opencl-c.h
  DEPENDS ${CLANG_OPENCL_C_H})

# Generate precompiled headers for opencl-c.h
set(OPENCL_C_H_SYSROOT "${CMAKE_BINARY_DIR}/include/oclgrind/")
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  string(REPLACE "/" "\\" OPENCL_C_H_SYSROOT "${OPENCL_C_H_SYSROOT}")
endif()
add_custom_command(
  OUTPUT include/oclgrind/opencl-c-1.2-32.pch
  POST_BUILD
  COMMAND
    ${CLANG}
    -cc1 -x cl -cl-std=CL1.2 -O0 -fno-builtin -fgnu89-inline
    -emit-pch -triple spir-unknown-unknown
    -relocatable-pch -isysroot "${OPENCL_C_H_SYSROOT}"
    include/oclgrind/opencl-c.h
    -o include/oclgrind/opencl-c-1.2-32.pch
  DEPENDS include/oclgrind/opencl-c.h
)
add_custom_command(
  OUTPUT include/oclgrind/opencl-c-1.2-64.pch
  POST_BUILD
  COMMAND
    ${CLANG}
    -cc1 -x cl -cl-std=CL1.2 -O0 -fno-builtin -fgnu89-inline
    -emit-pch -triple spir64-unknown-unknown
    -relocatable-pch -isysroot "${OPENCL_C_H_SYSROOT}"
    include/oclgrind/opencl-c.h
    -o include/oclgrind/opencl-c-1.2-64.pch
  DEPENDS include/oclgrind/opencl-c.h
)
add_custom_command(
  OUTPUT include/oclgrind/opencl-c-2.0-32.pch
  POST_BUILD
  COMMAND
    ${CLANG}
    -cc1 -x cl -cl-std=CL2.0 -O0 -fno-builtin -fgnu89-inline
    -emit-pch -triple spir-unknown-unknown
    -relocatable-pch -isysroot "${OPENCL_C_H_SYSROOT}"
    include/oclgrind/opencl-c.h
    -o include/oclgrind/opencl-c-2.0-32.pch
  DEPENDS include/oclgrind/opencl-c.h
)
add_custom_command(
  OUTPUT include/oclgrind/opencl-c-2.0-64.pch
  POST_BUILD
  COMMAND
    ${CLANG}
    -cc1 -x cl -cl-std=CL2.0 -O0 -fno-builtin -fgnu89-inline
    -emit-pch -triple spir64-unknown-unknown
    -relocatable-pch -isysroot "${OPENCL_C_H_SYSROOT}"
    include/oclgrind/opencl-c.h
    -o include/oclgrind/opencl-c-2.0-64.pch
  DEPENDS include/oclgrind/opencl-c.h
)


# Generate config.h
set(LLVM_VERSION ${LLVM_VERSION_MAJOR}${LLVM_VERSION_MINOR})
if(NOT INSTALLED_LIBRARY_DIR)
  set(INSTALLED_LIBRARY_DIR "${CMAKE_INSTALL_PREFIX}/lib")
endif()

configure_file("config.h.in" "config.h")


# Generate ICD loader if not on Windows
if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/oclgrind.icd
       CONTENT "${CMAKE_INSTALL_PREFIX}/lib${LIBDIR_SUFFIX}/$<TARGET_FILE_NAME:oclgrind-rt-icd>\n")
endif()

install(TARGETS
  oclgrind-exe oclgrind-kernel
  DESTINATION bin)
install(TARGETS
  oclgrind oclgrind-rt oclgrind-rt-icd
  DESTINATION "lib${LIBDIR_SUFFIX}")
install(FILES
  ${CORE_HEADERS} ${OPENCL_C_H}
  DESTINATION include/oclgrind)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  install(FILES
    src/CL/cl.h
    src/CL/cl_d3d10.h
    src/CL/cl_d3d11.h
    src/CL/cl_dx9_media_sharing.h
    src/CL/cl_egl.h
    src/CL/cl_ext.h
    src/CL/cl_gl.h
    src/CL/cl_gl_ext.h
    src/CL/cl_platform.h
    src/CL/opencl.h
    DESTINATION include/CL)
endif()


# Tests
enable_testing()

# Check for Python
find_package(PythonInterp)
if (PYTHONINTERP_FOUND)

  # Add test directories
  add_subdirectory(tests/apps)
  add_subdirectory(tests/kernels)
  add_subdirectory(tests/runtime)

else()
  message(WARNING "Tests will not be run (Python required)")
endif()


# CPack config
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OpenCL device simulator")
set(CPACK_PACKAGE_DESCRIPTION_FILE
    "${CMAKE_SOURCE_DIR}/src/install/cpack-description")
set(CPACK_PACKAGE_VENDOR "University of Bristol")
set(CPACK_PACKAGE_VERSION_MAJOR ${Oclgrind_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${Oclgrind_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION "${Oclgrind_VERSION_MAJOR}.${Oclgrind_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "0")

# CPack RPM config
set(CPACK_RPM_PACKAGE_GROUP "Development/Tools")
set(CPACK_RPM_PACKAGE_LICENSE "BSD")

include(CPack)
