#include "FilesystemInclude.h"

class TemporaryWorkingDirectory
{
public:
  TemporaryWorkingDirectory(const char* name)
  {
    originalDir = filesystem::current_path();
    workDir = filesystem::temp_directory_path() / name;
    ASSERT(filesystem::create_directories(workDir));
    filesystem::current_path(workDir);
  }

  ~TemporaryWorkingDirectory()
  {
    filesystem::current_path(originalDir);
    filesystem::remove_all(workDir);
  }

  const filesystem::path& operator()() const
  {
    return workDir;
  }

private:
  filesystem::path originalDir;
  filesystem::path workDir;
};

