#include "str.h"

#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "allocators.h"
#include "assert.h"
#include "logger.h"

Str strTrim(Str original) {
  int start = 0;
  int end = original.size - 1;

  for (int i = 0; i < original.size; i++) {
    char c = original.str[i];

    if (c != ' ') {
      start = i;
      break;
    }
  }

  for (int i = original.size - 1; i > 0; i--) {
    char c = original.str[i];

    if (c != ' ') {
      end = i;
      break;
    }
  }

  size_t size = end - start + 1;

  return (Str){
      .size = size,
      .str = &original.str[start],
  };
}

// Deprecated, use the one that returns a slice instead
Str strTrimDeprecated(Arena* arena, Str str) {
  int start = 0;
  int end = str.size - 1;
  bool startFound = false;

  for (int i = 0; i < str.size; i++) {
    char c = str.str[i];

    if (!startFound && c != ' ') {
      start = i;
      startFound = true;
    }

    if (c != ' ') {
      end = i;
    }
  }

  size_t size = end - start + 1;
  char* buf = arenaAlloc(arena, size);
  memcpy(buf, str.str + start, size);

  return (Str){
      .size = size,
      .str = buf,
  };
}

Str strJoin4(Arena* arena, Str a, Str b, Str c, Str d) {
  StrArray arr = strArrayInit(arena, 4);

  strArrayPush(&arr, a);
  strArrayPush(&arr, b);
  strArrayPush(&arr, c);
  strArrayPush(&arr, d);

  return strArrayArenaJoin(arena, &arr);
}

Str strJoin3(Arena* arena, Str a, Str b, Str c) {
  StrArray arr = strArrayInit(arena, 3);

  strArrayPush(&arr, a);
  strArrayPush(&arr, b);
  strArrayPush(&arr, c);

  return strArrayArenaJoin(arena, &arr);
}

Str strJoin2(Arena* arena, Str a, Str b) {
  StrArray arr = strArrayInit(arena, 2);

  strArrayPush(&arr, a);
  strArrayPush(&arr, b);

  return strArrayArenaJoin(arena, &arr);
}

// TODO: introduce a joiner param here for flexibility
Str strArrayArenaJoin(Arena* arena, StrArray* array) {
  char* buf = arenaAlloc(arena, strArrayTotalSize(array));
  return strArrayJoin(array, buf);
}

Str strArrayJoin(StrArray* array, char* result) {
  size_t size = 0;

  for (int i = 0; i < array->size; i++) {
    Str str = array->list[i];
    memcpy(result + size, str.str, str.size);
    size += str.size;
  }

  return (Str){
      .size = size,
      .str = result,
  };
}

StrArray strArrayWrap(Str* buffer, size_t size) {
  return (StrArray){
      .size = size,
      .capacity = size,
      .totalSize = -1,
      .list = buffer,
  };
}

void strArrayPush(StrArray* array, Str str) {
  assert(array->size + 1 <= array->capacity);

  array->list[array->size] = str;
  array->size++;

  if (array->totalSize == -1) {
    array->totalSize = 0;
  }

  array->totalSize += str.size;
}

size_t strArrayTotalSize(StrArray* array) {
  if (array->totalSize != -1) {
    return array->totalSize;
  }

  int size = 0;
  for (int i = 0; i < array->size; i++) {
    size += array->list[i].size;
  }

  array->totalSize = size;
  return size;
}

void pushTokenArray(TokenArray* array, Str str, Vec2 pos) {
  assert(array->size + 1 <= array->capacity);

  array->strArray[array->size] = str;
  array->posArray[array->size] = pos;
  array->size++;
}

int strArrayIndexOf(StrArray strArray, Str str) {
  for (int i = 0; i < strArray.size; i++) {
    Str currentStr = strArray.list[i];

    if (strEq(str, currentStr)) {
      return i;
    }
  }

  return -1;
}

StrArray wrapStrArray(Str* stackBuffer, int capacity) {
  return (StrArray){
      .capacity = capacity,
      .size = 0,
      .list = stackBuffer,
  };
}

