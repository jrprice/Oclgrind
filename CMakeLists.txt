# CMakeLists.txt (Oclgrind)
# Copyright (c) 2013-2016, James Price and Simon McIntosh-Smith,
# University of Bristol. All rights reserved.
#
# This program is provided under a three-clause BSD license. For full
# license terms please see the LICENSE file distributed with this
# source code.

cmake_minimum_required(VERSION 2.8.12)
project(Oclgrind)
set(Oclgrind_VERSION_MAJOR 15)
set(Oclgrind_VERSION_MINOR 5)

include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(CheckLibraryExists)

# Enable C99 for GCC (required for tests)
if (CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
endif()

# Enable rpath on OS X
set(CMAKE_MACOSX_RPATH 1)

if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
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
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-ignored-attributes -Wno-gcc-compat -Wno-availability")
endif()


# Find LLVM
find_package(LLVM REQUIRED CONFIG NO_CMAKE_BUILDS_PATH)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Check LLVM version
if (${LLVM_PACKAGE_VERSION} VERSION_LESS "3.6")
  message(FATAL_ERROR "LLVM version must be >= 3.6")
endif()
set(LLVM_VERSION ${LLVM_VERSION_MAJOR}${LLVM_VERSION_MINOR})

# Add flags for LLVM
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

# Get LLVM libraries for linking
llvm_map_components_to_libnames(LLVM_LIBS
  bitreader bitwriter core instrumentation ipo irreader
  linker lto mcparser objcarcopts option target)

if (NOT (${LLVM_PACKAGE_VERSION} VERSION_LESS "3.9"))
  llvm_map_components_to_libnames(LLVM_COVERAGE coverage)
  list(APPEND LLVM_LIBS ${LLVM_COVERAGE})
endif()

if (NOT (${LLVM_PACKAGE_VERSION} VERSION_LESS "4.0"))
  llvm_map_components_to_libnames(LLVM_COROUTINES coroutines)
  list(APPEND LLVM_LIBS ${LLVM_COROUTINES})
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
if ("${CLANG_LIB}" STREQUAL "CLANG_LIB-NOTFOUND")
  message(FATAL_ERROR "Clang libraries not found (set CLANG_ROOT)")
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

  message(STATUS ${CMAKE_REQUIRED_LIBRARIES})

  check_include_files("stdio.h;readline/readline.h" HAVE_READLINE_H)
  check_include_files("stdio.h;readline/history.h" HAVE_HISTORY_H)
  check_library_exists(readline readline "${READLINE_DIR}/lib" HAVE_READLINE_LIB)
  check_library_exists(readline add_history "${READLINE_DIR}/lib" HAVE_HISTORY_LIB)
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
set(LIBDIR_SUFFIX "")
get_property(USING_LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
if (USING_LIB64 AND NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
  set(LIBDIR_SUFFIX "64")
endif()

# Generate stringified clc.h
add_custom_command(
  OUTPUT src/core/clc_h.cpp
  COMMAND ${CMAKE_COMMAND} -DSOURCE_FILE=${CMAKE_SOURCE_DIR}/src/core/clc.h
    -P ${CMAKE_SOURCE_DIR}/src/core/gen_clc_h.cmake
  DEPENDS src/core/clc.h src/core/gen_clc_h.cmake
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
  src/core/clc_h.cpp
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
  src/plugins/ArithmeticExceptions.h
  src/plugins/ArithmeticExceptions.cpp
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
  src/plugins/Uninitialized.cpp)
target_link_libraries(oclgrind PRIVATE ${CORE_EXTRA_LIBS}
  clangFrontend clangSerialization clangDriver clangCodeGen
  clangParse clangSema clangAnalysis clangEdit clangAST clangLex clangBasic
  ${LLVM_LIBS})

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
set_target_properties(oclgrind-rt-icd PROPERTIES COMPILE_FLAGS -DOCLGRIND_ICD)
target_link_libraries(oclgrind-rt-icd ${CMAKE_DL_LIBS} oclgrind)

# Add runtime exports on Windows
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  set(DLL_EXPORTS src/runtime/runtime.def)
endif()

add_library(oclgrind-rt SHARED ${RUNTIME_SOURCES} ${DLL_EXPORTS})
target_link_libraries(oclgrind-rt ${CMAKE_DL_LIBS} oclgrind)

add_executable(oclgrind-exe src/runtime/oclgrind.cpp)
set_target_properties(oclgrind-exe PROPERTIES OUTPUT_NAME oclgrind)
target_compile_definitions(oclgrind-exe PRIVATE "-DLIBDIR_SUFFIX=\"${LIBDIR_SUFFIX}\"")

add_executable(oclgrind-kernel
  src/kernel/oclgrind-kernel.cpp
  src/kernel/Simulation.h
  src/kernel/Simulation.cpp)
target_link_libraries(oclgrind-kernel oclgrind)

set(CLC_HEADERS
 ${CMAKE_BINARY_DIR}/include/oclgrind/clc.h
 ${CMAKE_BINARY_DIR}/include/oclgrind/clc32.pch
 ${CMAKE_BINARY_DIR}/include/oclgrind/clc64.pch
)

add_custom_target(CLC_HEADERS ALL DEPENDS ${CLC_HEADERS})

add_custom_command(
  OUTPUT include/oclgrind/clc.h
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E
    copy ${CMAKE_SOURCE_DIR}/src/core/clc.h include/oclgrind/clc.h
  DEPENDS src/core/clc.h)

# Generate precompiled headers for clc.h
add_custom_command(
  OUTPUT include/oclgrind/clc32.pch
  POST_BUILD
  COMMAND
    ${CLANG}
    -cc1 -x cl -cl-std=CL1.2 -O0 -fno-builtin
    -emit-pch -triple spir-unknown-unknown
    -relocatable-pch -isysroot ${CMAKE_BINARY_DIR}/include/oclgrind/
    ${CMAKE_BINARY_DIR}/include/oclgrind/clc.h
    -o include/oclgrind/clc32.pch
  DEPENDS include/oclgrind/clc.h
)
add_custom_command(
  OUTPUT include/oclgrind/clc64.pch
  POST_BUILD
  COMMAND
    ${CLANG}
    -cc1 -x cl -cl-std=CL1.2 -O0 -fno-builtin
    -emit-pch -triple spir64-unknown-unknown
    -relocatable-pch -isysroot ${CMAKE_BINARY_DIR}/include/oclgrind/
    ${CMAKE_BINARY_DIR}/include/oclgrind/clc.h
    -o include/oclgrind/clc64.pch
  DEPENDS include/oclgrind/clc.h
)


# Generate config.h
configure_file("cmake_config.h.in" "config.h")


# Generate ICD loader if not on Windows
if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/oclgrind.icd
       CONTENT "$<TARGET_FILE:oclgrind-rt-icd>\n")
endif()

install(TARGETS
  oclgrind-exe oclgrind-kernel
  DESTINATION bin)
install(TARGETS
  oclgrind oclgrind-rt oclgrind-rt-icd
  DESTINATION "lib${LIBDIR_SUFFIX}")
install(FILES
  ${CORE_HEADERS} ${CLC_HEADERS} LICENSE
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
