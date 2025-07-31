#ifndef __DEP_CHECKER__FILESYSTEMINCLUDE_H
#define __DEP_CHECKER__FILESYSTEMINCLUDE_H

#ifdef WITH_BOOST

    #include <boost/filesystem.hpp>
    namespace filesystem = boost::filesystem;

#else
#   if __cplusplus >= 201703L
    #include <filesystem>
    namespace filesystem = std::filesystem;

    // Code specific to C++17 or later
    #define HAS_CPP17_FEATURES
#   elif __cplusplus >= 201103L
    #include <experimental/filesystem>
    namespace filesystem = std::experimental::filesystem;

    // Code specific to C++11 or later (but not C++17)
    #define HAS_CPP11_FEATURES
#   else
    // Code for older C++ versions (e.g., C++98/03)
    #pragma error("C++ version is too old, please use C++11 or later.")
#   endif

#endif

#endif
