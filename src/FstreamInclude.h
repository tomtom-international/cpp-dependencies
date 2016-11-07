#ifndef __DEP_CHECKER__FSTREAMINCLUDE_H
#define __DEP_CHECKER__FSTREAMINCLUDE_H

#ifdef WITH_BOOST

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

namespace streams = boost::filesystem;

#else

#include <fstream>

namespace streams = std;

#endif

#endif
