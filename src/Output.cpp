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
#include "Output.h"
#include "Constants.h"
#include "Configuration.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#define CURSES_CYCLIC_DEPENDENCY "[33m"
#define CURSES_PUBLIC_DEPENDENCY "[34m"
#define CURSES_PRIVATE_DEPENDENCY "[36m"
#define CURSES_RESET_COLOR "[39;49m"

static const std::string& getLinkColor(Component *a, Component *b) {
    // One of the links in this chain must be broken because it ties together a bundle of apparently unrelated components
    if (a->circulars.find(b) != a->circulars.end()) {
        return Configuration::Get().cycleColor;
    }
    if (a->pubDeps.find(b) != a->pubDeps.end()) {
        return Configuration::Get().publicDepColor;
    }
    return Configuration::Get().privateDepColor;
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

void OutputFlatDependencies(const boost::filesystem::path &outfile) {
    boost::filesystem::ofstream out(outfile);
    out << "digraph dependencies {" << "\n";
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
                        << getLinkColor(c.second, d) << "];" << "\n";
                }
            }
        }
        for (auto &d : c.second->pubDeps) {
            if (d->root.string().size() > 2 &&
                d->files.size()) {
                if (depcomps.insert(d).second) {
                    out << "  " << c.second->QuotedName() << " -> " << d->QuotedName() << " [color="
                        << getLinkColor(c.second, d) << "];" << "\n";
                }
            }
        }
    }
    out << "}" << "\n";
}

void OutputCircularDependencies(const boost::filesystem::path &outfile) {
    boost::filesystem::ofstream out(outfile);
    out << "digraph dependencies {" << "\n";
    for (const auto &c : components) {
        if (c.second->circulars.empty()) {
            continue;
        }

        out << "  " << c.second->QuotedName() << "\n";

        for (const auto &t : c.second->circulars) {
            out << "  " << c.second->QuotedName() << " -> " << t->QuotedName() << " [color="
                << getLinkColor(c.second, t) << "];" << "\n";
        }
    }
    out << "}" << "\n";
}

void PrintGraphOnTarget(const boost::filesystem::path &outfile, Component *c) {
    if (!c) {
        printf("Component does not exist (doublecheck spelling)\n");
        return;
    }
    boost::filesystem::ofstream out(outfile);

    std::stack<Component *> todo;
    std::set<Component *> comps;
    todo.push(c);
    comps.insert(c);
    out << "digraph dependencies {" << "\n";
    while (!todo.empty()) {
        Component *c2 = todo.top();
        todo.pop();

        out << "  " << c2->QuotedName() << "];" << "\n";

        std::set<Component *> depcomps;
        for (auto &d : c2->privDeps) {
            if (d->root.string().size() > 2 &&
                d->files.size()) {
                if (depcomps.insert(d).second) {
                    out << "  " << c2->QuotedName() << " -> " << d->QuotedName() << " [color=" << getLinkColor(c2, d)
                        << "];" << "\n";
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
                    out << "  " << c2->QuotedName() << " -> " << d->QuotedName() << " [color=" << getLinkColor(c2, d)
                        << "];" << "\n";
                }
                if (comps.insert(d).second) {
                    todo.push(d);
                }
            }
        }
    }
    out << "}" << "\n";
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
                printf("%s -> ", comp->NiceName('.').c_str());
            }
            printf("%s\n", origin->NiceName('.').c_str());
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
    printf("Public linked (%zu):", sortedPubLinks.size());
    for (auto &d : sortedPubLinks) {
        printf(" %s", d.c_str());
    }

    std::vector<std::string> sortedPrivLinks(SortedNiceNames(c->privLinks));
    printf("\nPrivate linked (%zu):", sortedPrivLinks.size());
    for (auto &d : sortedPrivLinks) {
        printf(" %s", d.c_str());
    }
    printf("\n");
}

void PrintInfoOnTarget(Component *c) {
    if (!c) {
        printf("Component does not exist (doublecheck spelling)\n");
        return;
    }
    printf("Root: %s\n", c->root.c_str());
    printf("Name: %s\n", c->name.c_str());
    printf("Type: %s\n", c->type.c_str());
    printf("Lines of Code: %zu\n", c->loc());
    printf("Recreate: %s\n", c->recreate ? "true" : "false");
    printf("Has CMakeAddon.txt: %s\n", c->hasAddonCmake ? "true" : "false");

    std::vector<std::string> sortedPubDeps(SortedNiceNames(c->pubDeps));
    printf("Public dependencies (%zu): ", sortedPubDeps.size());
    for (auto &d : sortedPubDeps) {
        printf(" %s", d.c_str());
    }
    std::vector<std::string> sortedPrivDeps(SortedNiceNames(c->privDeps));
    printf("\nPrivate dependencies (%zu):", sortedPrivDeps.size());
    for (auto &d : sortedPrivDeps) {
        printf(" %s", d.c_str());
    }
    printf("\nBuild-afters:");
    for (auto &d : c->buildAfters) {
        printf(" %s", d.c_str());
    }
    printf("\nFiles (%zu):", c->files.size());
    for (auto &d : c->files) {
        printf(" %s", d->path.string().c_str());
    }
    printf("\n");
    PrintLinksForTarget(c);
    printf("\n");
}

void PrintAllComponents(const char* description, bool (*predicate)(const Component&)) {
  std::vector<std::string> selected;
  for (auto& c : components) {
    if (predicate(*c.second)) {
      selected.push_back(c.second->NiceName('.'));
    }
  }
  if (selected.empty()) return;

  printf("%s\n", description);
  std::sort(selected.begin(), selected.end());
  for (auto& c : selected) {
    printf("  %s\n", c.c_str());
  }
  printf("\n");
}

void PrintAllFiles(const char* description, bool (*predicate)(const File&)) {
  std::vector<std::string> selected;
  for (auto& f : files) {
    if (predicate(f.second)) {
      selected.push_back(f.second.path.string().c_str());
    }
  }
  if (selected.empty()) return;

  printf("%s\n", description);
  std::sort(selected.begin(), selected.end());
  for (auto& c : selected) {
    printf("  %s\n", c.c_str());
  }
  printf("\n");
}

void FindSpecificLink(Component *from, Component *to) {
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
                    std::string color = getLinkColor(p, c2);
                    if (color == Configuration::Get().cycleColor) {
                        printf(CURSES_CYCLIC_DEPENDENCY);
                    } else if (color == Configuration::Get().publicDepColor) {
                        printf(CURSES_PUBLIC_DEPENDENCY);
                    } else {
                        printf(CURSES_PRIVATE_DEPENDENCY);
                    }
                    printf("%s -> %s\n", p->NiceName('.').c_str(), c2->NiceName('.').c_str());
                    printf(CURSES_RESET_COLOR);
                    for (auto &f : files) {
                        if (f.second.component == p) {
                            for (auto &f2 : f.second.dependencies) {
                                if (f2->component == c2) {
                                    printf("  %s includes %s\n", f.second.path.string().c_str(),
                                           f2->path.string().c_str());
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

    printf("No path could be found from %s to %s\n", from->NiceName('.').c_str(), to->NiceName('.').c_str());
}

