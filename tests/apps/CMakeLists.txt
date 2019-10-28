# CMakeLists.txt (Oclgrind)
# Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
# University of Bristol. All rights reserved.
#
# This program is provided under a three-clause BSD license. For full
# license terms please see the LICENSE file distributed with this
# source code.

set(COMMON_SOURCES ../common/common.c ../common/common.h)
include_directories(../common)

# Add app tests
foreach(test
  image
  vecadd)

  add_executable(${test} ${test}/${test}.c ${COMMON_SOURCES})
  target_link_libraries(${test} oclgrind-rt)

  # Generate test binaries in same dir as Oclgrind libraries on Windows
  if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    set_target_properties(${test} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
  else()
    set_target_properties(${test} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${test}")
    set_target_properties(${test} PROPERTIES LINKER_LANGUAGE CXX)
  endif()

  add_test(
    NAME app_${test}
    COMMAND
    ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/run_test.py
    $<TARGET_FILE:oclgrind-exe>
    $<TARGET_FILE:${test}>)

  set_tests_properties(app_${test} PROPERTIES DEPENDS ${test})

  # Set PCH directory
  set(ENV "OCLGRIND_TESTING=1")
  list(APPEND ENV "OCLGRIND_PCH_DIR=${CMAKE_BINARY_DIR}/include/oclgrind")
  set_tests_properties(app_${test} PROPERTIES ENVIRONMENT "${ENV}")

endforeach(${test})
