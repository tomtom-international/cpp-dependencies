#include "test.h"
#include "Analysis.h"


TEST(FindCircularDependenciesFindsNothingInADisconnectedGraph) {
  std::unordered_map<std::string, Component *> components;
  for (int n = 0; n < 10; n++) {
    char compName[12];
    sprintf(compName, "%d", n);
    components[compName] = new Component(compName);
  }

  FindCircularDependencies(components);
  for (auto& p : components) {
    ASSERT(p.second->circulars.empty());
  }
}

TEST(FindCircularDependenciesFindsNothingInAHeavilyConnectedTree) {
  std::unordered_map<std::string, Component *> components;
  std::vector<Component*> allSoFar;
  for (int n = 0; n < 10; n++) {
    char compName[12];
    sprintf(compName, "%d", n);
    Component* thisOne = components[compName] = new Component(compName);
    for (auto& c : allSoFar) {
      thisOne->pubDeps.insert(c);
    }
    allSoFar.push_back(thisOne);
  }

  FindCircularDependencies(components);
  for (auto& p : components) {
    ASSERT(p.second->circulars.empty());
  }
}

TEST(FindCircularDependenciesFindsASimpleCycle) {
  std::unordered_map<std::string, Component *> components;
  Component* a = components["a"] = new Component("a");
  Component* b = components["b"] = new Component("b");
  a->pubDeps.insert(b);
  b->pubDeps.insert(a);

  FindCircularDependencies(components);

  ASSERT(a->circulars.size() == 1);
  ASSERT(*a->circulars.begin() == b);
  ASSERT(b->circulars.size() == 1);
  ASSERT(*b->circulars.begin() == a);
}

TEST(FindCircularDependenciesSeesLargeCycle) {
  std::unordered_map<std::string, Component *> components;
  Component* a = components["a"] = new Component("a");
  Component* b = components["b"] = new Component("b");
  Component* c = components["c"] = new Component("c");
  Component* d = components["d"] = new Component("d");
  Component* e = components["e"] = new Component("e");
  a->pubDeps.insert(b);
  b->privDeps.insert(c);
  c->pubDeps.insert(d);
  d->privDeps.insert(e);
  e->pubDeps.insert(a);

  FindCircularDependencies(components);

  ASSERT(NodesWithCycles(components) == 5);
  ASSERT(a->circulars.size() == 1);
  ASSERT(*a->circulars.begin() == b);
  ASSERT(b->circulars.size() == 1);
  ASSERT(*b->circulars.begin() == c);
  ASSERT(c->circulars.size() == 1);
  ASSERT(*c->circulars.begin() == d);
  ASSERT(d->circulars.size() == 1);
  ASSERT(*d->circulars.begin() == e);
  ASSERT(e->circulars.size() == 1);
  ASSERT(*e->circulars.begin() == a);
}

TEST(FindCircularDependenciesSeesNoCycleAfterDroppingOneComponent) {
  std::unordered_map<std::string, Component *> components;
  Component* a = components["a"] = new Component("a");
  Component* b = components["b"] = new Component("b");
  Component* c = components["c"] = new Component("c");
  Component* d = components["d"] = new Component("d");
  Component* e = components["e"] = new Component("e");
  a->pubDeps.insert(b);
  b->privDeps.insert(c);
  c->pubDeps.insert(d);
  d->privDeps.insert(e);
  e->pubDeps.insert(a);

  FindCircularDependencies(components);
  KillComponent(components, "a");
  ASSERT(NodesWithCycles(components) == 0);
}

