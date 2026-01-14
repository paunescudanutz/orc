#pragma once

#define TEST(function)                      \
  {                                         \
    logInfo("Running test: %s", #function); \
    function();                             \
  }

// Call this inside main somewhere
void runTests();
