# CMakeLists.txt (Oclgrind)
# Copyright (c) 2013-2016, James Price and Simon McIntosh-Smith,
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

if ("${LLVM_VERSION_MAJOR}" STREQUAL "4")
  set(XFAIL
      ${XFAIL}
      bugs/byval_function_argument
      bugs/const_gep_expr_pointee_type
      bugs/multidim_array_in_struct
      memcheck/static_array_padded_struct
      misc/array
      uninitialized/private_array_initializer_list
  )
endif()

# Can't seem to get DIGlobalVariable in LLVM 3.9
if ("${LLVM_VERSION_MAJOR}" STRLESS "4")
  set(XFAIL ${XFAIL} interactive/struct_member)
endif()

# LLVM bug, exposed in 4.0, fixed in 5.0
if ("${LLVM_VERSION_MAJOR}" STREQUAL "4")
  set(XFAIL ${XFAIL} bugs/llvm_memcpyopt_bug)
endif()

# Expected failures
set_tests_properties(${XFAIL} PROPERTIES WILL_FAIL TRUE)
