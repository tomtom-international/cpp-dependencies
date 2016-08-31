#ifndef __DEP_CHECKER__BOOSTFILESYSTEM_H
#define __DEP_CHECKER__BOOSTFILESYSTEM_H

#include <boost/filesystem.hpp>

namespace adapted_namespace
{
typedef boost::filesystem::path path;
typedef boost::filesystem::directory_iterator directory_iterator;
typedef boost::filesystem::recursive_directory_iterator recursive_directory_iterator;
}

using boost::filesystem::current_path;
using boost::filesystem::file_size;
using boost::filesystem::is_directory;
using boost::filesystem::is_regular_file;
using boost::filesystem::is_regular_file;
using boost::filesystem::remove;
using boost::filesystem::rename;

#endif // __DEP_CHECKER__BOOSTFILESYSTEM_H
