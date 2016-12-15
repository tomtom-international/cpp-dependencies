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

#ifndef __DEP_CHECKER__OUTPUT_H
#define __DEP_CHECKER__OUTPUT_H

#include "FilesystemInclude.h"
#include <unordered_set>
#include <vector>
#include <functional>

struct Component;

void OutputFlatDependencies(const Configuration& config, std::unordered_map<std::string, Component *> &components,
                            const filesystem::path &outfile);
void OutputCircularDependencies(const Configuration& config, std::unordered_map<std::string, Component *> &components,
                                const filesystem::path &outfile);
void PrintGraphOnTarget(const Configuration& config, const filesystem::path &outfile, Component *c);
void PrintAllComponents(std::unordered_map<std::string, Component *> &components,
                        const char* description,
                        std::function<bool (const Component&)>);
void PrintAllFiles(std::unordered_map<std::string, File>& files, const char* description, std::function<bool(const File&)>);
void FindAndPrintCycleFrom(Component *origin, Component *c, std::unordered_set<Component *> alreadyHad,
                           std::vector<Component *> order);
void PrintCyclesForTarget(Component *c);
void PrintLinksForTarget(Component *c);
void PrintInfoOnTarget(Component *c);
void FindSpecificLink(const Configuration& config, std::unordered_map<std::string, File>& files, Component *from, Component *to);
void UpdateIncludes(std::unordered_map<std::string, File>& files, std::unordered_map<std::string, std::string> &includeLookup, Component* component, const std::string& desiredPath, bool isAbsolute);

#endif


