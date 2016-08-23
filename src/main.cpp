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
#include "Configuration.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

static bool CheckVersionFile() {
    const std::string currentVersion = CURRENT_VERSION;
    return currentVersion == Configuration::Get().versionUsed;
}

std::string targetFrom(const std::string &arg) {
    std::string target = arg;
    if (!target.empty() && target.back() == '/') {
        target.pop_back();
    }
    std::replace(target.begin(), target.end(), '.', '/');
    return "./" + target;
}

class Operations {
public:
    Operations(int argc, char** argv)
    : projectLoaded(false)
    , programName(argv[0])
    , args(argv+1, argv+argc)
    {
        RegisterCommands();
        projectRoot = outputRoot = boost::filesystem::current_path();
        ignorefiles = std::unordered_set<std::string>{
                "unistd.h",
                "console.h",
                "stdint.h",
                "windows.h",
                "library.h",
                "endian.h",
                "rle.h",
        };

    }
    void RunCommands() {
        if (args.empty()) {
            args.push_back("--help");
        }
        std::vector<std::string>::iterator it = args.begin(), localEnd = args.begin(), end = args.end();
        while (it != end) {
            localEnd = it;
            localEnd++;
            while (localEnd != end && ((*localEnd)[0] != '-' || (*localEnd)[1] != '-')) localEnd++;
            RunCommand(it, localEnd);
            it = localEnd;
        }
    }
private:
    typedef void (Operations::*Command)(std::vector<std::string>);
    void RegisterCommands() {
        commands["--outliers"] = &Operations::Outliers;
        commands["--ambiguous"] = &Operations::Ambiguous;
        commands["--help"] = &Operations::Help;
        commands["--info"] = &Operations::Info;
        commands["--usedby"] = &Operations::UsedBy;
        commands["--ignore"] = &Operations::Ignore;
        commands["--drop"] = &Operations::Drop;
        commands["--graph"] = &Operations::Graph;
        commands["--graph-cycles"] = &Operations::GraphCycles;
        commands["--graph-target"] = &Operations::GraphTarget;
        commands["--cycles"] = &Operations::Cycles;
        commands["--stats"] = &Operations::Stats;
        commands["--inout"] = &Operations::InOut;
        commands["--shortest"] = &Operations::Shortest;
        commands["--regeu"] = &Operations::Regen;
        commands["--dryregen"] = &Operations::DryRegen;
        commands["--dir"] = &Operations::Dir;
    }
    void RunCommand(std::vector<std::string>::iterator &arg, std::vector<std::string>::iterator &end) {
        std::string lowerCommand;
        std::transform(arg->begin(), arg->end(), std::back_inserter(lowerCommand), ::tolower);
        ++arg;
        
        Command c = commands[lowerCommand];
        if (!c) c = commands["--help"];
        (this->*c)(std::vector<std::string>(arg, end));
    }
    void LoadProject() {
        if (projectLoaded) return;
        LoadFileList(components, files, ignorefiles, projectRoot);
        CreateIncludeLookupTable(files, includeLookup, collisions);
        MapFilesToComponents(components, files);
        MapIncludesToDependencies(includeLookup, ambiguous, components, files);
        for (auto &i : ambiguous) {
            for (auto &c : collisions[i.first]) {
                files[c].hasInclude = true; // There is at least one include that might end up here.
            }
        }
        PropagateExternalIncludes(files);
        ExtractPublicDependencies(components);
        FindCircularDependencies(components);
        for (auto& c : deleteComponents) {
            KillComponent(components, c);
        }
        projectLoaded = true;
    }
    void UnloadProject() {
        components.clear();
        files.clear();
        collisions.clear();
        includeLookup.clear();
        ambiguous.clear();
        projectLoaded = false;
    }
    void Dir(std::vector<std::string> args) {
        if (args.empty()) {
            printf("No directory specified after --dir\n");
        } else {
            projectRoot = args[0];
        }
        UnloadProject();
    }
    void Ignore(std::vector<std::string> args) {
        if (args.empty())
            printf("No files specified to ignore?\n");
        for (auto& s : args) ignorefiles.insert(s);
        UnloadProject();
    }
    void Drop(std::vector<std::string> args) {
        if (args.empty())
            printf("No files specified to ignore?\n");
        for (auto& s : args) deleteComponents.insert(s);
        UnloadProject();
    }
    void Graph(std::vector<std::string> args) {
        LoadProject();
        if (args.empty()) {
            printf("No output file specified for graph\n");
        } else {
            OutputFlatDependencies(components, args[0]);
        }
    }
    void GraphCycles(std::vector<std::string> args) {
        LoadProject();
        if (args.empty()) {
            printf("No output file specified for cycle graph\n");
        } else {
            OutputCircularDependencies(components, args[0]);
        }
    }
    void GraphTarget(std::vector<std::string> args) {
        LoadProject();
        if (args.size() != 2) {
            printf("--graph-target requires a single component and a single output file name.\n");
        } else {
            PrintGraphOnTarget(args[1], components[targetFrom(args[0])]);
        }
    }
    void Cycles(std::vector<std::string> args) {
        LoadProject();
        if (args.empty())
            printf("No targets specified for finding in- and out-going links.\n");
        PrintCyclesForTarget(components[targetFrom(args[0])]);
    }
    void Stats(std::vector<std::string>) {
        LoadProject();
        std::size_t totalPublicLinks(0), totalPrivateLinks(0);
        for (const auto &c : components) {
            totalPublicLinks += c.second->pubDeps.size();
            totalPrivateLinks += c.second->privDeps.size();
        }
        fprintf(stderr, "%zu components with %zu public dependencies, %zu private dependencies\n",
                components.size(), totalPublicLinks, totalPrivateLinks);
        fprintf(stderr, "Detected %zu nodes in cycles\n", NodesWithCycles(components));
    }
    void InOut(std::vector<std::string> args) {
        LoadProject();
        if (args.empty())
            printf("No targets specified for finding in- and out-going links.\n");
        for (auto& s : args) {
            PrintLinksForTarget(components[targetFrom(s)]);
        }
    }
    void Shortest(std::vector<std::string> args) {
        LoadProject();
        if (args.size() != 2) {
            printf("Need two arguments to find the shortest path from one to the other\n");
            return;
        }
        Component* from = components[targetFrom(args[0])],
                 * to = components[targetFrom(args[1])];
        if (!from) {
            printf("No such component %s\n", args[0].c_str());
        } else if (!to) {
            printf("No such component %s\n", args[1].c_str());
        } else {
            FindSpecificLink(files, from, to);
        }
    }
    void Info(std::vector<std::string> args) {
        LoadProject();
        if (args.empty())
            printf("No targets specified to print info on...\n");
        for (auto& s : args) {
            PrintInfoOnTarget(components[targetFrom(s)]);
        }
    }
    void UsedBy(std::vector<std::string> args) {
        LoadProject();
        if (args.empty())
            printf("No files specified to find usage of...\n");
        for (auto& s : args) {
            printf("File %s is used by:\n", s.c_str());
            auto f = &files["./" + s];
            for (auto &p : files) {
                if (p.second.dependencies.find(f) != p.second.dependencies.end()) {
                    printf("  %s\n", p.second.path.string().c_str());
                }
            }
        }
    }
    void DoActualRegen(std::vector<std::string> args, bool dryRun) {
        LoadProject();
        boost::filesystem::current_path(projectRoot);
        if (args.empty()) {
            for (auto &c : components) {
                RegenerateCmakeFilesForComponent(c.second, dryRun);
            }
        } else {
            for (auto& s : args) {
                if (components.find(targetFrom(s)) != components.end()) {
                    RegenerateCmakeFilesForComponent(components[targetFrom(s)], dryRun);
                } else {
                    std::cout << "Target '" << targetFrom(s) << "' not found\n";
                }
            }
        }
    }
    void Regen(std::vector<std::string> args) {
        bool versionIsCorrect = CheckVersionFile();
        if (!versionIsCorrect)
            fprintf(stderr,
                    "Version of dependency checker not the same as the one used to generate the existing cmakelists. Refusing to regen\n"
                            "Please update " CONFIG_FILE " to version \"" CURRENT_VERSION "\" if you really want to do this.\n");

        DoActualRegen(args, !versionIsCorrect);
    }
    void DryRegen(std::vector<std::string> args) {
        DoActualRegen(args, true);
    }
    void Outliers(std::vector<std::string>) {
        LoadProject();
        PrintAllComponents(components, "Libraries with no links in:", [](const Component& c){
            return c.type == "library" && 
                !c.files.empty() && 
                c.pubLinks.empty() && c.privLinks.empty();
        });
        PrintAllComponents(components, "Libraries with too many outward links:", [](const Component& c){ return c.pubDeps.size() + c.privDeps.size() > Configuration::Get().componentLinkLimit; });
        PrintAllComponents(components, "Libraries with too few lines of code:", [](const Component& c) { return !c.files.empty() && c.loc() < Configuration::Get().componentLocLowerLimit; });
        PrintAllComponents(components, "Libraries with too many lines of code:", [](const Component& c) { return c.loc() > Configuration::Get().componentLocUpperLimit; });
        FindCircularDependencies(components);
        PrintAllComponents(components, "Libraries that are part of a cycle:", [](const Component& c) { return !c.circulars.empty(); });
        PrintAllFiles(files, "Files that are never used:", [](const File& f) { return !IsCompileableFile(f.path.extension().string()) && !f.hasInclude; });
        PrintAllFiles(files, "Files with too many lines of code:", [](const File& f) { return f.loc > Configuration::Get().fileLocUpperLimit; });
    }
    void Ambiguous(std::vector<std::string>) {
        LoadProject();
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
    }
    void Help(std::vector<std::string>) {
        printf("C++ Dependencies -- a tool to analyze large C++ code bases for #include dependency information\n");
        printf("Copyright (C) 2016, TomTom International BV\n");
        printf("Version " CURRENT_VERSION "\n");
        printf("\n");
        printf("  Usage:\n");
        printf("    %s [--dir <source-directory>] <command>\n", programName.c_str());
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
        printf("\n");
        printf("  Automatic CMakeLists.txt generation:\n");
        printf("     Note: These commands only have any effect on CMakeLists.txt marked with \"%s\"\n", Configuration::Get().regenTag.c_str());
        printf("  --regen                       : Re-generate all marked CMakeLists.txt with the component information derived.\n");
        printf("  --dryregen                    : Verify which CMakeLists would be regenerated if you were to run --regen now.\n");
    }
    bool projectLoaded;
    std::string programName;
    std::map<std::string, Command> commands;
    std::vector<std::string> args;
    std::unordered_set<std::string> ignorefiles;
    std::unordered_map<std::string, Component *> components;
    std::unordered_map<std::string, File> files;
    std::map<std::string, std::set<std::string>> collisions;
    std::unordered_map<std::string, std::string> includeLookup;
    std::map<std::string, std::vector<std::string>> ambiguous;
    std::set<std::string> deleteComponents;
    boost::filesystem::path outputRoot, projectRoot;
};

int main(int argc, char **argv) {
    Operations op(argc, argv);
    op.RunCommands();
    return 0;
}
