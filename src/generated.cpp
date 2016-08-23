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

void CreateIncludeLookupTable(std::unordered_map<std::string, File>& files,
                              std::unordered_map<std::string, std::string> &includeLookup,
                              std::map<std::string, std::set<std::string>> &collisions) {
    for (auto &p : files) {
        std::string lowercasePath;
        std::transform(p.first.begin(), p.first.end(), std::back_inserter(lowercasePath), ::tolower);
        const char *pa = lowercasePath.c_str();
        while ((pa = strstr(pa + 1, "/"))) {
            std::string &ref = includeLookup[pa + 1];
            if (ref.size() == 0) {
                ref = p.first;
            } else {
                collisions[pa + 1].insert(p.first);
                if (ref != "INVALID") {
                    collisions[pa + 1].insert(ref);
                }
                ref = "INVALID";
            }
        }
    }
}


