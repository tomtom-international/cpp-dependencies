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

std::string Component::NiceName(char sub) const {
    if (root.string() == ".") {
        return std::string("ROOT");
    }

    std::string name = root.generic_string().c_str() + 2;
    std::replace(name.begin(), name.end(), '/', sub);
    return name;
}

std::string Component::QuotedName() const {
    return std::string("\"") + NiceName('.') + std::string("\"");
}

std::string Component::CmakeName() const {
    return (recreate || name.empty()) ? NiceName('.') : name;
}

Component::Component(const std::experimental::filesystem::path &path)
        : root(path), recreate(false), hasAddonCmake(false), type("library") {
}

std::vector<std::string> SortedNiceNames(const std::unordered_set<Component *> &comps) {
    std::vector<std::string> ret;
    ret.resize(comps.size());
    std::transform(comps.begin(), comps.end(), ret.begin(), [](const Component *comp) -> std::string {
        return comp->NiceName('.');
    });
    std::sort(ret.begin(), ret.end());

    return ret;
}

Component &AddComponentDefinition(std::unordered_map<std::string, Component *> &components,
                                  const std::experimental::filesystem::path &path) {
    Component *&comp = components[path.generic_string()];
    if (!comp) {
        comp = new Component(path);
    }
    return *comp;
}

size_t NodesWithCycles(std::unordered_map<std::string, Component *> &components) {
    size_t count = 0;
    for (auto &c : components) {
        if (!c.second->circulars.empty()) {
            count++;
        }
    }
    return count;
}

void ExtractPublicDependencies(std::unordered_map<std::string, Component *> &components) {
    for (auto &c : components) {
        Component *comp = c.second;
        for (auto &fp : comp->files) {
            if (fp->hasExternalInclude) {
                for (auto &dep : fp->dependencies) {
                    comp->privDeps.erase(dep->component);
                    comp->pubDeps.insert(dep->component);
                }
            }
        }
        comp->pubDeps.erase(comp);
        comp->privDeps.erase(comp);
    }
}

