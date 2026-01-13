#pragma once

#include <filesystem>

class TemporaryWorkingDirectory
{
public:
  TemporaryWorkingDirectory(const char* name)
  {
    originalDir = std::filesystem::current_path();
    workDir = std::filesystem::temp_directory_path() / name;
    ASSERT(std::filesystem::create_directories(workDir));
    std::filesystem::current_path(workDir);
  }

  ~TemporaryWorkingDirectory()
  {
    std::filesystem::current_path(originalDir);
    std::filesystem::remove_all(workDir);
  }

  const std::filesystem::path& operator()() const
  {
    return workDir;
  }

private:
  std::filesystem::path originalDir;
  std::filesystem::path workDir;
};

