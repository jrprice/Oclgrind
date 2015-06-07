# CMakeLists.txt (Oclgrind)
# Copyright (c) 2013-2015, James Price and Simon McIntosh-Smith,
# University of Bristol. All rights reserved.
#
# This program is provided under a three-clause BSD license. For full
# license terms please see the LICENSE file distributed with this
# source code.

set(COMMON_SOURCES ../common/common.c ../common/common.h)
include_directories(../common)

# Add runtime tests
foreach(test
  map_buffer)

  add_executable(${test} ${test}.c ${COMMON_SOURCES})
  target_link_libraries(${test} oclgrind-rt)

  # Generate test binaries in same dir as Oclgrind libraries on Windows
  if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    set_target_properties(${test} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
  else()
    set_target_properties(${test} PROPERTIES LINKER_LANGUAGE CXX)
  endif()

  add_test(
    NAME rt_${test}
    COMMAND
    ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/run_test.py
    $<TARGET_FILE:${test}>)

  set_tests_properties(rt_${test} PROPERTIES DEPENDS ${test})

  # Set PCH directory
  set_tests_properties(rt_${test} PROPERTIES
      ENVIRONMENT "OCLGRIND_PCH_DIR=${CMAKE_BINARY_DIR}/include/oclgrind")

endforeach(${test})
