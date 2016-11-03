#ifndef TEST_H
#define TEST_H

#include <stdexcept>

class Test {
public:
  Test(const char* name) : name(name) {}
  virtual void Run() = 0;
  Test* nextTest;
  static Test* firstTest;
  const char* name;
};

#define TEST(x) static struct Test##x : public Test { Test##x() : Test(#x) { nextTest = firstTest; firstTest = this; } void Run() override; } __inst_##x; void Test##x::Run()
#define ASSERT(x) do { auto v = (x); if (!v) { throw std::runtime_error("Assertion failed: " #x); } } while(0)

#endif


