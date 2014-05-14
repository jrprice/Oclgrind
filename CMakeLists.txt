cmake_minimum_required(VERSION 2.6)
project(Oclgrind)
set(Oclgrind_VERSION_MAJOR 14)
set(Oclgrind_VERSION_MINOR X)

# Warn if not using Windows
if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  message(WARNING
"CMake build on non-Windows platform detected.\n"
"Recommended build system for Unix is 'configure' and 'make'.")
endif()

configure_file("cmake_config.h.in" "config.h")

# Set RPATH
set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list(FIND
  CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
  "${CMAKE_INSTALL_PREFIX}/lib"
  isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif("${isSystemDir}" STREQUAL "-1")

# Warn if on OS X and CMake doesn't support CMAKE_MACOSX_RPATH
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin" AND
    "${isSystemDir}" STREQUAL "-1" AND
     ${CMAKE_VERSION} STRLESS 2.8.12)
  message(WARNING
"RPATH not set for installed files on OS X with CMake version < 2.8.12.\n"
"This means that you will need to add the build directory to your "
"DYLD_LIBRARY_PATH.")
endif()

# Add LLVM required macro definitions
add_definitions(-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS)

# Set INSTALL_ROOT (used by spirsim::Program for finding PCH files)
add_definitions(-DINSTALL_ROOT=\"${CMAKE_INSTALL_PREFIX}\")

# Require that SPIR_ROOT is set and pointing to a Clang/SPIR generator
set(SPIR_ROOT " " CACHE PATH "Root of Clang/SPIR generator.")
if (NOT EXISTS ${SPIR_ROOT}/include/clang)
  message(FATAL_ERROR "Clang/SPIR generator not found (set SPIR_ROOT).")
endif()

# Add LLVM/Clang libraries and includes from SPIR generator
include_directories(${SPIR_ROOT}/include)
link_directories(${SPIR_ROOT}/lib)

# Generate stringified clc.h
add_custom_command(
  OUTPUT src/spirsim/clc_h.cpp
  COMMAND ${CMAKE_COMMAND} -DSOURCE_FILE=${CMAKE_SOURCE_DIR}/src/spirsim/clc.h
    -P ${CMAKE_SOURCE_DIR}/src/spirsim/gen_clc_h.cmake
  DEPENDS src/spirsim/clc.h src/spirsim/gen_clc_h.cmake
)

include_directories("src/" "${PROJECT_BINARY_DIR}")

if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  set(SPIRSIM_LIB_TYPE "SHARED")
  set(SPIRSIM_EXTRA_LIBS "${CMAKE_DL_LIBS}")

  if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    list(APPEND SPIRSIM_EXTRA_LIBS "pthread")
  endif()
endif()

set(SPIRSIM_HEADERS
  src/spirsim/clc.h
  src/spirsim/common.h
  src/spirsim/Device.h
  src/spirsim/half.h
  src/spirsim/Kernel.h
  src/spirsim/Memory.h
  src/spirsim/Program.h
  src/spirsim/Queue.h
  src/spirsim/WorkItem.h
  src/spirsim/WorkGroup.h)

add_library(spirsim ${SPIRSIM_LIB_TYPE}
  ${SPIRSIM_HEADERS}
  src/spirsim/clc_h.cpp
  src/spirsim/common.cpp
  src/spirsim/Device.cpp
  src/spirsim/Kernel.cpp
  src/spirsim/Memory.cpp
  src/spirsim/Program.cpp
  src/spirsim/Queue.cpp
  src/spirsim/WorkItem.cpp
  src/spirsim/WorkItemBuiltins.cpp
  src/spirsim/WorkGroup.cpp)
target_link_libraries(spirsim ${SPIRSIM_EXTRA_LIBS}
  clangAnalysis clangAST clangBasic clangCodeGen clangDriver clangEdit
  clangFrontend clangLex clangParse clangSema clangSerialization
  LLVMBitReader LLVMBitWriter LLVMAsmParser LLVMInstrumentation
  LLVMipo LLVMipa LLVMInstCombine LLVMLinker LLVMMC LLVMMCParser
  LLVMScalarOpts LLVMTarget LLVMTransformUtils LLVMVectorize
  LLVMAnalysis LLVMCore LLVMSupport)

# Sources for OpenCL runtime API frontend
set(OCLGRIND_SOURCES
  src/oclgrind/async_queue.h
  src/oclgrind/async_queue.cpp
  src/oclgrind/icd.h
  src/oclgrind/runtime.cpp)

# Add ICD exports on Windows
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  list(APPEND OCLGRIND_SOURCES src/oclgrind/icd.def)
endif()

add_library(oclgrind SHARED ${OCLGRIND_SOURCES})
target_link_libraries(oclgrind ${CMAKE_DL_LIBS} spirsim)

add_library(oclgrind-icd SHARED ${OCLGRIND_SOURCES})
set_target_properties(oclgrind-icd PROPERTIES COMPILE_FLAGS -DOCLGRIND_ICD)
target_link_libraries(oclgrind-icd ${CMAKE_DL_LIBS} spirsim)

add_executable(oclgrind-kernel
  src/oclgrind-kernel/main.cpp
  src/oclgrind-kernel/Simulation.h
  src/oclgrind-kernel/Simulation.cpp)
target_link_libraries(oclgrind-kernel spirsim)

# Install oclgrind script if not on Windows
if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  file(READ src/oclgrind/oclgrind OCLGRIND_SCRIPT)
  string(REGEX REPLACE
    "__ROOT__" "${CMAKE_INSTALL_PREFIX}"
    OCLGRIND_SCRIPT "${OCLGRIND_SCRIPT}")
  string(REGEX REPLACE
    "__VERSION__" "${Oclgrind_VERSION_MAJOR}.${Oclgrind_VERSION_MINOR}"
    OCLGRIND_SCRIPT "${OCLGRIND_SCRIPT}")
  file(WRITE oclgrind "${OCLGRIND_SCRIPT}")

  install(PROGRAMS
    oclgrind
    DESTINATION bin)
endif()

install(TARGETS
  oclgrind-kernel
  DESTINATION bin)
install(TARGETS
  oclgrind oclgrind-icd spirsim
  DESTINATION lib)
install(FILES
  ${SPIRSIM_HEADERS}
  DESTINATION include/spirsim
)

# Generate precompiled headers for clc.h
install(CODE "execute_process(COMMAND
  \"${SPIR_ROOT}/bin/clang\"
  -cc1 -x cl -g -emit-pch -triple spir64-unknown-unknown
  \"${CMAKE_INSTALL_PREFIX}/include/spirsim/clc.h\"
  -o \"${CMAKE_INSTALL_PREFIX}/include/spirsim/clc.pch\"
)")
install(CODE "execute_process(COMMAND
  \"${SPIR_ROOT}/bin/clang\"
  -cc1 -x cl -O0 -g -emit-pch -triple spir64-unknown-unknown
  \"${CMAKE_INSTALL_PREFIX}/include/spirsim/clc.h\"
  -o \"${CMAKE_INSTALL_PREFIX}/include/spirsim/clc.noopt.pch\"
)")


# Tests

# Check for Python
find_package(PythonInterp)
if (PYTHONINTERP_FOUND)
  enable_testing()
else()
  add_custom_target(test COMMENT "Python required for running tests.")
endif()

# Add kernel tests
file(READ tests/kernels/TESTS KERNEL_TESTS)
string(REPLACE "\n" ";" KERNEL_TESTS ${KERNEL_TESTS})
foreach(test ${KERNEL_TESTS})
  add_test(${test}
    ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/kernels/run_kernel_test.py
    ${CMAKE_SOURCE_DIR}/tests/kernels/${test}/${test}.sim)
endforeach(${test})

# Expected failures
set_tests_properties(
  atomic_intergroup_race
  atomic_cmpxchg_race
  PROPERTIES WILL_FAIL TRUE)
