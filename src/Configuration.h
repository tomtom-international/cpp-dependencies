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

#ifndef __DEP_CHECKER__CONFIGURATION_H
#define __DEP_CHECKER__CONFIGURATION_H

#include <string>
#include <unordered_set>

struct Configuration {
  Configuration();
  void read(std::istream& inputFile);
  std::string companyName;
  std::string licenseString;
  std::string regenTag;
  std::string cmakeVersion;
  std::string versionUsed;
  std::string cycleColor;
  std::string publicDepColor;
  std::string privateDepColor;
  std::unordered_set<std::string> addLibraryAliases;
  std::unordered_set<std::string> addExecutableAliases;
  std::unordered_set<std::string> addIgnores;
  std::unordered_set<std::string> blacklist;
  size_t componentLinkLimit;
  size_t componentLocLowerLimit;
  size_t componentLocUpperLimit;
  size_t fileLocUpperLimit;
  bool reuseCustomSections;
};

#endif


