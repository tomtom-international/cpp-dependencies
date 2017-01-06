include(WriteCompilerDetectionHeader)

write_compiler_detection_header(
  FILE "${CMAKE_CURRENT_BINARY_DIR}/compiler_feature_detection.h"
  PREFIX OPTIONAL_FEATURE
  COMPILERS GNU Clang AppleClang MSVC
  FEATURES
    cxx_override
)

add_executable(unittests
  AnalysisCircularDependencies.cpp
  test.cpp
)
target_link_libraries(unittests
  PRIVATE
    cpp_dependencies_coverage
)
target_include_directories(unittests
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

set_property(TARGET unittests APPEND PROPERTY LINK_FLAGS --coverage)
add_test(NAME unittests COMMAND unittests)
