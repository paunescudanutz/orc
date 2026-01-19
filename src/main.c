#include "main.h"

#include <asm-generic/errno.h>
#include <stdio.h>
#include <unistd.h>

#include "../test/test.h"
#include "cbash.h"

FILE* logFile;
struct termios origTermios;

// #define RUN_TESTS

#define LIST_WINDOWS S("tmux list-windows -F '#{window_name}'")
#define LIST_SESSIONS S("tmux list-sessions")

#define MAX_OUTPUT 8192
#define BUFFER_NAME S("FZF_SELECTION")

Str CWD(Arena* arena) {
  char buf[256] = {0};

  getcwd(buf, 256);

  return newStr(arena, buf);
}

Str getProjectName(Arena* arena) {
  Str cwd = CWD(arena);

  TokenArray tokens = createTokenArray(arena, 10);
  strTokens(&tokens, cwd, '/', false);
  return tokens.strArray[tokens.size - 1];
}

void initApp(App* app) {
  initLogger();
  logInfo("Started:");

#ifdef RUN_TESTS
  runTests();
#endif

  Arena* arena = arenaCreate(MB(1));

  *app = (App){
      .masterArena = arena,
      .projectName = getProjectName(arena),
  };
}

void releaseResources(App* app) {
  arenaFree(app->masterArena);
  fclose(logFile);
}

Str tmuxNewWindow(Arena* arena, Str name, Str program) {
  StrArray a = strArrayInit(arena, 256);

  strArrayPush(&a, S("tmux new-window -n "));
  strArrayPush(&a, S("\""));
  strArrayPush(&a, name);
  strArrayPush(&a, S("\" \""));
  strArrayPush(&a, program);
  strArrayPush(&a, S("\""));

  return strArrayArenaJoin(arena, &a);
}

Str openHelix(Arena* arena, Str file, Str line) {
  StrArray a = strArrayInit(arena, 256);

  strArrayPush(&a, S("helix "));
  strArrayPush(&a, S("+"));
  strArrayPush(&a, line);
  strArrayPush(&a, S(" \""));
  strArrayPush(&a, file);
  strArrayPush(&a, S("\""));

  return strArrayArenaJoin(arena, &a);
}

int cmdRun(Str str) {
  toStackStr(str, command);

  int ret = system(command);
  if (ret == -1) {
    perror("system");
    return -1;
  }

  logStr(str, "Command Run: ");

  return WEXITSTATUS(ret);
}

Str cmdOut(Arena* arena, Str cmd, size_t outBytes) {
  toStackStr(cmd, cStr);

  FILE* fp = popen(cStr, "r");

  if (!fp) {
    return (Str){0};
  }

  char* buf = arenaAlloc(arena, outBytes);

  int size = fread(buf, 1, outBytes, fp);
  pclose(fp);

  return (Str){
      .size = outBytes,
      .str = buf,
  };
}

Str tmuxReadBuffer(Arena* arena, Str bufferName) {
  char* outBuffer = arenaAlloc(arena, MAX_OUTPUT);
  StrArray a = strArrayInit(arena, 256);

  Str cmdStr = strJoin3(arena, S("tmux show-buffer -b "), bufferName, S(" 2>/dev/null"));

  return cmdOut(arena, cmdStr, MAX_OUTPUT);
}

void tmuxDeleteBuffer(Arena* arena, Str bufferName) {
  Str cmd = strJoin2(arena, S("tmux delete-buffer -b "), bufferName);
  cmdRun(cmd);
}

void tmuxSelector(Arena* arena, Str source, Str tmuxBuffer) {
  StrArray a = strArrayInit(arena, 256);

  strArrayPush(&a, S("tmux popup -B -w 70% -h 70% -E "));
  strArrayPush(&a, S("\""));
  strArrayPush(&a, source);
  strArrayPush(&a, S(" | "));
  strArrayPush(&a, S("fzf -e --border --reverse --ansi | "));
  strArrayPush(&a, S("tmux load-buffer -b "));
  strArrayPush(&a, tmuxBuffer);
  strArrayPush(&a, S(" -"));
  strArrayPush(&a, S("\""));

  Str cmd = strArrayArenaJoin(arena, &a);
  cmdRun(cmd);
}

StrArray tmuxList(Arena* arena, Str cmd) {
  Str raw = cmdOut(arena, cmd, MAX_OUTPUT);

  TokenArray array = createTokenArray(arena, 30);
  strTokens(&array, raw, '\n', false);
  return strArrayWrap(array.strArray, array.size);
}

