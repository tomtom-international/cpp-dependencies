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
#include "Input.h"
#include "CmakeRegen.h"
#include "Output.h"
#include "Analysis.h"
#include "Constants.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

/**
 * These variables contain the global lists of components and files.
 * They are globally accessible.
 */
std::unordered_map<std::string, Component *> components;
std::unordered_map<std::string, File> files;

// Variable to contain the root of the source tree.
static boost::filesystem::path root;

static bool CheckVersionFile() {
    const std::string currentVersion = CURRENT_VERSION;
    std::string version;
    boost::filesystem::ifstream in(root / VERSION_FILE);
    getline(in, version);
    return (version == currentVersion);
}

std::string targetFrom(const std::string &arg) {
    std::string target = arg;
    if (!target.empty() && target.back() == '/') {
        target.pop_back();
    }
    std::replace(target.begin(), target.end(), '.', '/');
    return target;
}

std::map<std::string, void (*)(int, char **)> functions = {
        {"--graph",        [](int argc, char **argv) {
            if (argc != 3) {
                printf("Usage: %s --graph <outputfile>\n", argv[0]);
            } else {
                FindCircularDependencies();
                OutputFlatDependencies(argv[2]);
            }
        }},
        {"--graph-cycles", [](int argc, char **argv) {
            if (argc != 3) {
                printf("Usage: %s --graph-cycles <outputfile>\n", argv[0]);
            } else {
                FindCircularDependencies();
                OutputCircularDependencies(argv[2]);
            }
        }},
        {"--graph-target", [](int argc, char **argv) {
            if (argc != 4) {
                printf("Usage: %s --graph-target <targetname> <outputfile>\n", argv[0]);
            } else {
                FindCircularDependencies();
                PrintGraphOnTarget(argv[3], components[std::string("./") + targetFrom(argv[2])]);
            }
        }},
        {"--cycles",       [](int argc, char **argv) {
            if (argc != 3) {
                printf("Usage: %s --cycles <targetname>\n", argv[0]);
            } else {
                FindCircularDependencies();
                PrintCyclesForTarget(components[std::string("./") + targetFrom(argv[2])]);
            }
        }},
        {"--stats",        [](int, char **) {
            std::size_t totalPublicLinks(0), totalPrivateLinks(0);
            for (const auto &c : components) {
                totalPublicLinks += c.second->pubDeps.size();
                totalPrivateLinks += c.second->privDeps.size();
            }
            fprintf(stderr, "%zu components with %zu public dependencies, %zu private dependencies\n",
                    components.size(), totalPublicLinks, totalPrivateLinks);
            FindCircularDependencies();
            fprintf(stderr, "Detected %zu nodes in cycles\n", NodesWithCycles());
        }},
        {"--inout",        [](int argc, char **argv) {
            if (argc != 3) {
                printf("Usage: %s --inout <targetname>\n", argv[0]);
            } else {
                PrintLinksForTarget(components[std::string("./") + targetFrom(argv[2])]);
            }
        }},
        {"--shortest",     [](int argc, char **argv) {
            if (argc != 4) {
                printf("Usage: %s --shortest <targetname-from> <targetname-to>\n", argv[0]);
            } else {
                FindCircularDependencies();
                Component* from = components[std::string("./") + targetFrom(argv[2])],
                         * to = components[std::string("./") + targetFrom(argv[3])];
                if (!from) {
                    printf("No such component %s\n", argv[2]);
                } else if (!to) {
                    printf("No such component %s\n", argv[3]);
                } else {
                    FindSpecificLink(from, to);
                }
            }
        }},
        {"--info",         [](int argc, char **argv) {
            if (argc != 3) {
                printf("Usage: %s --info <targetname>\n", argv[0]);
            } else {
                PrintInfoOnTarget(components[std::string("./") + targetFrom(argv[2])]);
            }
        }},
        {"--usedby",       [](int argc, char **argv) {
            if (argc != 3) {
                printf("Usage: %s --usedby <filename>\n", argv[0]);
            } else {
                auto f = &files[std::string("./") + argv[2]];
                for (auto &p : files) {
                    if (p.second.dependencies.find(f) != p.second.dependencies.end()) {
                        std::cout << p.second.path.string() << "\n";
                    }
                }
            }
        }},
        {"--regen",        [](int argc, char **argv) {
            boost::filesystem::current_path(root);
            if (argc >= 3) {
                for (int n = 2; n < argc; n++) {
                    if (components.find(targetFrom(argv[n])) != components.end()) {
                        RegenerateCmakeFilesForComponent(components[targetFrom(argv[2])], false);
                    } else {
                        std::cout << "Target '" << targetFrom(argv[2]) << "' not found\n";
                    }
                }
            } else {
                for (auto &c : components) {
                    RegenerateCmakeFilesForComponent(c.second, false);
                }
            }
        }},
        {"--dryregen",     [](int argc, char **argv) {
            boost::filesystem::current_path(root);
            if (argc >= 3) {
                for (int n = 2; n < argc; n++) {
                    if (components.find(targetFrom(argv[n])) != components.end()) {
                        RegenerateCmakeFilesForComponent(components[targetFrom(argv[2])], true);
                    } else {
                        std::cout << "Target '" << targetFrom(argv[2]) << "' not found\n";
                    }
                }
            } else {
                for (auto &c : components) {
                    RegenerateCmakeFilesForComponent(c.second, true);
                }
            }
        }},
        {"--outliers",       [](int, char**) {
            PrintAllComponents("Libraries with no links in:", [](const Component& c){
                return c.type == "library" && 
                    !c.files.empty() && 
                    c.pubLinks.empty() && c.privLinks.empty();
            });
            PrintAllComponents("Libraries with more than 30 outward links:", [](const Component& c){ return c.pubDeps.size() + c.privDeps.size() > 30; });
            PrintAllComponents("Libraries with less than 200 lines of code:", [](const Component& c) { return !c.files.empty() && c.loc() < 200; });
            PrintAllComponents("Libraries with more than 20000 lines of code:", [](const Component& c) { return c.loc() > 20000; });
            FindCircularDependencies();
            PrintAllComponents("Libraries that are part of a cycle:", [](const Component& c) { return !c.circulars.empty(); });
            PrintAllFiles("Files that are never used:", [](const File& f) { return !IsCompileableFile(f.path.extension().string()) && !f.hasInclude; });
            PrintAllFiles("Files with more than 2000 lines of code:", [](const File& f) { return f.loc > 2000; });
        }},
        {"--help",         [](int, char **argv) {
            printf("C++ Dependencies -- a tool to analyze large C++ code bases for #include dependency information\n");
            printf("Copyright (C) 2016, TomTom International BV\n");
            printf("\n");
            printf("  Usage:\n");
            printf("    %s [--dir <source-directory>] <command>\n", argv[0]);
            printf("    Source directory is assumed to be the current one if unspecified\n");
            printf("\n");
            printf("  Commands:\n");
            printf("  --help                        : Produce this help text\n");
            printf("\n");
            printf("  Extracting graphs:\n");
            printf("  --graph <output>              : Graph of all components with dependencies\n");
            printf("  --graph-cycles <output>       : Graph of components with cyclic dependencies on other components\n");
            printf("  --graph-for <output> <target> : Graph for all dependencies of a specific target\n");
            printf("\n");
            printf("  Getting information:\n");
            printf("  --stats                       : Info about code base size, complexity and cyclic dependency count\n");
            printf("  --cycles <targetname>         : Find all possible paths from this target back to itself\n");
            printf("  --shortest                    : Determine shortest path between components and its reason\n");
            printf("  --outliers                    : Finds all components and files that match a criterium for being out of the ordinary\n");
            printf("                                       - libraries that are not used\n");
            printf("                                       - components that use a lot of other components\n");
            printf("                                       - components with dependencies towards executables\n");
            printf("                                       - components with less than 200 LoC\n");
            printf("                                       - components with more than 20 kLoC\n");
            printf("                                       - components that are part of a cycle\n");
            printf("                                       - files that are more than 2000 LoC\n");
            printf("                                       - files that are not compiled and never included\n");
            printf("\n");
            printf("  Target information:\n");
            printf("  --info                        : Show all information on a given specific target\n");
            printf("  --usedby                      : Find all references to a specific header file\n");
            printf("  --inout                       : Find all incoming and outgoing links for a target\n");
            printf("  --ambiguous                   : Find all include statements that could refer to more than one header\n");
        }},
        // --ignore : Handled during input parsing as it is a pre-filter for CheckCycles instead.
};

