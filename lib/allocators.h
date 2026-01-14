
#ifndef BC12E500_3843_4531_BC40_4004D018539F
#define BC12E500_3843_4531_BC40_4004D018539F

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ASSERTS_ENABLED
#define ASSERTS_ENABLED
#endif

#define KB(ammount) ammount * 1000
#define MB(ammount) ammount* KB(1000)
#define GB(ammount) ammount* MB(1000)

typedef size_t bytes_t;

typedef struct Arena {
  bytes_t capacity;
  bytes_t size;
  void* base;
  void* offset;
} Arena;

typedef struct FreeSlot {
  void* ptr;
} FreeSlot;

typedef struct ObjPool {
  size_t capacity;
  size_t size;
  bytes_t elementSize;
  void* buffer;

  FreeSlot* freeSlots;
  size_t freeSlotsSize;
  size_t freeSlotsRead;
  size_t freeSlotsWrite;
} ObjPool;

Arena* arenaCreate(bytes_t capacity);
void initArena(Arena* arena, bytes_t capacity);
void* arenaAlloc(Arena* arena, bytes_t bytes);
void arenaReset(Arena* arena);
void arenaFree(Arena* arena);

// Obj Pool
void initObjPool(Arena* arena, ObjPool* objPool, bytes_t elementSize, size_t capacity);
void* objPoolAlloc(ObjPool* objPool);
void objPoolFree(ObjPool* objPool, void* ptr);

#endif /* BC12E500_3843_4531_BC40_4004D018539F */
