#ifndef __DEP_CHECKER__STDFILESYSTEM_H
#define __DEP_CHECKER__STDFILESYSTEM_H

#include <experimental/filesystem>

namespace adapted_namespace
{
typedef std::experimental::filesystem::path path;
typedef std::experimental::filesystem::directory_iterator directory_iterator;
typedef std::experimental::filesystem::recursive_directory_iterator recursive_directory_iterator;
}

using std::experimental::filesystem::current_path;
using std::experimental::filesystem::file_size;
using std::experimental::filesystem::is_directory;
using std::experimental::filesystem::is_regular_file;
using std::experimental::filesystem::is_regular_file;
using std::experimental::filesystem::remove;
using std::experimental::filesystem::rename;

#endif // __DEP_CHECKER__STDFILESYSTEM_H
