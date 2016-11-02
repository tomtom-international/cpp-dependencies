#ifndef __DEP_CHECKER__FILESYSTEMINCLUDE_H
#define __DEP_CHECKER__FILESYSTEMINCLUDE_H

#ifdef USE_BOOST

#include <boost/filesystem.hpp>

namespace filesystem = boost::filesystem;

#else

#include <experimental/filesystem>

namespace filesystem = std::experimental::filesystem;

#endif

#endif
