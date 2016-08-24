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

#include <string>
#include <experimental/filesystem>
#include <regex>
#include <unordered_set>

bool IsCompileableFile(const std::string& ext);
void LoadFileList(const std::unordered_set<std::string> &ignorefiles, const std::experimental::filesystem::path& sourceDir);

#endif


