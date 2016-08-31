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

struct Component;

void OutputFlatDependencies(std::unordered_map<std::string, Component *> &components,
                            const path &outfile);
void OutputCircularDependencies(std::unordered_map<std::string, Component *> &components,
                                const path &outfile);
void PrintGraphOnTarget(const path &outfile, Component *c);
void PrintAllComponents(std::unordered_map<std::string, Component *> &components,
                        const char* description,
                        bool (*)(const Component&));
void PrintAllFiles(std::unordered_map<std::string, File>& files, const char* description, bool (*predicate)(const File&));
void FindAndPrintCycleFrom(Component *origin, Component *c, std::unordered_set<Component *> alreadyHad,
                           std::vector<Component *> order);
void PrintCyclesForTarget(Component *c);
void PrintLinksForTarget(Component *c);
void PrintInfoOnTarget(Component *c);
void FindSpecificLink(std::unordered_map<std::string, File>& files, Component *from, Component *to);

#endif


