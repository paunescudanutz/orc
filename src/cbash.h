
#ifndef CMDFLOW_H
#define CMDFLOW_H

#include <stddef.h>

typedef struct Command Command;

struct Command {
  char** argv;
  struct Command* next;
};

Command cmd(const char* program, ...);
void cmdPipe(Command* src, Command* dst);

#endif
