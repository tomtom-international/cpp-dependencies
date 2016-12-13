cmake_minimum_required(VERSION 3.1)

project(cpp_dependencies LANGUAGES CXX)

# Enables running the unit tests
option(WITH_TESTS "Also build unit tests" ON)

# Running with Boost filesystem is typically faster, until platform specific std::filesystem comes out that is faster yet. 
# Note that Boost::filesystem needs to be installed for this to be used.
option(WITH_BOOST "Use Boost filesystem" OFF)

# Switch between using the mmap logic for reading files (faster, because one copy less) or a file read (slower, because a full copy, but portable).
option(WITH_MMAP "Use mmapped files" ON)

# Whether your platform provides a fast memrchr function. If it does not, turn this off and a slower replacement will be used.
option(HAS_MEMRCHR "Platform has memrchr function" ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES GNU)
  set(COMPILE_FLAGS -Wall -Wextra -Wpedantic)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(COMPILE_FLAGS /W4)
  # boost gets compiled as static libs on Windows
  set(Boost_USE_STATIC_LIBS ON)
endif()

if(WITH_MMAP)
  list(APPEND COMPILE_FLAGS -DWITH_MMAP)
endif()

if(NOT HAS_MEMRCHR)
  list(APPEND COMPILE_FLAGS -DNO_MEMRCHR)
endif()

if(WITH_BOOST)
  list(APPEND COMPILE_FLAGS -DWITH_BOOST)
  find_package(Boost COMPONENTS filesystem system REQUIRED)
  set(FILESYSTEM_LIBS ${Boost_LIBRARIES})
else()
  if(NOT WIN32)
    set(FILESYSTEM_LIBS stdc++fs)
  endif()
endif()

add_subdirectory(src)

if(WITH_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
