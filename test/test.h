#ifndef TEST_H
#define TEST_H

#include <stdexcept>
#include "compiler_feature_detection.h"

class Test {
public:
  virtual void Run() = 0;
  static int RunAll();
protected:
  Test(const char* name) : name(name), nextTest(firstTest) 
  { 
    firstTest = this; 
  }
private:
  const char* name;
  Test* nextTest;
  static Test* firstTest;
};

#define TEST(x) static struct Test##x : public Test { Test##x() : Test(#x) {} void Run() OPTIONAL_FEATURE_OVERRIDE; } __inst_##x; void Test##x::Run()
#define ASSERT(x) do { auto v = (x); if (!v) { throw std::runtime_error("Assertion failed: " #x); } } while(0)

#endif