int main(int argc, char **argv) {
    root = boost::filesystem::current_path();
    std::unordered_set<std::string> ignorefiles = {
            "unistd.h",
            "console.h",
            "stdint.h",
            "windows.h",
            "library.h",
            "endian.h",
            "rle.h",
    };
    if (argc > 1 && argv[1] == std::string("--dir")) {
        // Extract the root directory, reshuffle the other parameters.
        root = argv[2];
        argv[2] = argv[0];
        argv += 2;
        argc -= 2;
    }
    std::string command;
    if (argc > 1) {
        std::transform(argv[1], argv[1] + strlen(argv[1]), std::back_inserter(command), ::tolower);
    } else {
        command = "--help";
    }
    if (command == "--ignore") {
        for (int n = 2; n < argc; n++) {
            ignorefiles.insert(argv[n]);
        }
        argc = 1;
        command = "--stats";
    }
    if (command == "--regen" && !CheckVersionFile()) {
        fprintf(stderr,
                "Version of dependency checker not the same as the one used to generate the existing cmakelists. Refusing to regen\n"
                        "Please manually update " VERSION_FILE  " to version \"" CURRENT_VERSION "\" if you really want to do this.\n");
        command = "--dryregen";
    }

    LoadFileList(ignorefiles, root);
    {
        std::map<std::string, std::set<std::string>> collisions;
        std::unordered_map<std::string, std::string> includeLookup;
        CreateIncludeLookupTable(includeLookup, collisions);
        MapFilesToComponents();
        std::map<std::string, std::vector<std::string>> ambiguous;
        MapIncludesToDependencies(includeLookup, ambiguous);
        if (command == "--ambiguous") {
            printf("Found %zu ambiguous includes\n\n", ambiguous.size());
            for (auto &i : ambiguous) {
                printf("Include for %s\nFound in:\n", i.first.c_str());
                for (auto &s : i.second) {
                    printf("  included from %s\n", s.c_str());
                }
                printf("Options for file:\n");
                for (auto &c : collisions[i.first]) {
                    printf("  %s\n", c.c_str());
                }
                printf("\n");
            }
            exit(0);
        }

        for (auto &i : ambiguous) {
            for (auto &c : collisions[i.first]) {
                files[c].hasInclude = true; // There is at least one include that might end up here.
            }
        }
    }
    PropagateExternalIncludes();
    ExtractPublicDependencies();

    void (*function)(int, char **);
    function = functions[command];
    if (!function) {
        function = functions["--help"];
    }
    function(argc, argv);
    return 0;
}
