# CMakeLists.txt (Oclgrind)
# Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
# University of Bristol. All rights reserved.
#
# This program is provided under a three-clause BSD license. For full
# license terms please see the LICENSE file distributed with this
# source code.

set(COMMON_SOURCES ../common/common.c ../common/common.h)
include_directories(../common)

# Add runtime tests
foreach(test
  build_program
  kernel_scope_local_mem_usage
  map_buffer
  multqueues
  sampler)

  add_executable(${test} ${test}.c ${COMMON_SOURCES})
  target_compile_definitions(${test} PRIVATE
                             "-DROOT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
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
    $<TARGET_FILE:oclgrind-exe>
    $<TARGET_FILE:${test}>)

  set_tests_properties(rt_${test} PROPERTIES DEPENDS ${test})

  # Set PCH directory
  set(ENV "OCLGRIND_TESTING=1")
  list(APPEND ENV "OCLGRIND_PCH_DIR=${CMAKE_BINARY_DIR}/include/oclgrind")
  set_tests_properties(rt_${test} PROPERTIES ENVIRONMENT "${ENV}")

endforeach(${test})
