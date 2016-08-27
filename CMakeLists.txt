cmake_minimum_required(VERSION 3.1)

project(cpp_dependencies
        LANGUAGES CXX)

option(WITH_TESTS "Also build unit tests" ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES GNU)
    set(${PROJECT_NAME}_COMPILE_OPTIONS -Wall -Wextra -Wpedantic)
    set(${PROJECT_NAME}_TEST_COMPILE_OPTIONS --coverage)
elseif(${MSVC})
    set(${PROJECT_NAME}_COMPILE_OPTIONS /W4)
    # boost get's compiled as static libs on Windows
    set(Boost_USE_STATIC_LIBS ON)
endif()

find_package(Boost
             COMPONENTS filesystem system
             REQUIRED)

add_subdirectory(src)

if(${WITH_TESTS})
    enable_testing()
    add_subdirectory(test)
endif()
