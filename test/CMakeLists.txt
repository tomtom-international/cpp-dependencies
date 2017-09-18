add_executable(unittests
  AnalysisCircularDependencies.cpp
  CmakeRegenTest.cpp
  ConfigurationTest.cpp
  InputTest.cpp
  test.cpp
)
target_link_libraries(unittests
  PRIVATE
    cpp_dependencies_lib
)
target_compile_definitions(unittests
  PRIVATE
    _CRT_SECURE_NO_WARNINGS
)

if (NOT MSVC AND BUILD_COVERAGE)
  set_property(TARGET unittests APPEND PROPERTY LINK_FLAGS --coverage)
endif ()
add_test(NAME unittests COMMAND unittests)
