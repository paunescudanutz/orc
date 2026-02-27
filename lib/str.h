#pragma once

#include <stddef.h>

#include "allocators.h"
#include "assert.h"
#include "base.h"
#include "stdbool.h"

#define DEFAULT_TOKEN_ARRAY_SIZE 100
#define NO_TOKEN_FOUND -1

#define S(s) (Str){.str = s, sizeof(s) - 1}
#define STR_ARRAY(...) {.list = (Str[]){__VA_ARGS__}, .size = sizeof((Str[]){__VA_ARGS__}) / sizeof(Str), .capacity = sizeof((Str[]){__VA_ARGS__}) / sizeof(Str)}

typedef struct Str {
  char* str;
  size_t size;
} Str;

typedef enum CharFilter {
  NONE,
  ALPHANUMERIC,
  NUMBERS,
  LETTERS,
  SYMBOLS,
} CharFilter;

typedef struct MatchCursor {
  Str str;
  u32 cursor;
  bool isMatch;
} MatchCursor;

typedef struct StrArray {
  Str* list;
  size_t size;
  size_t totalSize;
  size_t capacity;
} StrArray;

typedef struct TokenArray {
  Str* strArray;
  Vec2* posArray;
  size_t size;
  size_t capacity;
} TokenArray;

typedef struct SplitSpec {
  bool tokenizePunctuation;  // consider punctuation sings as separate tokens
  // TODO: implement sometime
  bool reversedOrder;         // split from last char to first
  u32 splitLimit;             // will split first N delimiters, put the rest into single slice. Values below 0 are considered infinity
  char stopAtFirstOccurance;  // will stop at first occurance of this character. Values below 0 are considered infinity

} SplitSpec;

#define toStackStr(str, cStr) \
  char cStr[(str).size + 1];  \
  toCString((str), cStr)

Str wrapStr(char* cStr);
Str wrapStrN(char* cStr, size_t size);
Str newStr(Arena* arena, char* cStr);
Str allocStr(Arena* arena, size_t size);
Str copyStr(Arena* arena, Str original);
Str sliceStr(Str src, int a, int b);
void toCString(Str str, char* cStr);

size_t cStringSize(char* cStr);

bool strEq(Str s1, Str s2);
bool strEqCString(Str str, char* cStr);
bool isBlank(Str str);
int charIndex(Str src, char c, int startIndex);
int strSeekFirstNonBlank(Str str);
void strFill(Str str, char c);

StrArray strArrayInit(Arena* arena, size_t capacity);
StrArray strArrayWrap(Str* buffer, size_t size);
int strArrayIndexOf(StrArray strArray, Str str);
Str strArrayJoin(StrArray* array, char* result);
Str strJoin4(Arena* arena, Str a, Str b, Str c, Str d);
Str strJoin3(Arena* arena, Str a, Str b, Str c);
Str strJoin2(Arena* arena, Str a, Str b);
Str strArrayArenaJoin(Arena* arena, StrArray* array);
StrArray wrapStrArray(Str* stackBuffer, int size);
void strArrayPush(StrArray* array, Str str);
size_t strArrayTotalSize(StrArray* array);
Str strTrim(Str str);

Str strCopyBetween(Arena* arena, Str str, char start, char end);

TokenArray* createTokenArray(Arena* arena, size_t capacity);
// void pushTokenArray(TokenArray* array, Token token);
void pushTokenArray(TokenArray* array, Str str, Vec2 pos);
// void strTokens(TokenArray* result, Str str, char delimiter, bool tokenizePunctuatuion);
void strTokens(TokenArray* result, Str str, char delimiter, SplitSpec* spec);
TokenArray* strTokenize(Arena* arena, int capacity, Str str, char delimiter, SplitSpec* spec);
Vec2 getTokenPos(Str token, Str str);
int getNextRightToken(TokenArray* tokens, int index);
int getNextLeftToken(TokenArray* tokens, int index);

bool matchUntil(MatchCursor* cursor, Str match);
bool matchExact(MatchCursor* cursor, Str match);
void matchAny(MatchCursor* cursor, char expected);
