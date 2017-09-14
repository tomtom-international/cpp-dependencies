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
#include "Output.h"
#include <iostream>
#include <stack>

#ifndef _WIN32
#define CURSES_CYCLIC_DEPENDENCY "[33m"
#define CURSES_PUBLIC_DEPENDENCY "[34m"
#define CURSES_PRIVATE_DEPENDENCY "[36m"
#define CURSES_RESET_COLOR "[39;49m"
#else
#define CURSES_CYCLIC_DEPENDENCY ""
#define CURSES_PUBLIC_DEPENDENCY ""
#define CURSES_PRIVATE_DEPENDENCY ""
#define CURSES_RESET_COLOR ""
#endif

static const std::string& getLinkColor(const Configuration& config, Component *a, Component *b) {
    // One of the links in this chain must be broken because it ties together a bundle of apparently unrelated components
    if (a->circulars.find(b) != a->circulars.end()) {
        return config.cycleColor;
    }
    if (a->pubDeps.find(b) != a->pubDeps.end()) {
        return config.publicDepColor;
    }
    return config.privateDepColor;
}

static const char* getShapeForSize(Component* c) {
    size_t loc = c->loc();
    if (loc < 1000) {
        return "oval";
    } else if (loc < 5000) {
        return "doublecircle";
    } else if (loc < 20000) {
        return "octagon";
    } else if (loc < 50000) {
        return "doubleoctagon";
    } else {
        return "tripleoctagon";
    }
}

struct OstreamHolder {
  OstreamHolder(const filesystem::path& outFile) {
    if (outFile == "-") {
      isStdout = true;
    } else {
      isStdout = false;
      out.open(outFile);
    }
  }
  ::std::ostream& get() { return isStdout ? std::cout : out; }
  bool isStdout;
  streams::ofstream out;
};

void OutputFlatDependencies(const Configuration& config, std::unordered_map<std::string, Component *> &components, const filesystem::path &outfile) {
    OstreamHolder outHolder(outfile);
    std::ostream& out = outHolder.get();
    out << "digraph dependencies {" << '\n';
    for (const auto &c : components) {
        if (c.second->root.string().size() > 2 &&
            c.second->files.size()) {
            out << "  " << c.second->QuotedName() << " [shape=" << getShapeForSize(c.second) << "];\n";
        }

        std::set<Component *> depcomps;
        for (auto &d : c.second->privDeps) {
            if (d->root.string().size() > 2 &&
                d->files.size()) {
                if (depcomps.insert(d).second) {
                    out << "  " << c.second->QuotedName() << " -> " << d->QuotedName() << " [color="
                        << getLinkColor(config, c.second, d) << "];" << '\n';
                }
            }
        }
        for (auto &d : c.second->pubDeps) {
            if (d->root.string().size() > 2 &&
                d->files.size()) {
                if (depcomps.insert(d).second) {
                    out << "  " << c.second->QuotedName() << " -> " << d->QuotedName() << " [color="
                        << getLinkColor(config, c.second, d) << "];" << '\n';
                }
            }
        }
    }
    out << "}" << '\n';
}

void OutputCircularDependencies(const Configuration& config, std::unordered_map<std::string, Component *> &components,
                                const filesystem::path &outfile) {
    OstreamHolder outHolder(outfile);
    std::ostream& out = outHolder.get();
    out << "digraph dependencies {" << '\n';
    for (const auto &c : components) {
        if (c.second->circulars.empty()) {
            continue;
        }

        out << "  " << c.second->QuotedName() << " [shape=" << getShapeForSize(c.second) << "];\n";

        for (const auto &t : c.second->circulars) {
            out << "  " << c.second->QuotedName() << " -> " << t->QuotedName() << " [color="
                << getLinkColor(config, c.second, t) << "];" << '\n';
        }
    }
    out << "}" << '\n';
}

void PrintGraphOnTarget(const Configuration& config, const filesystem::path &outfile, Component *c) {
    if (!c) {
        std::cout << "Component does not exist (double-check spelling)\n";
        return;
    }
    OstreamHolder outHolder(outfile);
    std::ostream& out = outHolder.get();

    std::stack<Component *> todo;
    std::set<Component *> comps;
    todo.push(c);
    comps.insert(c);
    out << "digraph dependencies {" << '\n';
    while (!todo.empty()) {
        Component *c2 = todo.top();
        todo.pop();

        out << "  " << c2->QuotedName() << " [shape=" << getShapeForSize(c2) << "];\n";

        std::set<Component *> depcomps;
        for (auto &d : c2->privDeps) {
            if (d->root.string().size() > 2 &&
                d->files.size()) {
                if (depcomps.insert(d).second) {
                    out << "  " << c2->QuotedName() << " -> " << d->QuotedName() << " [color=" << getLinkColor(config, c2, d)
                        << "];" << '\n';
                }
                if (comps.insert(d).second) {
                    todo.push(d);
                }
            }
        }
        for (auto &d : c2->pubDeps) {
            if (d->root.string().size() > 2 &&
                d->files.size()) {
                if (depcomps.insert(d).second) {
                    out << "  " << c2->QuotedName() << " -> " << d->QuotedName() << " [color=" << getLinkColor(config, c2, d)
                        << "];" << '\n';
                }
                if (comps.insert(d).second) {
                    todo.push(d);
                }
            }
        }
    }
    out << "}" << '\n';
}

