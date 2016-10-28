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

#include "Component.h"
#include "Configuration.h"
#include "FstreamInclude.h"
#include "Input.h"
#include <algorithm>

static std::string GetPathFromIncludeLine(const std::string &str) {
    size_t posAfterInclStmt = str.find("#include");
    if (posAfterInclStmt == str.npos) {
        return "";
    }

    // ignore includes that are commented out, basic heuristic
    const std::string &lineStart = str.substr(0, 2);
    if (lineStart == "//" || lineStart == "/*") {
        return "";
    }

    size_t open = str.find("<", posAfterInclStmt),
            close = str.find(">", posAfterInclStmt);
    if (open == str.npos) {
        open = str.find("\"", posAfterInclStmt);
        close = str.find("\"", open + 1);
    }
    if (close == str.npos) {
        return "";
    }
    return str.substr(open + 1, close - open - 1);
}

static bool IsItemBlacklisted(const adapted_namespace::path &path, const std::unordered_set<std::string> &ignorefiles) {
    // Add your own blacklisted items here.
    std::string file = path.filename().generic_string();
    return ignorefiles.find(file) != ignorefiles.end();
}

static void ReadCmakelist(std::unordered_map<std::string, Component *> &components,
                          const adapted_namespace::path &path) {
    adapted_namespace::ifstream in(path);
    std::string line;
    Component &comp = AddComponentDefinition(components, path.parent_path());
    do {
        getline(in, line);
        if (strstr(line.c_str(), Configuration::Get().regenTag.c_str())) {
            comp.recreate = true;
        }
        if (strstr(line.c_str(), "project(") == line.c_str()) {
            size_t end = line.find(')');
            if (end != line.npos) {
                comp.name = line.substr(8, end - 8);
            }
        } else if (strstr(line.c_str(), "_library")) {
            comp.type = "library";
        } else if (strstr(line.c_str(), "_executable")) {
            comp.type = "executable";
        }
    } while (in.good());
}

static void ReadCode(std::unordered_map<std::string, File>& files, const adapted_namespace::path &path) {
    File &f = files[path.generic_string()];
    f.path = path;
    std::vector<std::string> &includes = f.rawIncludes;
    adapted_namespace::ifstream in(path);
    std::string line;
    do {
        f.loc++;
        getline(in, line);
        if (strstr(line.c_str(), "#include")) {
            std::string val = GetPathFromIncludeLine(line);
            if (val.size()) {
                includes.push_back(val);
            }
        }
    } while (in.good());
}

bool IsCompileableFile(const std::string& ext) {
  std::string lower;
  std::transform(ext.begin(), ext.end(), std::back_inserter(lower), ::tolower);
  return lower == ".c" ||
         lower == ".cc" ||
         lower == ".cpp";
}

static bool IsCode(const std::string &ext) {
    std::string lower;
    std::transform(ext.begin(), ext.end(), std::back_inserter(lower), ::tolower);
    return lower == ".h" ||
           lower == ".hpp" ||
           IsCompileableFile(ext);
}

void LoadFileList(std::unordered_map<std::string, Component *> &components,
                  std::unordered_map<std::string, File>& files,
                  const std::unordered_set<std::string> &ignorefiles,
                  const adapted_namespace::path& sourceDir,
                  bool inferredComponents) {
    adapted_namespace::path outputpath = current_path();
    current_path(sourceDir.c_str());
    AddComponentDefinition(components, ".");
    for (adapted_namespace::recursive_directory_iterator it("."), end;
         it != end; ++it) {
        const auto &parent = it->path().parent_path();
        if (inferredComponents) AddComponentDefinition(components, parent);

        // skip hidden files and dirs
        if (is_directory(parent) &&
            parent.filename().generic_string().size() > 2 &&
            parent.filename().generic_string()[0] == '.') {
            it.pop();
            if (it == end) {
                break;
            }
        }

        if (IsItemBlacklisted(it->path(), ignorefiles)) {
            continue;
        }
        if (it->path().filename() == "CMakeLists.txt") {
            ReadCmakelist(components, it->path());
        } else if (is_regular_file(it->status())) {
            if (it->path().generic_string().find("CMakeAddon.txt") != std::string::npos) {
                AddComponentDefinition(components, parent).hasAddonCmake = true;
            } else if (IsCode(it->path().extension().generic_string().c_str())) {
                ReadCode(files, it->path());
            }
        }
    }
    current_path(outputpath);
}

