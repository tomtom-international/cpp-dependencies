add_executable(unittests
  AnalysisCircularDependencies.cpp
  ConfigurationTest.cpp
  test.cpp
)
target_link_libraries(unittests
  PRIVATE
    cpp_dependencies_coverage
)

set_property(TARGET unittests
             APPEND PROPERTY LINK_FLAGS --coverage)
add_test(NAME unittests
         COMMAND unittests)
