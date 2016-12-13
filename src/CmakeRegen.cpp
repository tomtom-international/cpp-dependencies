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

void RegenerateCmakeFilesForComponent(Component *comp, bool dryRun) {
    if (comp->recreate) {
        const Configuration& config = Configuration::Get();
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
            if (p.extension() == ".cpp") {
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
            streams::ofstream o(comp->root / "CMakeLists.txt.generated");

            o << "#\n";
            o << "# Copyright (C) " << Configuration::Get().companyName << ". All rights reserved.\n";
            o << "#\n";
            o << "# " << Configuration::Get().regenTag << " - do not edit, your changes will be lost" << "\n";
            o << "# If you must edit, remove these two lines to avoid regeneration" << "\n\n";

            if (!files.empty()) {
                o << "project(" << compname << ")" << "\n\n";
            }

            o << "\n";

            if (!files.empty()) {
                o << comp->type << "(${PROJECT_NAME}";

                if (isHeaderOnly) {
                    o << " INTERFACE" << "\n";
                } else {
                    if (config.addLibraryAliases.count(comp->type) == 1) {
                        o << " STATIC" << "\n";
                    } else {
                        o << "\n";
                    }

                    for (auto &f : files) {
                        o << "  " << f << "\n";
                    }
                }
                o << ")" << "\n\n";

                if (!publicDeps.empty() || !privateDeps.empty()) {
                    o << "target_link_libraries(${PROJECT_NAME}" << "\n";
                    if (isHeaderOnly) {
                        o << "  INTERFACE" << "\n";
                    }

                    if (!publicDeps.empty() && !isHeaderOnly) {
                        o << "  PUBLIC" << "\n";
                    }
                    for (auto &s : publicDeps) {
                        o << "    " << s << "\n";
                    }

                    if (!privateDeps.empty() && !isHeaderOnly) {
                        o << "  PRIVATE" << "\n";
                    }
                    for (auto &s : privateDeps) {
                        o << "    " << s << "\n";
                    }

                    o << ")" << "\n\n";
                }

                if (!publicIncl.empty() || !privateIncl.empty()) {
                    o << "target_include_directories(${PROJECT_NAME}" << "\n";
                    if (isHeaderOnly) {
                        o << "  INTERFACE" << "\n";
                    }

                    if (!publicIncl.empty() && !isHeaderOnly) {
                        o << "  PUBLIC" << "\n";
                    }
                    for (auto &s : publicIncl) {
                        o << "    " << s << "\n";
                    }

                    if (!privateIncl.empty() && !isHeaderOnly) {
                        o << "  PRIVATE" << "\n";
                    }
                    for (auto &s : privateIncl) {
                        o << "    " << s << "\n";
                    }
                    o << ")" << "\n\n";
                }

                if (!comp->buildAfters.empty()) {
                    o << "add_dependencies(${PROJECT_NAME}" << "\n";
                    for (auto &s : comp->buildAfters) {
                        o << "  " << s << "\n";
                    }
                    o << ")" << "\n\n";
                }
            }
            // Add existing subdirectories that contain a CMakeLists file
            filesystem::directory_iterator it(comp->root), end;
            std::set<std::string> items;
            for (; it != end; ++it) {
                if (filesystem::is_regular_file(it->path() / "CMakeLists.txt")) {
                    items.insert(it->path().filename().generic_string());
                }
            }

            if (comp->hasAddonCmake) {
                o << "include(CMakeAddon.txt)" << "\n";
            }

            for (auto &i : items) {
                o << "add_subdirectory(" << i << ")" << "\n";
            }
        }
        if (dryRun) {
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

