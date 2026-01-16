
#include <math.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "../lib/allocators.h"
#include "../lib/logger.h"
#include "../lib/str.h"
#include "cbash.h"

extern struct termios origTermios;

typedef enum Action {
  ACTION_INIT,
  ACTION_WORK,
  ACTION_GOTO_FILE,
  ACTION_GOTO_SEARCH,
  ACTION_SWITCH_ENV,
  NO_ACTION,
} Action;

typedef struct Params {
  Action action;
  Str p1;
} Params;

typedef struct App {
  Arena* masterArena;
} App;

void parseParams(Params* p, int argc, char* argv[]);
void disableRawMode(struct termios* origTermios);
void enableRawMode(struct termios* origTermios);
