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

#include "Analysis.h"
#include "CmakeRegen.h"
#include "Component.h"
#include "Configuration.h"
#include "Constants.h"
#include "FilesystemInclude.h"
#include "Input.h"
#include "Output.h"
#include <iostream>

static bool CheckVersionFile() {
    const std::string currentVersion = CURRENT_VERSION;
    return currentVersion == Configuration::Get().versionUsed;
}

std::string targetFrom(const std::string &arg) {
    std::string target = arg;
    if (!target.empty() && target.back() == '/') {
        target.erase(target.end() - 1);
    }
    std::replace(target.begin(), target.end(), '.', '/');
    return "./" + target;
}

class Operations {
public:
    Operations(int argc, const char** argv)
    : loadStatus(Unloaded)
    , inferredComponents(false)
    , programName(argv[0])
    , allArgs(argv+1, argv+argc)
    {
        RegisterCommands();
        projectRoot = outputRoot = filesystem::current_path();
    }
    void RunCommands() {
        if (allArgs.empty()) {
            allArgs.push_back("--help");
        }
        std::vector<std::string>::iterator it = allArgs.begin(), end = allArgs.end();
        while (it != end) {
            std::vector<std::string>::iterator localEnd = it;
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
        commands["--regen"] = &Operations::Regen;
        commands["--dryregen"] = &Operations::DryRegen;
        commands["--dir"] = &Operations::Dir;
        commands["--infer"] = &Operations::Infer;
        commands["--includesize"] = &Operations::IncludeSize;
    }
    void RunCommand(std::vector<std::string>::iterator &arg, std::vector<std::string>::iterator &end) {
        std::string lowerCommand;
        std::transform(arg->begin(), arg->end(), std::back_inserter(lowerCommand), ::tolower);
        ++arg;

        Command c = commands[lowerCommand];
        if (!c) c = commands["--help"];
        (this->*c)(std::vector<std::string>(arg, end));
    }
    void LoadProject(bool withLoc = false) {
        if (!withLoc && loadStatus >= FastLoad) return;
        if (withLoc && loadStatus >= FullLoad) return;
        LoadFileList(components, files, projectRoot, inferredComponents, withLoc);
        CreateIncludeLookupTable(files, includeLookup, collisions);
        MapFilesToComponents(components, files);
        MapIncludesToDependencies(includeLookup, ambiguous, components, files);
        for (auto &i : ambiguous) {
            for (auto &c : collisions[i.first]) {
                files.find(c)->second.hasInclude = true; // There is at least one include that might end up here.
            }
        }
        PropagateExternalIncludes(files);
        ExtractPublicDependencies(components);
        FindCircularDependencies(components);
        for (auto& c : deleteComponents) {
            KillComponent(components, c);
        }
        loadStatus = (withLoc ? FullLoad : FastLoad);
    }
    void UnloadProject() {
        components.clear();
        files.clear();
        collisions.clear();
        includeLookup.clear();
        ambiguous.clear();
        loadStatus = Unloaded;
    }
    void Dir(std::vector<std::string> args) {
        if (args.empty()) {
            std::cout << "No directory specified after --dir\n";
        } else {
            projectRoot = args[0];
        }
        UnloadProject();
    }
    void Ignore(std::vector<std::string> args) {
        if (args.empty())
            std::cout << "No files specified to ignore?\n";
        for (auto& s : args) Configuration::Get().blacklist.push_back(s);
        UnloadProject();
    }
    void Infer(std::vector<std::string> ) {
        inferredComponents = true;
        UnloadProject();
    }
    void Drop(std::vector<std::string> args) {
        if (args.empty())
            std::cout << "No files specified to ignore?\n";
        for (auto& s : args) deleteComponents.insert(s);
        UnloadProject();
    }
    void Graph(std::vector<std::string> args) {
        LoadProject();
        if (args.empty()) {
            std::cout << "No output file specified for graph\n";
        } else {
            OutputFlatDependencies(components, args[0]);
        }
    }
    void GraphCycles(std::vector<std::string> args) {
        LoadProject();
        if (args.empty()) {
            std::cout << "No output file specified for cycle graph\n";
        } else {
            OutputCircularDependencies(components, args[0]);
        }
    }
    void GraphTarget(std::vector<std::string> args) {
        LoadProject();
        if (args.size() != 2) {
            std::cout << "--graph-target requires a single component and a single output file name.\n";
        } else {
            PrintGraphOnTarget(args[1], components[targetFrom(args[0])]);
        }
    }
    void Cycles(std::vector<std::string> args) {
        LoadProject();
        if (args.empty()) {
            std::cout << "No targets specified for finding in- and out-going links.\n";
        } else {
            PrintCyclesForTarget(components[targetFrom(args[0])]);
        }
    }
    void Stats(std::vector<std::string>) {
        LoadProject(true);
        std::size_t totalPublicLinks(0), totalPrivateLinks(0);
        for (const auto &c : components) {
            totalPublicLinks += c.second->pubDeps.size();
            totalPrivateLinks += c.second->privDeps.size();
        }
        std::cout << components.size() << " components with "
            << totalPublicLinks << " public dependencies, "
            << totalPrivateLinks << " private dependencies\n";
        std::cout << "Detected " << NodesWithCycles(components) << " nodes in cycles\n";
    }
    void InOut(std::vector<std::string> args) {
        LoadProject();
        if (args.empty())
            std::cout << "No targets specified for finding in- and out-going links.\n";
        for (auto& s : args) {
            PrintLinksForTarget(components[targetFrom(s)]);
        }
    }
    void Shortest(std::vector<std::string> args) {
        LoadProject();
        if (args.size() != 2) {
            std::cout << "Need two arguments to find the shortest path from one to the other\n";
            return;
        }
        Component* from = components[targetFrom(args[0])],
                 * to = components[targetFrom(args[1])];
        if (!from) {
            std::cout << "No such component " << args[0] << "\n";
        } else if (!to) {
            std::cout << "No such component " << args[1] << "\n";
        } else {
            FindSpecificLink(files, from, to);
        }
    }
    void Info(std::vector<std::string> args) {
        LoadProject();
        if (args.empty())
            std::cout << "No targets specified to print info on...\n";
        for (auto& s : args) {
            PrintInfoOnTarget(components[targetFrom(s)]);
        }
    }
    void UsedBy(std::vector<std::string> args) {
        LoadProject();
        if (args.empty())
            std::cout << "No files specified to find usage of...\n";
        for (auto& s : args) {
            std::cout << "File " << s << " is used by:\n";
            File* f = &files.find("./" + s)->second;
            for (auto &p : files) {
                if (p.second.dependencies.find(f) != p.second.dependencies.end()) {
                    std::cout << "  " << p.second.path.string() << "\n";
                }
            }
        }
    }
    void DoActualRegen(std::vector<std::string> args, bool dryRun) {
        LoadProject();
        filesystem::current_path(projectRoot);
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
            std::cout << "Version of dependency checker not the same as the one used to generate the existing cmakelists. Refusing to regen\n"
                            "Please update " CONFIG_FILE " to version \"" CURRENT_VERSION "\" if you really want to do this.\n";

        DoActualRegen(args, !versionIsCorrect);
    }
    void DryRegen(std::vector<std::string> args) {
        DoActualRegen(args, true);
    }
    void Outliers(std::vector<std::string>) {
        LoadProject(true);
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
    void IncludeSize(std::vector<std::string>) {
        LoadProject(true);
        std::unordered_map<File*, size_t> includeCount;
        for (auto& f : files) {
            if (f.second.hasInclude) continue;
            std::set<File*> filesIncluded;
            std::vector<File*> todo;
            todo.push_back(&f.second);
            while (!todo.empty()) {
                File* file_todo = todo.back();
                todo.pop_back();
                for (auto& d : file_todo->dependencies) {
                    if (filesIncluded.insert(d).second) todo.push_back(d);
                }
            }

            size_t total = 0;
            for (auto& i : filesIncluded) { 
                total += i->loc;
                i->includeCount++;
            }
        }
        for (auto& f : files) {
            if (!f.second.hasInclude) continue;
            std::set<File*> filesIncluded;
            std::vector<File*> todo;
            todo.push_back(&f.second);
            while (!todo.empty()) {
                File* todo_file = todo.back();
                todo.pop_back();
                for (auto& d : todo_file->dependencies) {
                    if (filesIncluded.insert(d).second) todo.push_back(d);
                }
            }
            size_t total = 0;
            for (auto& i : filesIncluded) total += i->loc;
            if (f.second.includeCount > 0 && total > 0) {
                std::cout << total << " LOC used " << f.second.includeCount << " times from " << f.second.path.string() << "\n";
                std::cout << "impact " << f.second.includeCount * total << " for " << f.second.path.string() << "\n";
            }
        }
    }
    void Ambiguous(std::vector<std::string>) {
        LoadProject();
        std::cout << "Found " << ambiguous.size() << " ambiguous includes\n\n";
        for (auto &i : ambiguous) {
            std::cout << "Include for " << i.first << "\nFound in : \n";
            for (auto &s : i.second) {
                std::cout << "  included from " << s << "\n";
            }
            std::cout << "Options for file:\n";
            for (auto &c : collisions[i.first]) {
                std::cout << "  " << c << "\n";
            }
            std::cout << "\n";
        }
    }
    void Help(std::vector<std::string>) {
        std::cout << "C++ Dependencies -- a tool to analyze large C++ code bases for #include dependency information\n";
        std::cout << "Copyright (C) 2016, TomTom International BV\n";
        std::cout << "Version " CURRENT_VERSION "\n";
        std::cout << "\n";
        std::cout << "  Usage:\n";
        std::cout << "    " << programName << " [--dir <source - directory>] <command>\n";
        std::cout << "    Source directory is assumed to be the current one if unspecified\n";
        std::cout << "\n";
        std::cout << "  Commands:\n";
        std::cout << "  --help                           : Produce this help text\n";
        std::cout << "\n";
        std::cout << "  Extracting graphs:\n";
        std::cout << "  --graph <output>                 : Graph of all components with dependencies\n";
        std::cout << "  --graph-cycles <output>          : Graph of components with cyclic dependencies on other components\n";
        std::cout << "  --graph-target <target> <output> : Graph for all dependencies of a specific target\n";
        std::cout << "\n";
        std::cout << "  Getting information:\n";
        std::cout << "  --stats                          : Info about code base size, complexity and cyclic dependency count\n";
        std::cout << "  --cycles <targetname>            : Find all possible paths from this target back to itself\n";
        std::cout << "  --shortest                       : Determine shortest path between components and its reason\n";
        std::cout << "  --outliers                       : Finds all components and files that match a criterium for being out of the ordinary\n";
        std::cout << "                                          - libraries that are not used\n";
        std::cout << "                                          - components that use a lot of other components\n";
        std::cout << "                                          - components with dependencies towards executables\n";
        std::cout << "                                          - components with less than 200 LoC\n";
        std::cout << "                                          - components with more than 20 kLoC\n";
        std::cout << "                                          - components that are part of a cycle\n";
        std::cout << "                                          - files that are more than 2000 LoC\n";
        std::cout << "                                          - files that are not compiled and never included\n";
        std::cout << "  --includesize                    : For every file, calculate its contribution when included, and the total impact on\n";
        std::cout << "                                     the project, for each header\n";
        std::cout << "\n";
        std::cout << "  Target information:\n";
        std::cout << "  --info                           : Show all information on a given specific target\n";
        std::cout << "  --usedby                         : Find all references to a specific header file\n";
        std::cout << "  --inout                          : Find all incoming and outgoing links for a target\n";
        std::cout << "  --ambiguous                      : Find all include statements that could refer to more than one header\n";
        std::cout << "\n";
        std::cout << "  Automatic CMakeLists.txt generation:\n";
        std::cout << "     Note: These commands only have any effect on CMakeLists.txt marked with \"" << Configuration::Get().regenTag << "\"\n";
        std::cout << "  --regen                          : Re-generate all marked CMakeLists.txt with the component information derived.\n";
        std::cout << "  --dryregen                       : Verify which CMakeLists would be regenerated if you were to run --regen now.\n";
        std::cout << "\n";
        std::cout << "  What-if analysis:\n";
        std::cout << "     Note: These commands modify the analysis results and are intended for interactive analysis.\n";
        std::cout << "           They only affect the commands after their own position in the argument list. You can use them to\n";
        std::cout << "           analyze twice with a given what-if in between. For example: --stats --ignore myFile.h --stats\n";
        std::cout << "  --ignore                         : Ignore the following path(s) or file names in the analysis.\n";
        std::cout << "  --drop                           : Completely remove knowledge of a given component and re-analyze dependencies. Differs\n";
        std::cout << "                                     from ignoring the component as it will not disambiguate headers that are ambiguous\n";
        std::cout << "                                     because of this component\n";
        std::cout << "  --infer                          : Pretend that every folder that holds a source file is also a component.\n";
    }
    enum LoadStatus {
      Unloaded,
      FastLoad,
      FullLoad,
    } loadStatus;
    bool inferredComponents;
    std::string programName;
    std::map<std::string, Command> commands;
    std::vector<std::string> allArgs;
    std::unordered_map<std::string, Component *> components;
    std::unordered_map<std::string, File> files;
    std::map<std::string, std::set<std::string>> collisions;
    std::unordered_map<std::string, std::string> includeLookup;
    std::map<std::string, std::vector<std::string>> ambiguous;
    std::set<std::string> deleteComponents;
    filesystem::path outputRoot, projectRoot;
};

int main(int argc, const char **argv) {
    Operations op(argc, argv);
    op.RunCommands();
    return 0;
}
