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

#include "Analysis.h"

static bool DoesDependencyTransitiveClosureContainSelf(Component *source, Component *target) {
    std::vector<Component *> outs;
    outs.push_back(target);
    for (size_t index = 0; index < outs.size(); ++index) {
        for (auto &c : outs[index]->pubDeps) {
            if (c == source) {
                return true;
            }
            if (std::find(outs.begin(), outs.end(), c) == outs.end()) {
                outs.push_back(c);
            }
        }
        for (auto &c : outs[index]->privDeps) {
            if (c == source) {
                return true;
            }
            if (std::find(outs.begin(), outs.end(), c) == outs.end()) {
                outs.push_back(c);
            }
        }
    }
    return false;
}

void FindCircularDependencies() {
    for (auto &c : components) {
        for (auto &c2 : c.second->pubDeps) {
            if (DoesDependencyTransitiveClosureContainSelf(c.second, c2)) {
                c.second->circulars.insert(c2);
            }
        }
        for (auto &c2 : c.second->privDeps) {
            if (DoesDependencyTransitiveClosureContainSelf(c.second, c2)) {
                c.second->circulars.insert(c2);
            }
        }
    }
}

void MapFilesToComponents() {
    for (auto &fp : files) {
        std::string nameCopy = fp.first;
        size_t slashPos = nameCopy.find_last_of('/');
        while (slashPos != nameCopy.npos) {
            nameCopy.resize(slashPos);
            auto it = components.find(nameCopy);
            if (it != components.end()) {
                fp.second.component = it->second;
                it->second->files.insert(&fp.second);
                break;
            }
            slashPos = nameCopy.find_last_of('/');
        }
    }
}

void MapIncludesToDependencies(std::unordered_map<std::string, std::string> &includeLookup,
                               std::map<std::string, std::vector<std::string>> &ambiguous) {
    for (auto &fp : files) {
        for (auto &i : fp.second.rawIncludes) {
            std::string &fullPath = includeLookup[i];
            if (fullPath == "INVALID") {
                ambiguous[i.c_str()].push_back(fp.first);
            } else if (fullPath.find("GENERATED:") == 0) {
                if (fp.second.component) {
                    fp.second.component->buildAfters.insert(fullPath.substr(10));
                    Component *c = components["./" + fullPath.substr(10)];
                    if (c) {
                        fp.second.component->privDeps.insert(c);
                    }
                }
            } else {
                auto it = files.find(fullPath);
                if (it != files.end()) {
                    File *dep = &it->second;
                    fp.second.dependencies.insert(dep);

                    if (fp.second.component && dep->component) {
                        std::string inclpath = fullPath.substr(0, fullPath.size() - i.size() - 1);
                        if (fp.second.path.parent_path().string() == dep->path.parent_path().string()) {
                            // Omit include paths for files in your own folder. This under-declares for pointy-bracket includes in your own
                            // folder, but at least it prevents overdeclaring.
                            inclpath = "";
                        } else if (inclpath.size() == dep->component->root.string().size()) {
                            inclpath = ".";
                        } else if (inclpath.size() > dep->component->root.string().size() + 1) {
                            inclpath = inclpath.substr(dep->component->root.string().size() + 1);
                        } else {
                            inclpath = "";
                        }
                        if (!inclpath.empty()) {
                            dep->includePaths.insert(inclpath);
                        }

                        if (fp.second.component != dep->component) {
                            fp.second.component->privDeps.insert(dep->component);
                            dep->component->privLinks.insert(fp.second.component);
                            dep->hasExternalInclude = true;
                        }
                    }
                    dep->hasInclude = true;
                }
            }
        }
    }
}

void PropagateExternalIncludes() {
    bool foundChange;
    do {
        foundChange = false;
        for (auto &fp : files) {
            if (fp.second.hasExternalInclude && fp.second.component) {
                for (auto &dep : fp.second.dependencies) {
                    if (!dep->hasExternalInclude && dep->component == fp.second.component) {
                        dep->hasExternalInclude = true;
                        foundChange = true;
                    }
                }
            }
        }
    } while (foundChange);
}


