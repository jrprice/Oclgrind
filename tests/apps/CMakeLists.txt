# CMakeLists.txt (Oclgrind)
# Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
# University of Bristol. All rights reserved.
#
# This program is provided under a three-clause BSD license. For full
# license terms please see the LICENSE file distributed with this
# source code.

# Add app tests
foreach(test
  vecadd)

  add_executable(${test} ${test}/${test}.c)
  target_link_libraries(${test} oclgrind)

  # Generate test binaries in same dir as Oclgrind libraries on Windows
  if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    add_test(app_${test} "${CMAKE_BINARY_DIR}/${test}")
    set_target_properties(${test} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
  else()
    add_test(app_${test} "${test}")
  endif()

  set_tests_properties(app_${test} PROPERTIES DEPENDS ${test})

endforeach(${test})
