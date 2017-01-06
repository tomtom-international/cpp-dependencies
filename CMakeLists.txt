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

# Default package format to use when building a package with CPack
if(APPLE)
  set(DEFAULT_CPACK_GENERATOR "DragNDrop")
elseif(UNIX)
  set(DEFAULT_CPACK_GENERATOR "DEB")
elseif(WIN32)
  set(DEFAULT_CPACK_GENERATOR "NSIS")
endif()
set(CPACK_GENERATOR "${DEFAULT_CPACK_GENERATOR}" CACHE STRING "Package type to generate with CPack")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES GNU)
  include(CheckCXXCompilerFlag)

  set(COMPILE_FLAGS -Wall -Wextra)

  check_cxx_compiler_flag("-Wpedantic" PEDANTIC_SUPPORTED)
  if(PEDANTIC_SUPPORTED)
    list(APPEND COMPILE_FLAGS -Wpedantic)
  endif()
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

set(CPACK_PACKAGE_NAME cpp-dependencies)
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 1)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_PACKAGE_VENDOR "TomTom International BV")
set(CPACK_PACKAGE_CONTACT "${CPACK_PACKAGE_VENDOR}")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Tool to check C++ #include dependencies
 The tool cpp-dependencies creates #include dependency information for C++
 source code files, which it derives from scanning a full source tree.
 .
 The dependency information is output as .dot files, which can be visualized
 in, for example, GraphViz.")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE https://github.com/tomtom-international/cpp-dependencies)
set(CPACK_DEBIAN_PACKAGE_SUGGESTS graphviz)
set(CPACK_DEBIAN_PACKAGE_SECTION devel)

if(CPACK_GENERATOR STREQUAL DEB)
  find_program(DPKG_CMD dpkg REQUIRED)
  if(NOT DPKG_CMD)
    message(STATUS "Can not find dpkg in your path, default to i386.")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE i386)
  else()
    execute_process(COMMAND "${DPKG_CMD}" --print-architecture
      OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )
  endif()

  # Because the default package name produced by CPack fails to meet Debian packaging conventions
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
endif()

include(CPack)