void FindAndPrintCycleFrom(Component *origin, Component *c, std::unordered_set<Component *> alreadyHad,
                           std::vector<Component *> order) {
    if (!alreadyHad.insert(c).second) {
        return;
    }
    order.push_back(c);
    for (auto &d : c->circulars) {
        if (d == origin) {
            for (auto &comp : order) {
                std::cout << comp->NiceName('.') << " -> ";
            }
            std::cout << origin->NiceName('.') << "\n";
        } else {
            FindAndPrintCycleFrom(origin, d, alreadyHad, order);
        }
    }
}

void PrintCyclesForTarget(Component *c) {
    std::unordered_set<Component *> alreadyHad;
    std::vector<Component *> order;
    FindAndPrintCycleFrom(c, c, alreadyHad, order);
}

void PrintLinksForTarget(Component *c) {
    std::vector<std::string> sortedPubLinks(SortedNiceNames(c->pubLinks));
    std::cout << "Public linked (" << sortedPubLinks.size() << "):";
    for (auto &d : sortedPubLinks) {
        std::cout << ' ' << d;
    }

    std::vector<std::string> sortedPrivLinks(SortedNiceNames(c->privLinks));
    std::cout << "Private linked (" << sortedPrivLinks.size() << "):";
    for (auto &d : sortedPrivLinks) {
        std::cout << ' ' << d;
    }
    std::cout << '\n';
}

void PrintInfoOnTarget(Component *c) {
    if (!c) {
        std::cout << "Component does not exist (double-check spelling)\n";
        return;
    }
    std::cout << "Root: " << c->root.string() << '\n';
    std::cout << "Name: " << c->name.c_str() << '\n';
    std::cout << "Type: " << c->type.c_str() << '\n';
    std::cout << "Lines of Code: " << c->loc() << '\n';
    std::cout << "Recreate: " << (c->recreate ? "true" : "false") << '\n';
    std::cout << "Has CMakeAddon.txt: " << (c->hasAddonCmake ? "true" : "false") << '\n';

    std::vector<std::string> sortedPubDeps(SortedNiceNames(c->pubDeps));
    std::cout << "Public dependencies (" << sortedPubDeps.size() << "): ";
    for (auto &d : sortedPubDeps) {
        std::cout << ' ' << d;
    }
    std::vector<std::string> sortedPrivDeps(SortedNiceNames(c->privDeps));
    std::cout << "\nPrivate dependencies (" << sortedPrivDeps.size() << "): ";
    for (auto &d : sortedPrivDeps) {
        std::cout << ' ' << d;
    }
    std::cout << "\nBuild-afters:";
    for (auto &d : c->buildAfters) {
        std::cout << ' ' << d;
    }
    std::cout << "\nFiles (" << c->files.size() << "):";
    for (auto &d : c->files) {
        std::cout << ' ' << d->path.string();
    }
    std::cout << '\n';
    PrintLinksForTarget(c);
    std::cout << '\n';
}

void PrintAllComponents(std::unordered_map<std::string, Component *> &components, const char* description, std::function<bool(const Component&)> predicate) {
  std::vector<std::string> selected;
  for (auto& c : components) {
    if (predicate(*c.second)) {
      selected.push_back(c.second->NiceName('.'));
    }
  }
  if (selected.empty()) return;

  std::cout << description << '\n';
  std::sort(selected.begin(), selected.end());
  for (auto& c : selected) {
    std::cout << "  " << c << '\n';
  }
  std::cout << '\n';
}

void PrintAllFiles(std::unordered_map<std::string, File>& files, const char* description, std::function<bool(const File&)> predicate) {
  std::vector<std::string> selected;
  for (auto& f : files) {
    if (predicate(f.second)) {
      selected.push_back(f.second.path.string().c_str());
    }
  }
  if (selected.empty()) return;

  std::cout << description << '\n';
  std::sort(selected.begin(), selected.end());
  for (auto& c : selected) {
    std::cout << "  " << c << '\n';
  }
  std::cout << '\n';
}

