#include "test.h"

Test* Test::firstTest;

int Test::RunAll() {
  Test* t = Test::firstTest;
  Test* c = t;
  int count = 0, current = 0, fails = 0;
  while (c) { c = c->nextTest; count++; }
  while (t) {
    current++;
    printf("\rRunning test %d/%d: %s", current, count, t->name);
    try {
      t->Run();
    } catch (std::exception& e) {
      printf("\nFailed: %s\n", e.what());
      fails++;
    }
    t = t->nextTest;
  }

  if (fails) {
    printf("\r%d/%d failed                                                                       \n", fails, count);
  } else {
    printf("\rAll tests successful!                                                                \n");
  }
  return (fails > 0);
}

int main() {
  return Test::RunAll();
}

