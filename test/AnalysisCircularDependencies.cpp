#include <gtest/gtest.h>
#include "Analysis.h"

class AnalysisCircularFixture : public testing::Test
{
protected:
  std::unordered_map<std::string, Component *> components;
};

TEST_F(AnalysisCircularFixture, FindCircularDependenciesFindsNothingInADisconnectedGraph) {
  for (int n = 0; n < 10; n++) {
    char name[12];
    sprintf(name, "%d", n);
    components[name] = new Component(name);
  }

  FindCircularDependencies(components);
  for (auto& p : components) {
    ASSERT_TRUE(p.second->circulars.empty());
  }
}

TEST_F(AnalysisCircularFixture, FindCircularDependenciesFindsNothingInAHeavilyConnectedTree) {
  std::vector<Component*> allSoFar;
  for (int n = 0; n < 10; n++) {
    char name[12];
    sprintf(name, "%d", n);
    Component* thisOne = components[name] = new Component(name);
    for (auto& c : allSoFar) {
      thisOne->pubDeps.insert(c);
    }
    allSoFar.push_back(thisOne);
  }

  FindCircularDependencies(components);
  for (auto& p : components) {
    ASSERT_TRUE(p.second->circulars.empty());
  }
}

TEST_F(AnalysisCircularFixture, FindCircularDependenciesFindsASimpleCycle) {
  Component* a = components["a"] = new Component("a");
  Component* b = components["b"] = new Component("b");
  a->pubDeps.insert(b);
  b->pubDeps.insert(a);

  FindCircularDependencies(components);

  ASSERT_TRUE(a->circulars.size() == 1);
  ASSERT_TRUE(*a->circulars.begin() == b);
  ASSERT_TRUE(b->circulars.size() == 1);
  ASSERT_TRUE(*b->circulars.begin() == a);
}

TEST_F(AnalysisCircularFixture, FindCircularDependenciesSeesLargeCycle) {
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

  ASSERT_TRUE(a->circulars.size() == 1);
  ASSERT_TRUE(*a->circulars.begin() == b);
  ASSERT_TRUE(b->circulars.size() == 1);
  ASSERT_TRUE(*b->circulars.begin() == c);
  ASSERT_TRUE(c->circulars.size() == 1);
  ASSERT_TRUE(*c->circulars.begin() == d);
  ASSERT_TRUE(d->circulars.size() == 1);
  ASSERT_TRUE(*d->circulars.begin() == e);
  ASSERT_TRUE(e->circulars.size() == 1);
  ASSERT_TRUE(*e->circulars.begin() == a);
}


