/*
 * Copyright (C) 2012-2016. TomTom International BV (http://tomtom.com).
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "CmakeRegen.h"
#include "Component.h"
#include "Configuration.h"
#include "FstreamInclude.h"
#include "FilesystemInclude.h"
#include "Input.h"
#include <list>
#include <set>


static bool FilesAreDifferent(const filesystem::path &a, const filesystem::path &b) {
    if (filesystem::file_size(a) != filesystem::file_size(b)) {
        return true;
    }
    streams::ifstream as(a), bs(b);
    std::string l1, l2;
    while (as.good() && bs.good()) {
        getline(as, l1);
        getline(bs, l2);
        if (l1 != l2) {
            return true;
        }
    }
    return !(as.eof() && bs.eof());
}

void MakeCmakeComment(std::string& cmakeComment, const std::string& contents)
{
    cmakeComment.reserve(contents.size());
    size_t lastPos = 0, findPos;
    while ((findPos = contents.find('\n', lastPos)) != std::string::npos) {
        cmakeComment.append("# ");
        cmakeComment.append(contents, lastPos, findPos - lastPos + 1);
        lastPos = findPos + 1;
    }
    if (lastPos < contents.size()) {
        cmakeComment.append("# ");
        cmakeComment.append(contents, lastPos, std::string::npos);
        cmakeComment.append("\n");
    }
}

void RegenerateCmakeFilesForComponent(const Configuration& config, Component *comp, bool dryRun, bool writeToStdout) {
    if (comp->recreate || writeToStdout) {
        std::string compname = comp->CmakeName();
        bool isHeaderOnly = true;
        std::set<std::string> publicDeps, privateDeps, publicIncl, privateIncl;
        std::list<std::string> files;
        for (auto &fp : comp->files) {
            files.push_back(fp->path.generic_string().c_str() + compname.size() + 3);
            filesystem::path p = fp->path;
            if (fp->hasInclude) {
                (fp->hasExternalInclude ? publicIncl : privateIncl).insert(fp->includePaths.begin(),
                                                                           fp->includePaths.end());
            }
            if (IsCompileableFile(p.extension().string())) {
                isHeaderOnly = false;
            }
        }
        for (auto &d : comp->privDeps) {
            privateDeps.insert(d->CmakeName());
        }
        for (auto &d : comp->pubDeps) {
            publicDeps.insert(d->CmakeName());
        }
        files.sort();
        if (!files.empty() && comp->type == "") {
            comp->type = "add_library";
        }

        for (auto &s : publicDeps) {
            privateDeps.erase(s);
        }
        for (auto &s : publicIncl) {
            privateIncl.erase(s);
        }

        {
            streams::ofstream out;
            if (writeToStdout) {
                std::cout << "######## Start of " << comp->root.generic_string() << "/CMakeLists.txt\n";
            } else {
                out.open(comp->root / "CMakeLists.txt.generated");
            }
            std::ostream& o = writeToStdout ? std::cout : out;

            RegenerateCmakeHeader(o, config);

            if (comp->root == ".") {
                // Do this for the user, so that you don't get a warning on either 
                // not doing this, or doing this in the addon file.
                o << "cmake_minimum_required(VERSION " << config.cmakeVersion << ")\n";
            }

            if (!files.empty()) {
                o << "project(" << compname << ")" << "\n\n";
            }

            o << "\n";

            if (!files.empty()) {
                RegenerateCmakeAddTarget(o, config, *comp, files, isHeaderOnly);
                RegenerateCmakeTargetLinkLibraries(o, publicDeps, privateDeps, isHeaderOnly);
                RegenerateCmakeTargetIncludeDirectories(o, publicIncl, privateIncl, isHeaderOnly);
                RegenerateCmakeAddDependencies(o, *comp);
            }

            if (comp->hasAddonCmake) {
                o << "include(CMakeAddon.txt)" << "\n";
            }

            if (config.reuseCustomSections) {
                o << comp->additionalCmakeDeclarations;
            }

            RegenerateCmakeAddSubdirectory(o, *comp);
        }
        if (writeToStdout) {
            std::cout << "######## End of " << comp->root.generic_string() << "/CMakeLists.txt\n";
            // No actual file written.
        } else if (dryRun) {
            if (FilesAreDifferent(comp->root / "CMakeLists.txt.generated", comp->root / "CMakeLists.txt")) {
                std::cout << "Difference detected at " << comp->root << "\n";
            }
            filesystem::remove(comp->root / "CMakeLists.txt.generated");
        } else {
            if (FilesAreDifferent(comp->root / "CMakeLists.txt.generated", comp->root / "CMakeLists.txt")) {
                filesystem::rename(comp->root / "CMakeLists.txt.generated", comp->root / "CMakeLists.txt");
            } else {
                filesystem::remove(comp->root / "CMakeLists.txt.generated");
            }
        }
    }
}

void RegenerateCmakeHeader(std::ostream& o, const Configuration& config) {
    std::string licenseString;
    MakeCmakeComment(licenseString, config.licenseString);

    o << "#\n";
    o << "# Copyright (c) " << config.companyName << ". All rights reserved.\n";
    o << licenseString;
    o << "#\n\n";

    o << "# " << config.regenTag << " - do not edit, your changes will be lost\n";
    o << "# If you must edit, remove these two lines to avoid regeneration\n\n";
}

void RegenerateCmakeAddDependencies(std::ostream& o, const Component& comp) {
    if (!comp.buildAfters.empty()) {
        o << "add_dependencies(${PROJECT_NAME}\n";
        for (auto &s : comp.buildAfters) {
            o << "  " << s << "\n";
        }
        o << ")\n\n";
    }
}

void RegenerateCmakeAddSubdirectory(std::ostream& o,
                                    const Component& comp)
{
    // Temporary uses a set to sort subdirectories
    std::set<std::string> subdirs;
    filesystem::directory_iterator it(comp.root), end;
    for (; it != end; ++it) {
        if (filesystem::is_regular_file(it->path() / "CMakeLists.txt")) {
            subdirs.insert(it->path().filename().generic_string());
        }
    }
    for (auto subdir : subdirs) {
        o << "add_subdirectory(" << subdir << ")\n";
    }
        
}

void RegenerateCmakeAddTarget(std::ostream& o,
                              const Configuration& config,
                              const Component& comp,
                              const std::list<std::string>& files,
                              bool isHeaderOnly) {
    o << comp.type << "(${PROJECT_NAME}";

    if (isHeaderOnly) {
        o << " INTERFACE\n";
    } else {
        if (config.addLibraryAliases.count(comp.type) == 1) {
            o << " STATIC\n";
        } else {
            o << "\n";
        }

        for (auto &f : files) {
            o << "  " << f << "\n";
        }
    }
    o << comp.additionalTargetParameters;
    o << ")\n\n";
}

void RegenerateCmakeTargetIncludeDirectories(std::ostream& o,
                                             const std::set<std::string>& publicIncl,
                                             const std::set<std::string>& privateIncl,
                                             bool isHeaderOnly) {
    if (!publicIncl.empty() || !privateIncl.empty()) {
        o << "target_include_directories(${PROJECT_NAME}\n";
        if (isHeaderOnly) {
            o << "  INTERFACE\n";
        }

        if (!publicIncl.empty() && !isHeaderOnly) {
            o << "  PUBLIC\n";
        }
        for (auto &s : publicIncl) {
            o << "    " << s << "\n";
        }

        if (!privateIncl.empty() && !isHeaderOnly) {
            o << "  PRIVATE\n";
        }
        for (auto &s : privateIncl) {
            o << "    " << s << "\n";
        }
        o << ")\n\n";
    }
}

void RegenerateCmakeTargetLinkLibraries(std::ostream& o,
                                        const std::set<std::string>& publicDeps,
                                        const std::set<std::string>& privateDeps,
                                        bool isHeaderOnly) {
    if (!publicDeps.empty() || !privateDeps.empty()) {
        o << "target_link_libraries(${PROJECT_NAME}\n";
        if (isHeaderOnly) {
            o << "  INTERFACE\n";
        }

        if (!publicDeps.empty() && !isHeaderOnly) {
            o << "  PUBLIC\n";
        }
        for (auto &s : publicDeps) {
            o << "    " << s << "\n";
        }

        if (!privateDeps.empty() && !isHeaderOnly) {
            o << "  PRIVATE\n";
        }
        for (auto &s : privateDeps) {
            o << "    " << s << "\n";
        }

        o << ")\n\n";
    }
}
