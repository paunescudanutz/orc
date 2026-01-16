#include "main.h"

void parseParams(Params* p, int argc, char* argv[]) {
  p->action = NO_ACTION;

  if (argc > 1) {
    logInfo("parsing input params");
    int i = 1;

    while (i < argc) {
      char* param = argv[i];

      if (strcmp(param, "--help") == 0 || strcmp(param, "-h") == 0) {
        printf("zug zug!");
        exit(0);
      } else if (strcmp(param, "switch-env") == 0) {
        i++;
        p->action = ACTION_SWITCH_ENV;
        p->p1 = wrapStr(argv[i]);
      } else if (strcmp(param, "init") == 0) {
        i++;
        p->action = ACTION_INIT;
      } else if (strcmp(param, "work") == 0) {
        i++;
        p->action = ACTION_WORK;
      } else if (strcmp(param, "file") == 0) {
        i++;
        p->action = ACTION_GOTO_FILE;
      } else if (strcmp(param, "search") == 0) {
        i++;
        p->action = ACTION_GOTO_SEARCH;
      } else {
        i++;
      }
    }
  }
}
