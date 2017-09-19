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

#ifndef __DEP_CHECKER__CMAKEREGEN_H
#define __DEP_CHECKER__CMAKEREGEN_H

#include <list>
#include <ostream>
#include <set>
#include <string>

struct Component;
struct Configuration;

void RegenerateCmakeFilesForComponent(const Configuration& config,
                                      Component *comp,
                                      bool dryRun, 
                                      bool writeToStdout);

void MakeCmakeComment(std::string& cmakeComment,
                      const std::string& contents);

void RegenerateCmakeAddDependencies(std::ostream& o,
                                    const Component& comp);
void RegenerateCmakeAddSubdirectory(std::ostream& o,
                                    const Component& comp);
void RegenerateCmakeAddTarget(std::ostream& o,
                              const Configuration& config,
                              const Component& comp,
                              const std::list<std::string>& files,
                              bool isHeaderOnly);
void RegenerateCmakeHeader(std::ostream& o, const Configuration& config);
void RegenerateCmakeTargetIncludeDirectories(std::ostream& o,
                                             const std::set<std::string>& publicIncl,
                                             const std::set<std::string>& privateIncl,
                                             bool isHeaderOnly);
void RegenerateCmakeTargetLinkLibraries(std::ostream& o,
                                        const std::set<std::string>& publicDeps,
                                        const std::set<std::string>& privateDeps,
                                        bool isHeaderOnly);

#endif