StrArray strArrayInit(Arena* arena, size_t capacity) {
  return (StrArray){
      .capacity = capacity,
      .size = 0,
      .totalSize = -1,
      .list = arenaAlloc(arena, sizeof(Str) * capacity),
  };
}

TokenArray* createTokenArray(Arena* arena, size_t capacity) {
  TokenArray* result = arenaAlloc(arena, sizeof(TokenArray));

  *result = (TokenArray){
      .capacity = capacity,
      .size = 0,
      .strArray = arenaAlloc(arena, sizeof(Str) * capacity),
      .posArray = arenaAlloc(arena, sizeof(Vec2) * capacity),
  };

  return result;
}

int getNextRightToken(TokenArray* tokens, int position) {
  assert(tokens->size > 0);

  if (tokens->size == 1) {
    Vec2 pos = tokens->posArray[0];

    if (position != pos.end) {
      return 0;
    }
  }

  for (int i = 0; i < tokens->size - 1; i++) {
    Vec2 pos = tokens->posArray[i];
    if (position < pos.end) {
      return i;
    }

    pos = tokens->posArray[i + 1];
    if (position < pos.end) {
      return i + 1;
    }
  }

  return NO_TOKEN_FOUND;
}

int getNextLeftToken(TokenArray* tokens, int position) {
  assert(tokens->size > 0);

  if (tokens->size == 1) {
    Vec2 pos = tokens->posArray[0];

    if (position != pos.start) {
      return 0;
    }
  }

  // TODO: maybe try to iterate like in the right one but check the bounds like in this one to see if the two methods could be merged somehow
  for (int i = tokens->size - 1; i >= 1; i--) {
    Vec2 pos = tokens->posArray[i];
    if (position > pos.start) {
      return i;
    }

    pos = tokens->posArray[i - 1];
    if (position > pos.start) {
      return i - 1;
    }
  }

  return NO_TOKEN_FOUND;
}

TokenArray* strTokenize(Arena* arena, int capacity, Str str, char delimiter, SplitSpec* spec) {
  TokenArray* tokens = createTokenArray(arena, capacity);
  strTokens(tokens, str, delimiter, spec);
  return tokens;
}

// TODO: enhance the token array object with extra arrays that gather more information about tokens such as what kind of stuff is in each one
//  like is it punctuation, or is it numeric or alphanumeric and such, for fast querying
void strTokens(TokenArray* result, Str str, char delimiter, SplitSpec* spec) {
  assert(result != NULL);
  assert(result->capacity > 0);
  assert(str.size > 0);

  bool tokenizePunctuation = false;
  s32 splitLimit = -1;
  char stopChar = -1;

  if (spec != NULL) {
    tokenizePunctuation = spec->tokenizePunctuation;
    splitLimit = spec->splitLimit;
    stopChar = spec->stopAtFirstOccurance;
  }

  if (result->size > 0) {
    // logWarn("Token array is not empty, contents will be overwritten");
    result->size = 0;
  }

  int i = 0;
  int start = 0;
  int end = 0;

  s32 splitCount = 0;

  while (true) {
    if (i == str.size) {
      break;
    }

    char c = str.str[i];

    if (stopChar != -1 && c == stopChar) {
      pushTokenArray(result, sliceStr(str, start, end), (Vec2){start, end});
      break;
    }

    if (tokenizePunctuation && (c == '/' || c == '*' || c == '&' || c == '#' || c == '!' || c == '+' || c == '-' || c == '>' || c == '<' || c == ')' || c == '(' || c == ']' || c == '[' || c == '}' || c == '{' || c == ',' || c == '.' || c == '=' || c == ';' || c == '(' || c == ')')) {
      if (start < end) {
        pushTokenArray(result, sliceStr(str, start, end), (Vec2){start, end - 1});

        start = i + 1;
        end = start;
      }

      pushTokenArray(result, sliceStr(str, i, i + 1), (Vec2){i, i});

      start = i + 1;
      end = start;
    } else if (c == delimiter && (splitLimit == -1 || (splitLimit != -1 && splitCount < splitLimit))) {
      splitCount++;
      // logInfo("asd: %u", splitCount);
      if (start < end) {
        pushTokenArray(result, sliceStr(str, start, end), (Vec2){start, end - 1});
      }

      start = i + 1;
      end = start;
    } else if (i == str.size - 1) {
      pushTokenArray(result, sliceStr(str, start, end + 1), (Vec2){start, end});
    } else {
      end++;
    }

    i++;
  }
}

