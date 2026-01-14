
#include <math.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "../lib/allocators.h"
#include "../lib/logger.h"
#include "../lib/str.h"
#include "cbash.h"

extern struct termios origTermios;

typedef struct App {
  Arena* masterArena;
} App;

void disableRawMode(struct termios* origTermios);
void enableRawMode(struct termios* origTermios);
