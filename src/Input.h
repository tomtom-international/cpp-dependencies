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

#ifndef __DEP_CHECKER__INPUT_H
#define __DEP_CHECKER__INPUT_H

#include "FilesystemInclude.h"
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>

struct File;
struct Component;

bool IsCompileableFile(const std::string& ext);

void ForgetEmptyComponents(std::unordered_map<std::string, Component *> &components);
void LoadFileList(const Configuration& config,
                  std::unordered_map<std::string, Component *> &components,
                  std::unordered_map<std::string, File>& files,
                  const filesystem::path& sourceDir,
                  bool inferredComponents,
                  bool withLoc);

#endif


