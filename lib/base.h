#pragma once

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef union Vec3 {
  int array[3];
  struct {
    int r;
    int g;
    int b;
  };
  struct {
    int x;
    int y;
    int z;
  };
} Vec3;

typedef union Vec2 {
  int array[2];
  struct {
    int a;
    int b;
  };
  struct {
    int x;
    int y;
  };
  struct {
    int col;
    int row;
  };
  struct {
    int start;
    int end;
  };
  struct {
    int width;
    int height;
  };
} Vec2;

typedef union Range {
  struct {
    Vec2 start;
    Vec2 end;
  };
  struct {
    Vec2 pos;
    Vec2 size;
  };
  struct {
    int startCol;
    int startRow;
    int endCol;
    int endRow;
  };
  struct {
    int x;
    int y;
    int width;
    int height;
  };
} Range;

typedef Range Box;

typedef struct HSV {
  int h;
  float s;
  float v;
} HSV;
