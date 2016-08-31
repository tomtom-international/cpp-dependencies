#ifndef __DEP_CHECKER__FILESYSTEM_H
#define __DEP_CHECKER__FILESYSTEM_H

#ifdef _MSC_VER

#if ( _MSC_VER < 1900 )
#include <boost/filesystem.hpp>

using boost::filesystem::path;
using boost::filesystem::is_directory;
using boost::filesystem::is_regular_file;

#else
#include <experimental/filesystem>

using std::experimental::filesystem::path;
using std::experimental::filesystem::is_directory;
using std::experimental::filesystem::is_regular_file;

#endif

#endif // _MSC_VER

#ifdef __GNUC__

#if __GNUC__ > 5 ||
    (__GNUC__ == 5 &&
        (__GNUC_MINOR__ > 3 || (__GNUC_MINOR__ == 3 && __GNUC_PATCHLEVEL__ > 0)))

#include <experimental/filesystem>

using std::experimental::filesystem::path;
using std::experimental::filesystem::is_directory;
using std::experimental::filesystem::is_regular_file;

#else

#include <boost/filesystem.hpp>

using boost::filesystem::path;
using boost::filesystem::is_directory;
using boost::filesystem::is_regular_file;

#endif

#endif // __GNUC__

#endif // !__DEP_CHECKER__FILESYSTEM_H
