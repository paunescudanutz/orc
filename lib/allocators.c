#include "allocators.h"

#include "assert.h"
#include "logger.h"

Arena* arenaCreate(bytes_t capacity) {
  int headerSize = sizeof(Arena);

  Arena* arena = calloc(1, headerSize + capacity);
  if (arena == NULL) {
    logError("Could not allocate new arena");
    exit(0);
  }

  arena->base = arena + 1;
  arena->capacity = capacity;
  arena->offset = arena->base;
  arena->size = 0;

  return arena;
}

void arenaFree(Arena* arena) {
  free(arena);
}

// Avoid using this init. it cannot be cleared with the arenaFree function
// void initArena(Arena* arena, bytes_t capacity) {
//   arena->base = calloc(1, capacity);

//   if (arena->base == NULL) {
//     logError("Could not allocate new buffer");
//     exit(0);
//   }

//   arena->capacity = capacity;
//   arena->offset = arena->base;
//   arena->size = 0;
// }

void* arenaAlloc(Arena* arena, bytes_t newBlockInBytes) {
  if (arena->size + newBlockInBytes > arena->capacity) {
    logError(
        "New allocation of %ld bytes block exceeds capacity of "
        "%ld bytes",
        newBlockInBytes, arena->capacity);
    exit(0);
  }

  void* currentPtr = arena->offset;
  arena->offset += newBlockInBytes;

  arena->size += newBlockInBytes;

  return currentPtr;
}

void arenaReset(Arena* arena) {
  if (arena->size == 0) {
    return;
  }

  memset(arena->base, 0, arena->capacity);
  arena->offset = arena->base;
  arena->size = 0;
}

void initObjPool(Arena* arena, ObjPool* objPool, bytes_t elementSize, size_t capacity) {
  bytes_t bufferCapacity = elementSize * capacity;
  bytes_t headerCapacity = capacity * sizeof(FreeSlot);

  assert(bufferCapacity + headerCapacity <= arena->capacity - arena->size);

  *objPool = (ObjPool){
      .capacity = capacity,
      .size = 0,
      .elementSize = elementSize,
      .buffer = arenaAlloc(arena, bufferCapacity),
      .freeSlots = (FreeSlot*)arenaAlloc(arena, headerCapacity),
      .freeSlotsSize = 0,
      .freeSlotsRead = 0,
      .freeSlotsWrite = 0,
  };
}

void* objPoolAlloc(ObjPool* objPool) {
  void* newPtr = NULL;

  if (objPool->freeSlotsSize != 0) {
    newPtr = objPool->freeSlots[objPool->freeSlotsRead].ptr;

    objPool->freeSlotsRead = (objPool->freeSlotsRead + 1) % objPool->capacity;
    objPool->freeSlotsSize--;
    objPool->size++;
  } else {
    bytes_t elementSize = objPool->elementSize;
    size_t newSize = objPool->size + 1;

    if (newSize * elementSize > objPool->capacity * elementSize) {
      logError("Could not allocate freeList element - Capacity full");
      exit(0);
    }

    newPtr = objPool->buffer + (objPool->size * objPool->elementSize);
    objPool->size = newSize;
  }

  return newPtr;
}

void objPoolFree(ObjPool* objPool, void* ptr) {
  if (objPool->size == 0) {
    return;
  }

  objPool->size--;

  if (objPool->size == 0) {
    objPool->freeSlotsSize = 0;
    return;
  }

  if (objPool->buffer > ptr || (objPool->buffer + (objPool->capacity * objPool->elementSize)) < ptr) {
    logError("Illegal free: ptr not allocated in (this) memory block");
    exit(0);
  }

  objPool->freeSlotsSize++;
  objPool->freeSlots[objPool->freeSlotsWrite] = (FreeSlot){.ptr = ptr};
  objPool->freeSlotsWrite = (objPool->freeSlotsWrite + 1) % objPool->capacity;
}
