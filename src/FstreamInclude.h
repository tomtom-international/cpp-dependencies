#ifndef __DEP_CHECKER__FSTREAM_H
#define __DEP_CHECKER__FSTREAM_H

#ifdef _MSC_VER

#if ( _MSC_VER < 1900 )

#include <experimental/filesystem>

using boost::filesystem::ifstream;
using boost::filesystem::ofstream;

#else

#include <fstream>

using std::ifstream;
using std::ofstream;

#endif

#endif // _MSC_VER

#ifdef __GNUC__

#if __GNUC__ > 5 || \
    (__GNUC__ == 5 &&
        (__GNUC_MINOR__ > 3 || (__GNUC_MINOR__ == 3 && __GNUC_PATCHLEVEL__ > 0)))

#include <fstream>

using std::ifstream;
using std::ofstream;


#else

#include <experimental/filesystem>

using boost::filesystem::ifstream;
using boost::filesystem::ofstream;

#endif

#endif // __GNUC__

#endif // !__DEP_CHECKER__FSTREAM_H
