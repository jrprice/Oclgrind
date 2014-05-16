# Add app tests
foreach(test
  vecadd)

  add_executable(${test} ${test}/${test}.c)
  target_link_libraries(${test} oclgrind)

  add_test(app_${test} "${CMAKE_BINARY_DIR}/${test}")
  set_tests_properties(app_${test} PROPERTIES DEPENDS ${test})

  # Generate test binaries in same dir as Oclgrind libraries on Windows
  if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    set_target_properties(${test} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
  endif()

endforeach(${test})
