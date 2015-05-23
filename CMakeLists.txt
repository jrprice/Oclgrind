# CMakeLists.txt (Oclgrind)
# Copyright (c) 2013-2015, James Price and Simon McIntosh-Smith,
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
include(CheckLibraryExists)

# Enable C99 for GCC (required for tests)
if (CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
endif()

# Enable rpath on OS X
set(CMAKE_MACOSX_RPATH 1)

# Enable C++11 for Clang/GCC
if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
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
set(CLANG ${LLVM_TOOLS_BINARY_DIR}/clang)

# Get LLVM libraries for linking
llvm_map_components_to_libnames(LLVM_LIBS
  bitreader bitwriter core instrumentation ipo irreader
  linker mcparser objcarcopts option)


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
  src/plugins/RaceDetector.cpp)
target_link_libraries(oclgrind ${CORE_EXTRA_LIBS}
  clangAnalysis clangAST clangBasic clangCodeGen clangDriver clangEdit
  clangFrontend clangLex clangParse clangSema clangSerialization
  ${LLVM_LIBS})

# Sources for OpenCL runtime API frontend
set(RUNTIME_SOURCES
  src/runtime/async_queue.h
  src/runtime/async_queue.cpp
  src/runtime/icd.h
  src/runtime/runtime.cpp)

# Add ICD exports on Windows
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  list(APPEND RUNTIME_SOURCES src/runtime/icd.def)
endif()

add_library(oclgrind-rt-icd SHARED ${RUNTIME_SOURCES})
set_target_properties(oclgrind-rt-icd PROPERTIES COMPILE_FLAGS -DOCLGRIND_ICD)
target_link_libraries(oclgrind-rt-icd ${CMAKE_DL_LIBS} oclgrind)

# Add runtime exports on Windows
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  list(APPEND RUNTIME_SOURCES src/runtime/runtime.def)
endif()

add_library(oclgrind-rt SHARED ${RUNTIME_SOURCES})
target_link_libraries(oclgrind-rt ${CMAKE_DL_LIBS} oclgrind)

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
    -cc1 -x cl -cl-std=CL1.2 -O0 -g -fno-builtin
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
    -cc1 -x cl -cl-std=CL1.2 -O0 -g -fno-builtin
    -emit-pch -triple spir64-unknown-unknown
    -relocatable-pch -isysroot ${CMAKE_BINARY_DIR}/include/oclgrind/
    ${CMAKE_BINARY_DIR}/include/oclgrind/clc.h
    -o include/oclgrind/clc64.pch
  DEPENDS include/oclgrind/clc.h
)


# Generate config.h
configure_file("cmake_config.h.in" "config.h")


# Install oclgrind script if not on Windows
if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  file(READ src/runtime/oclgrind OCLGRIND_SCRIPT)
  string(REGEX REPLACE
    "__VERSION__" "${Oclgrind_VERSION_MAJOR}.${Oclgrind_VERSION_MINOR}"
    OCLGRIND_SCRIPT "${OCLGRIND_SCRIPT}")
  file(WRITE ${CMAKE_BINARY_DIR}/oclgrind "${OCLGRIND_SCRIPT}")

  # Generate ICD loader
  get_property(OCLGRIND_RT_FILENAME TARGET oclgrind-rt-icd PROPERTY LOCATION)
  file(WRITE ${CMAKE_BINARY_DIR}/oclgrind.icd "${OCLGRIND_RT_FILENAME}\n")

  install(PROGRAMS
    ${CMAKE_BINARY_DIR}/oclgrind
    DESTINATION bin)
endif()

install(TARGETS
  oclgrind-kernel
  DESTINATION bin)
install(TARGETS
  oclgrind oclgrind-rt oclgrind-rt-icd
  DESTINATION lib)
install(FILES
  ${CORE_HEADERS} ${CMAKE_BINARY_DIR}/config.h ${CLC_HEADERS} LICENSE
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

  # Add kernel tests
  file(READ tests/kernels/TESTS KERNEL_TESTS)
  string(REPLACE "\n" ";" KERNEL_TESTS ${KERNEL_TESTS})
  foreach(test ${KERNEL_TESTS})
    add_test(
      NAME ${test}
      COMMAND
      ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/kernels/run_kernel_test.py
      $<TARGET_FILE:oclgrind-kernel>
      ${CMAKE_SOURCE_DIR}/tests/kernels/${test}.sim)
  endforeach(${test})

  # Set PCH directory
  set_tests_properties(${KERNEL_TESTS} PROPERTIES
      ENVIRONMENT "OCLGRIND_PCH_DIR=${CMAKE_BINARY_DIR}/include/oclgrind")

  # Expected failures
  set_tests_properties(
    atomics/atomic_intergroup_race
    data-race/intragroup_hidden_race
    PROPERTIES WILL_FAIL TRUE)

else()
  message(WARNING "Kernel tests will not be run (Python required)")
endif()

# Add app tests
add_subdirectory(tests/apps)
