add_executable(unittests
  AnalysisCircularDependencies.cpp
  test.cpp
)
target_link_libraries(unittests
  PRIVATE
    cpp_dependencies_coverage
)
target_compile_definitions(unittests
  PRIVATE
    _CRT_SECURE_NO_WARNINGS
)

set_property(TARGET unittests APPEND PROPERTY LINK_FLAGS --coverage)
add_test(NAME unittests COMMAND unittests)
