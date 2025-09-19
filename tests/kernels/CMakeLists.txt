# CMakeLists.txt (Oclgrind)
# Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
# University of Bristol. All rights reserved.
#
# This program is provided under a three-clause BSD license. For full
# license terms please see the LICENSE file distributed with this
# source code.

# Add kernel tests
file(READ TESTS KERNEL_TESTS)
string(REPLACE "\n" ";" KERNEL_TESTS ${KERNEL_TESTS})
foreach(test ${KERNEL_TESTS})
  add_test(
    NAME ${test}
    COMMAND
    ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/run_test.py
    $<TARGET_FILE:oclgrind-kernel>
    ${CMAKE_SOURCE_DIR}/tests/kernels/${test}.sim)
endforeach(${test})

# Set PCH directory
set_tests_properties(${KERNEL_TESTS} PROPERTIES
    ENVIRONMENT "OCLGRIND_PCH_DIR=${CMAKE_BINARY_DIR}/include/oclgrind")

# https://github.com/jrprice/Oclgrind/218
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" AND
    ${LLVM_VERSION_MAJOR} VERSION_EQUAL "18")
  set(XFAIL ${XFAIL} memcheck/static_array_padded_struct)
endif()

if (${LLVM_PACKAGE_VERSION} VERSION_LESS "19.0")
  set(XFAIL ${XFAIL} interactive/pointers)
  set(XFAIL ${XFAIL} interactive/struct_member)
endif()

# Expected failures
set_tests_properties(${XFAIL} PROPERTIES WILL_FAIL TRUE)
