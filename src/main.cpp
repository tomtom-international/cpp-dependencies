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
#include "FstreamInclude.h"
#include "Input.h"
#include "Output.h"
#include <iostream>

static bool CheckVersionFile(const Configuration& config) {
    const std::string currentVersion = CURRENT_VERSION;
    return currentVersion == config.versionUsed;
}

static std::string targetFrom(const std::string &arg) {
    if (arg == "ROOT") {
      return ".";
    }
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
    , recursive(false)
    {
        if (filesystem::is_regular_file(CONFIG_FILE)) {
            streams::ifstream in(CONFIG_FILE);
            config.read(in);
        }
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
        if (lastCommandDidNothing) {
            std::cout << "\nThe last command you entered did not result in output, so in effect it did nothing.\n";
            std::cout << "Remember that commands are executed in the order in which they appear on the command-line, so\n";
            std::cout << "if you want an analysis-changing command (such as --infer) to change the analysis, it has to\n";
            std::cout << "come before the analysis you want it to affect.\n\n";
            std::cout << "You can also use this to run an analysis multiple times with a single change between them, or\n";
            std::cout << "to get various outputs from a single analysis run.\n";
        }
    }
private:
    typedef void (Operations::*Command)(std::vector<std::string>);
    void RegisterCommands() {
        commands["--ambiguous"] = &Operations::Ambiguous;
        commands["--cycles"] = &Operations::Cycles;
        commands["--dir"] = &Operations::Dir;
        commands["--drop"] = &Operations::Drop;
        commands["--dryregen"] = &Operations::DryRegen;
        commands["--fixincludes"] = &Operations::FixIncludes;
        commands["--graph-cycles"] = &Operations::GraphCycles;
        commands["--graph"] = &Operations::Graph;
        commands["--graph-target"] = &Operations::GraphTarget;
        commands["--help"] = &Operations::Help;
        commands["--ignore"] = &Operations::Ignore;
        commands["--includesize"] = &Operations::IncludeSize;
        commands["--infer"] = &Operations::Infer;
        commands["--info"] = &Operations::Info;
        commands["--inout"] = &Operations::InOut;
        commands["--outliers"] = &Operations::Outliers;
        commands["--recursive"] = &Operations::Recursive;
        commands["--regen"] = &Operations::Regen;
        commands["--shortest"] = &Operations::Shortest;
        commands["--stats"] = &Operations::Stats;
        commands["--usedby"] = &Operations::UsedBy;
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
        LoadFileList(config, components, files, projectRoot, inferredComponents, withLoc);
        CreateIncludeLookupTable(files, includeLookup, collisions);
        MapFilesToComponents(components, files);
        ForgetEmptyComponents(components);
        if (components.size() < 3) {
            std::cout << "Warning: Analyzing your project resulted in a very low amount of components. This either points to a small project, or\n";
            std::cout << "to cpp-dependencies not recognizing the components.\n\n";

            std::cout << "It tries to recognize components by the existence of project build files - CMakeLists.txt, Makefiles, MyProject.vcxproj\n";
            std::cout << "or similar files. If it does not recognize any such files, it will assume everything belongs to the project it is\n";
            std::cout << "contained in. You can invert this behaviour to assume that any code file will belong to a component local to it - in\n";
            std::cout << "effect, making every folder of code a single component - by using the --infer option.\n\n";

            std::cout << "Another reason for this warning may be running the tool in a folder that doesn't have any code. You can either change\n";
            std::cout << "to the desired directory, or use the --dir <myProject> option to make it analyze another directory.\n\n";
        }
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
        lastCommandDidNothing = false;
    }
    void UnloadProject() {
        components.clear();
        files.clear();
        collisions.clear();
        includeLookup.clear();
        ambiguous.clear();
        loadStatus = Unloaded;
        lastCommandDidNothing = true;
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
        for (auto& s : args) config.blacklist.insert(s);
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
            OutputFlatDependencies(config, components, args[0]);
        }
    }
    void GraphCycles(std::vector<std::string> args) {
        LoadProject();
        if (args.empty()) {
            std::cout << "No output file specified for cycle graph\n";
        } else {
            OutputCircularDependencies(config, components, args[0]);
        }
    }
    void GraphTarget(std::vector<std::string> args) {
        LoadProject();
        if (args.size() != 2) {
            std::cout << "--graph-target requires a single component and a single output file name.\n";
        } else {
            PrintGraphOnTarget(config, args[1], components[targetFrom(args[0])]);
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
            FindSpecificLink(config, files, from, to);
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
                RegenerateCmakeFilesForComponent(config, c.second, dryRun, false);
            }
        } else {
            bool writeToStdoutInstead = false;
            if (args[0] == "-") {
                dryRun = true; // Can't rewrite actual CMakeFiles if you asked them to be sent to stdout.
                writeToStdoutInstead = true;
                args.erase(args.begin());
            }
            for (auto& s : args) {
                const std::string target = targetFrom(s);
                if (recursive) {
                    for (auto& c : components) {
                        if (strstr(c.first.c_str(), target.c_str())) {
                            RegenerateCmakeFilesForComponent(config, c.second, dryRun, writeToStdoutInstead);
                        }
                    }
                } else {
                    if (components.find(target) != components.end()) {
                        RegenerateCmakeFilesForComponent(config, components[target], dryRun, writeToStdoutInstead);
                    } else {
                        std::cout << "Target '" << target << "' not found\n";
                    }
                }
            }
        }
    }
    void Regen(std::vector<std::string> args) {
        bool versionIsCorrect = CheckVersionFile(config);
        if (!versionIsCorrect)
            std::cout << "Version of dependency checker not the same as the one used to generate the existing cmakelists. Refusing to regen\n"
                            "Please update " CONFIG_FILE " to version \"" CURRENT_VERSION "\" if you really want to do this.\n";

        DoActualRegen(args, !versionIsCorrect);
    }
    void DryRegen(std::vector<std::string> args) {
        DoActualRegen(args, true);
    }
    void FixIncludes(std::vector<std::string> args) {
        if (args.size() < 2 || args.size() > 3) {
            std::cout << "Invalid input to fixincludes command\n";
            std::cout << "Required: --fixincludes <component> <desired path> [<relative root>]\n";
            std::cout << "Relative root can be \"project\" for absolute paths or \"component\" for component-relative paths\n";
            return;
        }

        LoadProject();
        bool absolute = args.size() == 3 && args[2] == "project";
        Component* c = components[targetFrom(args[0])];
        if (!c) {
            std::cout << "No such component " << args[0] << "\n";
        } else {
            UpdateIncludes(files, includeLookup, c, args[1], absolute);
        }
    }
    void Outliers(std::vector<std::string>) {
        LoadProject(true);
        PrintAllComponents(components, "Libraries with no links in:", [this](const Component& c){
            return config.addLibraryAliases.count(c.type) == 1 &&
                !c.files.empty() &&
                c.pubLinks.empty() && c.privLinks.empty();
        });
        PrintAllComponents(components, "Libraries with too many outward links:", [this](const Component& c){ return c.pubDeps.size() + c.privDeps.size() > config.componentLinkLimit; });
        PrintAllComponents(components, "Libraries with too few lines of code:", [this](const Component& c) { return !c.files.empty() && c.loc() < config.componentLocLowerLimit; });
        PrintAllComponents(components, "Libraries with too many lines of code:", [this](const Component& c) { return c.loc() > config.componentLocUpperLimit; });
        FindCircularDependencies(components);
        PrintAllComponents(components, "Libraries that are part of a cycle:", [](const Component& c) { return !c.circulars.empty(); });
        PrintAllFiles(files, "Files that are never used:", [](const File& f) { return !IsCompileableFile(f.path.extension().string()) && !f.hasInclude; });
        PrintAllFiles(files, "Files with too many lines of code:", [this](const File& f) { return f.loc > config.fileLocUpperLimit; });
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
    void Recursive(std::vector<std::string>) {
        recursive = true;
        UnloadProject();
    }
    void Help(std::vector<std::string>) {
        std::cout << "C++ Dependencies -- a tool to analyze large C++ code bases for #include dependency information\n";
        std::cout << "Copyright (C) 2016, TomTom International BV\n";
        std::cout << "Version " CURRENT_VERSION "\n";
        std::cout << "\n";
        std::cout << "  Usage:\n";
        std::cout << "    " << programName << " [--dir <source - directory>] <commands>\n";
        std::cout << "    Source directory is assumed to be the current one if unspecified\n";
        std::cout << "\n";
        std::cout << "  Commands:\n";
        std::cout << "    --help                           : Produce this help text\n";
        std::cout << "\n";
        std::cout << "    Extracting graphs:\n";
        std::cout << "    --graph <output>                 : Graph of all components with dependencies\n";
        std::cout << "    --graph-cycles <output>          : Graph of components with cyclic dependencies on other components\n";
        std::cout << "    --graph-target <target> <output> : Graph for all dependencies of a specific target\n";
        std::cout << "\n";
        std::cout << "    Getting information:\n";
        std::cout << "    --stats                          : Info about code base size, complexity and cyclic dependency count\n";
        std::cout << "    --cycles <targetname>            : Find all possible paths from this target back to itself\n";
        std::cout << "    --shortest                       : Determine shortest path between components and its reason\n";
        std::cout << "    --outliers                       : Finds all components and files that match a criterium for being out of the ordinary\n";
        std::cout << "                                            - libraries that are not used\n";
        std::cout << "                                            - components that use a lot of other components\n";
        std::cout << "                                            - components with dependencies towards executables\n";
        std::cout << "                                            - components with less than 200 LoC\n";
        std::cout << "                                            - components with more than 20 kLoC\n";
        std::cout << "                                            - components that are part of a cycle\n";
        std::cout << "                                            - files that are more than 2000 LoC\n";
        std::cout << "                                            - files that are not compiled and never included\n";
        std::cout << "    --includesize                    : Calculate the total number of lines added to each file through #include\n";
        std::cout << "\n";
        std::cout << "  Target information:\n";
        std::cout << "    --info                           : Show all information on a given specific target\n";
        std::cout << "    --usedby                         : Find all references to a specific header file\n";
        std::cout << "    --inout                          : Find all incoming and outgoing links for a target\n";
        std::cout << "    --ambiguous                      : Find all include statements that could refer to more than one header\n";
        std::cout << "\n";
        std::cout << "  Refactoring:\n";
        std::cout << "    --fixincludes <targetname> <desired path> [<relative root>]\n";
        std::cout << "                                     : Unify include paths for headers in <targetname> in  all source files.\n";
        std::cout << "                                       <relative root> can be:\n";
        std::cout << "                                            - \"project\" for absolute paths (default);\n";
        std::cout << "                                            - \"component\" for component-relative paths.\n";
        std::cout << "\n";
        std::cout << "  Automatic CMakeLists.txt generation:\n";
        std::cout << "     Note: These commands only have any effect on CMakeLists.txt marked with \"" << config.regenTag << "\"\n";
        std::cout << "    --regen                          : Re-generate all marked CMakeLists.txt with the component information derived.\n";
        std::cout << "    --dryregen                       : Verify which CMakeLists would be regenerated if you were to run --regen now.\n";
        std::cout << "\n";
        std::cout << "  What-if analysis:\n";
        std::cout << "     Note: These commands modify the analysis results and are intended for interactive analysis.\n";
        std::cout << "           They only affect the commands after their own position in the argument list. You can use them to\n";
        std::cout << "           analyze twice with a given what-if in between. For example: --stats --ignore myFile.h --stats\n";
        std::cout << "    --ignore                         : Ignore the following path(s) or file names in the analysis.\n";
        std::cout << "    --drop                           : Completely remove knowledge of a given component and re-analyze dependencies. Differs\n";
        std::cout << "                                       from ignoring the component as it will not disambiguate headers that are ambiguous\n";
        std::cout << "                                       because of this component\n";
        std::cout << "    --infer                          : Pretend that every folder that holds a source file is also a component.\n";
        std::cout << "    --dir <sourcedirectory>          : Source directory to run in. Assumed current one if unspecified.\n";
        std::cout << "    --recursive                      : If for the following command a single target/directory is specified\n";
        std::cout << "                                       recursively process the underlying targets/directories too.\n";
    }
    Configuration config;
    enum LoadStatus {
      Unloaded,
      FastLoad,
      FullLoad,
    } loadStatus;
    bool inferredComponents;
    bool lastCommandDidNothing;
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
    bool recursive;
};

int main(int argc, const char **argv) {
    Operations op(argc, argv);
    op.RunCommands();
    return 0;
}
