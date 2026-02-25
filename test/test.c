
#include "test.h"

#define ASSERTS_ENABLED

#include "../lib/assert.h"
#include "../lib/logger.h"
#include "../lib/str.h"

void splitLimitTest();
void wrapString();
void strTrimTest();
void stopAtCharTest();

void runTests() {
  logSeparator("Running Tests Start");

  TEST(splitLimitTest);
  TEST(wrapString);
  TEST(strTrimTest);
  TEST(stopAtCharTest);

  logSeparator("Running Tests Ended");
}

void wrapString() {
  Str dummy = S("abcd   \n");
  Str test = wrapStrN(dummy.str, dummy.size - 1);
  assert(strEq(test, S("abcd   ")));
}

void strTrimTest() {
  Str s = S("  foo()     ");
  assert(s.size == 12);
  Str res = strTrim(s);

  assert(strEq(res, S("foo()")));
  res = strTrim(res);
  assert(strEq(res, S("foo()")));
}

void splitLimitTest() {
  Str s = S("/src/main.c:23:45: foo(':')");
  Arena* arena = arenaCreate(KB(1));

  SplitSpec spec = {.splitLimit = 3, .tokenizePunctuation = false};
  TokenArray* result = strTokenize(arena, 4, s, ':', &spec);

  assert(strEq(result->strArray[0], S("/src/main.c")));
  assert(strEq(result->strArray[1], S("23")));
  assert(strEq(result->strArray[2], S("45")));
  assert(strEq(result->strArray[3], S(" foo(':')")));

  arenaFree(arena);
}
void stopAtCharTest() {
  Str s = S("/src/main.c:23:45: foo()\n____");
  Arena* arena = arenaCreate(KB(1));

  SplitSpec spec = {.splitLimit = -1, .tokenizePunctuation = false, .stopAtFirstOccurance = '\n'};
  TokenArray* result = strTokenize(arena, 4, s, ':', &spec);

  logStr(result->strArray[3], "actual: ");
  assert(strEq(result->strArray[3], S(" foo()")));

  arenaFree(arena);
}