void FindSpecificLink(const Configuration& config, std::unordered_map<std::string, File>& files, Component *from, Component *to) {
    std::unordered_map<Component *, Component *> parents;
    std::unordered_set<Component *> alreadyHad;
    std::deque<Component *> tocheck;
    tocheck.push_back(from);
    while (!tocheck.empty()) {
        Component *c = tocheck.front();
        tocheck.pop_front();
        if (alreadyHad.find(c) == alreadyHad.end()) {
            alreadyHad.insert(c);
            if (c == to) {
                std::vector<Component *> links;
                while (c != from) {
                    links.push_back(c);
                    c = parents[c];
                }
                Component *p = c;
                while (!links.empty()) {
                    Component *c2 = links.back();
                    links.pop_back();
                    std::string color = getLinkColor(config, p, c2);
                    if (color == config.cycleColor) {
                        std::cout << CURSES_CYCLIC_DEPENDENCY;
                    } else if (color == config.publicDepColor) {
                        std::cout << CURSES_PUBLIC_DEPENDENCY;
                    } else {
                        std::cout << CURSES_PRIVATE_DEPENDENCY;
                    }
                    std::cout << p->NiceName('.') << " -> " << c2->NiceName('.') << '\n';
                    std::cout << CURSES_RESET_COLOR;
                    for (auto &f : files) {
                        if (f.second.component == p) {
                            for (auto &f2 : f.second.dependencies) {
                                if (f2->component == c2) {
                                    std::cout << "  " << f.second.path.string() << " includes " << f2->path.string() << '\n';
                                }
                            }
                        }
                    }
                    p = c2;
                }
                return;
            }
            for (auto &d : c->pubDeps) {
                tocheck.push_back(d);
                Component *&p = parents[d];
                if (!p) {
                    p = c;
                }
            }
            for (auto &d : c->privDeps) {
                tocheck.push_back(d);
                Component *&p = parents[d];
                if (!p) {
                    p = c;
                }
            }
        }
    }

    std::cout << "No path could be found from " << from->NiceName('.') << " to " << to->NiceName('.') << '\n';
}

static void UpdateIncludeFor(std::unordered_map<std::string, File>& files, std::unordered_map<std::string, std::string> &includeLookup, File* from, Component* comp, const std::string& desiredPath, bool isAbsolute) {
    filesystem::path newName = from->path.generic_string() + ".new";
    {
        streams::ifstream in(from->path);
        streams::ofstream out(newName.generic_string().c_str());
        while (in.good()) {
            bool isReplacement = false;
            std::string line;
            std::getline(in, line);
            const char* l = strstr(line.c_str(), "#");
            if (l && (l = strstr(l, "include"))) {
                const char* start = strstr(l, "<");
                const char* end;
                if (start) {
                    end = strstr(start + 1, ">");
                } else {
                    start = strstr(l, "\"");
                    end = start ? strstr(start+1, "\"") : nullptr;
                }
                if (start && end) {
                    std::string includePath(start+1, end);
                    std::string lowerPath;
                    std::transform(includePath.begin(), includePath.end(), std::back_inserter(lowerPath), ::tolower);
                    std::string postLookup = includeLookup[lowerPath];
                    if (!postLookup.empty() && postLookup != "INVALID" && files.find(postLookup) != files.end()) {
                        File* f = &files.find(postLookup)->second;
                        std::string path = f->path.generic_string();
                        std::string pathToStrip = (isAbsolute ? "." : comp->root.generic_string()) + "/";
                        if (desiredPath != ".") pathToStrip += desiredPath + "/";
                        std::string newInclude = path.substr(pathToStrip.size());
                        std::string componentPath = comp->root.generic_string();
                        // Don't make an include ambiguous by doing this change
                        if (includeLookup[newInclude] != "INVALID" && path.compare(0, componentPath.size(), componentPath) == 0 && path.compare(0, pathToStrip.size(), pathToStrip) == 0) {
                            isReplacement = true;
                            if (from->component == f->component) {
                                out << "#include \"" + newInclude + "\"\n";
                            } else {
                                out << "#include <" + newInclude + ">\n";
                            }
                        }
                    }
                }
            }
            if (!isReplacement) {
                out << line << '\n';
            }
        }
    }
    filesystem::rename(newName, from->path);
}

void UpdateIncludes(std::unordered_map<std::string, File>& files, std::unordered_map<std::string, std::string> &includeLookup, Component* component, const std::string& desiredPath, bool isAbsolute) {
    for (auto& p : files) {
        for (auto& d : p.second.dependencies) {
            if (component->files.find(d) != component->files.end()) {
                UpdateIncludeFor(files, includeLookup, &p.second, component, desiredPath, isAbsolute);
                std::cout << p.second.path.generic_string() << "\n";
                break;
            }
        }
    }
}