void gotoFile(Arena* arena) {
  Str tokenSource = S("find ./ -type f ");

  tmuxSelector(arena, tokenSource, BUFFER_NAME);
  Str path = tmuxReadBuffer(arena, BUFFER_NAME);

  TokenArray tokens = createTokenArray(arena, 10);
  strTokens(&tokens, path, '\n', false);
  path = tokens.strArray[0];

  StrArray activeWindows = tmuxList(arena, LIST_WINDOWS);

  if (strArrayIndexOf(activeWindows, path) == -1) {
    Str helixCmd = openHelix(arena, path, S("0"));
    Str newWindowCmd = tmuxNewWindow(arena, path, helixCmd);
    cmdRun(newWindowCmd);
  }

  tmuxDeleteBuffer(arena, BUFFER_NAME);
}

void tmuxSwitchWindow(Arena* arena, Str session, Str windowId) {
  cmdRun(strJoin4(arena, S("tmux select-window -t "), session, S(":"), windowId));
}

void editorGotoLine(Arena* arena, Str session, Str id, Str line) {
  StrArray a = strArrayInit(arena, 256);

  strArrayPush(&a, S("tmux send-keys -t "));
  strArrayPush(&a, session);
  strArrayPush(&a, S(":"));
  strArrayPush(&a, id);
  strArrayPush(&a, S(" :goto Space "));
  strArrayPush(&a, line);
  strArrayPush(&a, S(" Enter"));

  Str cmd = strArrayArenaJoin(arena, &a);
  cmdRun(cmd);
}

void gotoSearch(App* app) {
  logSeparator("GOTO SEARCH");
  Arena* arena = app->masterArena;

  Str tokenSource = S("rg --line-number --color=always '' . ");

  tmuxSelector(arena, tokenSource, BUFFER_NAME);
  Str selection = tmuxReadBuffer(arena, BUFFER_NAME);

  TokenArray selectionTokens = createTokenArray(arena, 10);
  strTokens(&selectionTokens, selection, ':', false);
  Str path = selectionTokens.strArray[0];
  Str line = selectionTokens.strArray[1];

  logStr(path, "Searching for: ");
  Str listWindows = strJoin3(arena, S("tmux list-windows -t "), app->projectName, S(" -F '#{window_id} #{window_name}'"));
  StrArray openWindows = tmuxList(arena, listWindows);

  TokenArray windowTokens = createTokenArray(arena, 2);
  bool windowExists = false;

  for (int i = 0; i < openWindows.size; i++) {
    windowTokens.size = 0;
    strTokens(&windowTokens, openWindows.list[i], ' ', false);
    Str id = windowTokens.strArray[0];
    Str name = windowTokens.strArray[1];

    if (strEq(name, path)) {
      StrArray a = strArrayInit(arena, 4);

      tmuxSwitchWindow(arena, app->projectName, id);
      editorGotoLine(arena, app->projectName, id, line);

      windowExists = true;
      break;
    }
  }

  if (!windowExists) {
    Str helixCmd = openHelix(arena, path, line);
    Str newWindowCmd = tmuxNewWindow(arena, path, helixCmd);
    cmdRun(newWindowCmd);
  }

  tmuxDeleteBuffer(arena, BUFFER_NAME);
}

void tmuxSwitchSessions(Arena* arena, Str sessionName) {
  Str cmd = strJoin2(arena, S("tmux switch-client -t"), sessionName);

  cmdRun(cmd);
}

void initEnv(Arena* arena, Str filePath) {
  StrArray a = strArrayInit(arena, 256);

  strArrayPush(&a, S("tmux new-session "));
  strArrayPush(&a, S(" -s $(basename \"$PWD\")"));
  strArrayPush(&a, S(" -n \""));
  strArrayPush(&a, filePath);
  strArrayPush(&a, S("\" \" "));
  strArrayPush(&a, S("helix "));
  strArrayPush(&a, filePath);
  strArrayPush(&a, S("\""));

  Str cmd = strArrayArenaJoin(arena, &a);
  cmdRun(cmd);
}

int main(int argc, char* argv[]) {
  App app = {0};
  initApp(&app);
  Params p = {0};
  parseParams(&p, argc, argv);

  Arena* arena = app.masterArena;

  switch (p.action) {
    case ACTION_INIT:
      initEnv(arena, p.p1);
      break;
    case ACTION_SWITCH_ENV:
      tmuxSwitchSessions(arena, p.p1);
      break;
    case ACTION_GOTO_FILE:
      gotoFile(arena);
      break;
    case ACTION_GOTO_SEARCH:
      gotoSearch(&app);
      break;
    default:
      break;
  }

  logInfo("Stopped!");

  releaseResources(&app);
  return 0;
}
