cmake_minimum_required(VERSION 3.1)

project(cpp_dependencies LANGUAGES CXX)

include(CTest)

# Coverage build doesn't work with MSVC
option(BUILD_COVERAGE "Build cpp_dependencies for coverage" OFF)

if (WIN32)
  set(DEFAULT_BOOST OFF)
else()
  set(DEFAULT_BOOST ON)
endif()

# Running with Boost filesystem is typically faster, until platform specific std::filesystem comes out that is faster yet. 
# Note that Boost::filesystem needs to be installed for this to be used.
option(WITH_BOOST "Use Boost filesystem" ${DEFAULT_BOOST})

if (WIN32)
  set(DEFAULT_MMAP OFF)
else()
  set(DEFAULT_MMAP ON)
endif()

# Switch between using the mmap logic for reading files (faster, because one copy less) or a file read (slower, because a full copy, but portable).
option(WITH_MMAP "Use mmapped files" ${DEFAULT_MMAP})

if (WIN32 OR APPLE)
  set(DEFAULT_MEMRCHR OFF)
else()
  set(DEFAULT_MEMRCHR ON)
endif()

# Whether your platform provides a fast memrchr function. If it does not, turn this off and a slower replacement will be used.
option(HAS_MEMRCHR "Platform has memrchr function" ${DEFAULT_MEMRCHR})

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

if(BUILD_TESTING)
  add_subdirectory(test)
endif()

set(CPACK_PACKAGE_NAME cpp-dependencies)
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

# Version: update this before or while tagging a new release, for out of repo builds
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 1)
set(CPACK_PACKAGE_VERSION_PATCH 0)

# Automatically detect package version to use from git
find_package(Git)
if(Git_FOUND OR GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --long --dirty --always
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE DESCRIBE_RESULT
    OUTPUT_VARIABLE DESCRIBE_STDOUT
    )
  if(DESCRIBE_RESULT EQUAL 0)
    string(STRIP "${DESCRIBE_STDOUT}" DESCRIBE_STDOUT)
    message(STATUS "Git reported this project's version as '${DESCRIBE_STDOUT}'")
    if(DESCRIBE_STDOUT MATCHES "^(.*)-(dirty)$")
      set(DESCRIBE_DIRTY "${CMAKE_MATCH_2}")
      set(DESCRIBE_STDOUT "${CMAKE_MATCH_1}")
    endif()
    if(DESCRIBE_STDOUT MATCHES "^([0-9a-f]+)$")
      set(DESCRIBE_COMMIT_NAME "${CMAKE_MATCH_1}")
      set(DESCRIBE_STDOUT "")
    elseif(DESCRIBE_STDOUT MATCHES "^(.*)-g([0-9a-f]+)$")
      set(DESCRIBE_COMMIT_NAME "${CMAKE_MATCH_2}")
      set(DESCRIBE_STDOUT "${CMAKE_MATCH_1}")
    endif()
    if(DESCRIBE_STDOUT MATCHES "^(.*)-([0-9]+)$")
      set(DESCRIBE_COMMIT_COUNT "${CMAKE_MATCH_2}")
      set(DESCRIBE_TAG "${CMAKE_MATCH_1}")
      set(DESCRIBE_STDOUT "")
    endif()
    if("${DESCRIBE_TAG}.0.0" MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+).*$")
      set(CPACK_PACKAGE_VERSION_MAJOR "${CMAKE_MATCH_1}")
      set(CPACK_PACKAGE_VERSION_MINOR "${CMAKE_MATCH_2}")
      set(CPACK_PACKAGE_VERSION_PATCH "${CMAKE_MATCH_3}")
    endif()
    if(DESCRIBE_COMMIT_COUNT GREATER 0)
      # Make it a pre-release version of the next patch release
      math(EXPR CPACK_PACKAGE_VERSION_PATCH "${CPACK_PACKAGE_VERSION_PATCH} + 1")
    endif()

    set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
    set(CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}")

    # Now for the rest: format global CPack version according to semver.org, Debian so that it'll get proper sorting for dpkg
    if(DESCRIBE_COMMIT_COUNT GREATER 0)
      set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}-${DESCRIBE_COMMIT_COUNT}")
      set(CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_DEBIAN_PACKAGE_VERSION}~${DESCRIBE_COMMIT_COUNT}")
    endif()

    set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}+g${DESCRIBE_COMMIT_NAME}")
    set(CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_DEBIAN_PACKAGE_VERSION}+g${DESCRIBE_COMMIT_NAME}")

    if(DESCRIBE_DIRTY)
      string(TIMESTAMP DESCRIBE_DIRTY_TIMESTAMP "%Y%m%d%H%M%S" UTC)
      set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}.dirty.${DESCRIBE_DIRTY_TIMESTAMP}")
      set(CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_DEBIAN_PACKAGE_VERSION}+dirty${DESCRIBE_DIRTY_TIMESTAMP}")
    endif()
  else()
    message(WARNING "Git failed to report the version")
  endif()
endif()

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

  if(NOT DEFINED CPACK_DEBIAN_PACKAGE_VERSION)
    set(CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
  endif()

  # Because the default package name produced by CPack fails to meet Debian packaging conventions
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_DEBIAN_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
endif()

include(CPack)
