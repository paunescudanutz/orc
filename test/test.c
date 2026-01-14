
#include "test.h"

#define ASSERTS_ENABLED

#include "../lib/assert.h"
#include "../lib/logger.h"

void dummyTest();
void runTests() {
  logSeparator("Running Tests Start");

  TEST(dummyTest);

  logSeparator("Running Tests Ended");
}

void dummyTest() {
  assert(1 == 1);
}