void strFill(Str str, char c) {
  memset(str.str, c, str.size);
}

int strSeekFirstNonBlank(Str str) {
  size_t size = str.size;

  if (size == 0) {
    return 0;
  }

  for (int i = 0; i < size; i++) {
    if (str.str[i] != ' ') {
      return i;
    }
  }

  return -1;
}

size_t cStringSize(char* str) {
  size_t size = 0;
  while (str[size] != '\0') {
    size++;
  };

  return size--;
}

bool strEqCString(Str str, char* cStr) {
  if (str.str == cStr) {
    return true;
  }

  int i = 0;
  for (; i < str.size; i++) {
    if (str.str[i] != cStr[i]) {
      return false;
    }
  }

  if (cStr[i] != '\0') {
    return false;
  }

  return true;
}

bool strEq(Str t1, Str t2) {
  if (t1.size != t2.size) {
    return false;
  }

  if (t1.str == t2.str) {
    return true;
  }

  int i = 0;
  for (; i < t1.size; i++) {
    if (t1.str[i] != t2.str[i]) {
      return false;
    }
  }

  return true;
}

Str wrapStr(char* cStr) {
  Str str;
  str.size = cStringSize(cStr);
  str.str = cStr;
  return str;
}

Str wrapStrN(char* cStr, size_t size) {
  Str str;
  str.size = size;
  str.str = cStr;
  return str;
}

Str copyStr(Arena* arena, Str original) {
  size_t size = original.size;

  Str str = {
      .size = size,
      .str = arenaAlloc(arena, size),
  };

  memcpy(str.str, original.str, size);

  return str;
}

Str newStr(Arena* arena, char* cStr) {
  size_t size = cStringSize(cStr);

  Str str = {
      .size = size,
      .str = arenaAlloc(arena, size),
  };

  memcpy(str.str, cStr, size);

  return str;
}

// Think about how to solve this;
Str allocStr(Arena* arena, size_t size) {
  Str str = {
      .size = size,
      .str = arenaAlloc(arena, size),
  };

  return str;
}

// NOTE: use '\0' to denote start and end limits.
// 'start' and 'end' cannot be both '\0' at the same time
Str strCopyBetween(Arena* arena, Str str, char start, char end) {
  assert(str.size > 0);
  assert(start != '\0' || end != '\0');

  int a = -1;
  int b = str.size;
  bool foundA = false;
  bool foundB = false;

  if (start == '\0') {
    start = true;
  }

  if (end == '\0') {
    end = true;
  }

  for (int i = 0; i < str.size; i++) {
    char c = str.str[i];

    if (!foundA && c == start) {
      a = i;
      foundA = true;
    }

    if (!foundB && c == end) {
      b = i;
      foundB = true;
    }

    if (foundA && foundB) {
      break;
    }
  }

  if ((b - 1 <= a) || b == 0 || a == str.size - 2) {
    return (Str){
        .size = 0,
        .str = NULL,
    };
  }

  size_t size = b - a - 1;
  char* buf = arenaAlloc(arena, size);
  memcpy(buf, str.str + a + 1, size);

  return (Str){
      .size = size,
      .str = buf,
  };
}

bool isBlank(Str str) {
  return str.size == 0 || str.str == NULL;
}

int charIndex(Str src, char c, int startIndex) {
  assert(startIndex < src.size);

  for (int i = startIndex; i < src.size; i++) {
    if (src.str[i] == c) {
      return i;
    }
  }

  return -1;
}

Str sliceStr(Str src, int a, int b) {
  assert(b >= a);

  int size = b - a;
  if (size == 0) {
    size = 1;
  }

  return (Str){
      .size = size,
      .str = &src.str[a],
  };
}

void toCString(Str str, char* cStr) {
  memcpy(cStr, str.str, str.size);
  cStr[str.size] = '\0';
}
