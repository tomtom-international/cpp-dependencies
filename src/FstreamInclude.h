#ifndef __DEP_CHECKER__FSTREAM_H
#define __DEP_CHECKER__FSTREAM_H

#ifdef _MSC_VER

#if ( _MSC_VER < 1900 )

#include <experimental/filesystem>

namespace adapted_namespace
{
typedef boost::filesystem::ifstream ifstream;
typedef boost::filesystem::ofstream ifstream;
}

#else

#include <fstream>

namespace adapted_namespace
{
typedef std::ifstream ifstream;
typedef std::ofstream ofstream ;
}

#endif

#endif // _MSC_VER

#ifdef __GNUC__

#if __GNUC__ > 5 || \
    (__GNUC__ == 5 && \
    (__GNUC_MINOR__ > 3 || (__GNUC_MINOR__ == 3 && __GNUC_PATCHLEVEL__ > 0)))

#include <fstream>

namespace adapted_namespace
{
typedef std::ifstream ifstream;
typedef std::ofstream ofstream;
}

#else

#include <boost/filesystem/fstream.hpp>

namespace adapted_namespace
{
typedef boost::filesystem::ifstream ifstream;
typedef boost::filesystem::ofstream ifstream;
}

#endif

#endif // __GNUC__

#endif // !__DEP_CHECKER__FSTREAM_H
