add_library(cpp_dependencies_lib STATIC
  Analysis.h
  CmakeRegen.h
  Component.h
  Configuration.h
  Constants.h
  Input.h
  Output.h

  Analysis.cpp
  CmakeRegen.cpp
  Component.cpp
  Configuration.cpp
  generated.cpp
  Input.cpp
  Output.cpp
)
target_compile_options(cpp_dependencies_lib
  PUBLIC 
    ${COMPILE_FLAGS}
    -O3
)
target_link_libraries(cpp_dependencies_lib
  PRIVATE 
    ${FILESYSTEM_LIBS}
)
target_include_directories(cpp_dependencies_lib
  PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}
    ${Boost_INCLUDE_DIRS}
)

add_library(cpp_dependencies_coverage STATIC
  Analysis.h
  CmakeRegen.h
  Component.h
  Configuration.h
  Constants.h
  Input.h
  Output.h

  Analysis.cpp
  CmakeRegen.cpp
  Component.cpp
  Configuration.cpp
  generated.cpp
  Input.cpp
  Output.cpp
)
target_compile_options(cpp_dependencies_coverage
  PUBLIC 
    ${COMPILE_FLAGS}
    --coverage
)
target_link_libraries(cpp_dependencies_coverage
  PRIVATE 
    ${FILESYSTEM_LIBS}
)
target_include_directories(cpp_dependencies_coverage
  PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}
    ${Boost_INCLUDE_DIRS}
)

add_executable(cpp-dependencies
  main.cpp
)
target_link_libraries(cpp-dependencies
  PRIVATE
    cpp_dependencies_lib
)

install(TARGETS cpp-dependencies RUNTIME DESTINATION bin)
