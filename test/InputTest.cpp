#include "test.h"
#include "TestUtils.h"

#include "Component.h"
#include "Configuration.h"
#include "Constants.h"
#include "Input.h"
#include "FilesystemInclude.h"
#include "FstreamInclude.h"
#include <sstream>

void CreateCMakeProject(const std::string& projectName,
                        const std::string& alias,
                        const filesystem::path& workDir)
{
  filesystem::create_directories(workDir / projectName);
  streams::ofstream out(workDir / projectName / "CMakeLists.txt");
  out << "project(" << projectName << ")\n"
      << alias << "(${PROJECT_NAME}\n"
      << "  somesourcefile.cpp\n"
      << ")\n";
}

TEST(Input_Aliases)
{
  TemporaryWorkingDirectory workDir(name);

  CreateCMakeProject("Renderer", "add_render_library", workDir());
  CreateCMakeProject("UI", "add_ui_library", workDir());
  CreateCMakeProject("DrawTest", "add_test", workDir());
  CreateCMakeProject("Service", "add_service", workDir());

  {
    streams::ofstream out(workDir() / "CMakeLists.txt");
    out << "add_subdirectory(Renderer)\n"
        << "add_subdirectory(UI)\n"
        << "add_subdirectory(DrawTest)\n"
        << "add_subdirectory(Service)\n";
  }

  std::stringstream ss;
  ss << "addLibraryAlias: [\n"
     << "  add_render_library\n"
     << "  add_ui_library\n"
     << "  ]\n"
     << "addExecutableAlias: [\n"
     << "  add_service\n"
     << "  add_test\n"
     << "  ]\n";

  Configuration config;
  config.read(ss);

  std::unordered_map<std::string, Component*> components;
  std::unordered_map<std::string, File> files;

  LoadFileList(config, components, files, workDir(), true, false);

  ASSERT(components.size() == 5);

  for (auto& pair : components) {
    const Component& comp = *pair.second;

    if (comp.CmakeName() == "Renderer") {
      ASSERT(comp.type == "add_render_library");
    } else if (comp.CmakeName() == "UI") {
      ASSERT(comp.type == "add_ui_library");
    } else if (comp.CmakeName() == "DrawTest") {
      ASSERT(comp.type == "add_test");
    } else if (comp.CmakeName() == "Service") {
      ASSERT(comp.type == "add_service");
    } else {
      ASSERT(comp.CmakeName() == "ROOT");
    }
  }
}
