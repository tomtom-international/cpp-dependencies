#ifndef __DEP_CHECKER__FSTREAMINCLUDE_H
#define __DEP_CHECKER__FSTREAMINCLUDE_H

#if ( defined _MSC_VER && ( _MSC_VER > 1900 || _MSC_VER == 1900 )  ) || \
        ( defined __GNUC__ && (__GNUC__ > 5 || \
                                (__GNUC__ == 5 && \
                                    (__GNUC_MINOR__ > 3 || (__GNUC_MINOR__ == 3 && __GNUC_PATCHLEVEL__ > 0) ) ) ) )

#include <fstream>

namespace adapted_namespace
{
typedef std::ifstream ifstream;
typedef std::ofstream ofstream;
}

#else

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

namespace adapted_namespace
{
typedef boost::filesystem::ifstream ifstream;
typedef boost::filesystem::ofstream ofstream;
}

#endif

#endif // !__DEP_CHECKER__FSTREAMINCLUDE_H
