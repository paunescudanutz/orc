
#include "cbash.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static char** buildArgv(const char* program, va_list args) {
  size_t count = 1;
  va_list copy;
  va_copy(copy, args);

  while (va_arg(copy, const char*)) {
    count++;
  }

  va_end(copy);

  char** argv = calloc(count + 1, sizeof(char*));
  argv[0] = strdup(program);

  for (size_t i = 1; i <= count; i++) {
    const char* arg = va_arg(args, const char*);
    if (!arg) {
      break;
    }

    argv[i] = strdup(arg);
  }

  return argv;
}

Command cmd(const char* program, ...) {
  Command cmd = {0};

  va_list args;
  va_start(args, program);
  cmd.argv = buildArgv(program, args);
  va_end(args);

  return cmd;
}

void cmdPipe(Command* src, Command* dst) {
  src->next = dst;
}

static void runChain(Command* cmd, int inputFd) {
  if (!cmd) {
    return;
  }

  int pipeFd[2];
  if (cmd->next) {
    pipe(pipeFd);
  }

  pid_t pid = fork();
  if (pid == 0) {
    if (inputFd != STDIN_FILENO) {
      dup2(inputFd, STDIN_FILENO);
      close(inputFd);
    }

    if (cmd->next) {
      dup2(pipeFd[1], STDOUT_FILENO);
      close(pipeFd[0]);
      close(pipeFd[1]);
    }

    execvp(cmd->argv[0], cmd->argv);
    _exit(1);
  }

  if (inputFd != STDIN_FILENO)
    close(inputFd);

  if (cmd->next) {
    close(pipeFd[1]);
    runChain(cmd->next, pipeFd[0]);
  }

  waitpid(pid, NULL, 0);
}
