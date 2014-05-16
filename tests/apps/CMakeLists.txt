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
