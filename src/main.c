
#include "main.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../test/test.h"

FILE* logFile;
struct termios origTermios;

#define RUN_TESTS

#define LIST_WINDOWS S("tmux list-windows -F '#{window_name}'")
#define LIST_SESSIONS S("tmux list-sessions")

#define MAX_OUTPUT 8192
#define BUFFER_NAME S("FZF_SELECTION")

SplitSpec PUNCTUATION_SPLIT = {.tokenizePunctuation = true, .splitLimit = -1, .stopAtFirstOccurance = -1};
SplitSpec VIM_PATH_SPLIT = {.tokenizePunctuation = false, .splitLimit = 3, .stopAtFirstOccurance = '\n'};

Str CWD(Arena* arena) {
  char buf[256] = {0};

  getcwd(buf, 256);

  return newStr(arena, buf);
}

Str getProjectName(Arena* arena) {
  Str cwd = CWD(arena);

  TokenArray* tokens = createTokenArray(arena, 10);
  strTokens(tokens, cwd, '/', NULL);
  return tokens->strArray[tokens->size - 1];
}

void initApp(App* app) {
  initLogger();
  logSeparator("Started:");

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
  StrArray a = strArrayInit(arena, 6);

  strArrayPush(&a, S("tmux new-window -n "));
  strArrayPush(&a, S("\""));
  strArrayPush(&a, name);
  strArrayPush(&a, S("\" \""));
  strArrayPush(&a, program);
  strArrayPush(&a, S("\""));

  return strArrayArenaJoin(arena, &a);
}

Str openHelix(Arena* arena, Str file, Str line) {
  StrArray a = strArrayInit(arena, 6);

  strArrayPush(&a, S("helix "));
  strArrayPush(&a, S("+"));
  strArrayPush(&a, line);
  strArrayPush(&a, S(" \""));
  strArrayPush(&a, file);
  strArrayPush(&a, S("\""));

  return strArrayArenaJoin(arena, &a);
}

void cmdRunAsync(Str cmd) {
  pid_t pid = fork();

  if (pid == 0) {
    toStackStr(cmd, command);
    execlp("sh", "sh", "-c", cmd, (char*)NULL);
    exit(1);
  }
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
  logStr(cmd, "Executing pipe read: ");

  FILE* fp = popen(cStr, "r");
  if (!fp) {
    return (Str){0};
  }

  char* buf = arenaAlloc(arena, outBytes);
  size_t total = 0;

  while (total < outBytes) {
    size_t n = fread(buf + total, 1, outBytes - total, fp);

    if (n > 0) {
      total += n;
      continue;
    }

    if (feof(fp)) {
      break;
    }

    if (ferror(fp)) {
      logError("fread error");
      break;
    }
  }

  pclose(fp);

  logDebug("total size: %d", total);
  return (Str){
      .size = total,
      .str = buf,
  };
}

// TODO: rename to something more specific for goto-definition

Str tmuxReadBuffer(Arena* arena, Str bufferName) {
  char* outBuffer = arenaAlloc(arena, MAX_OUTPUT);

  Str cmdStr = strJoin3(arena, S("tmux show-buffer -b "), bufferName, S(" 2>/dev/null"));

  return cmdOut(arena, cmdStr, MAX_OUTPUT);
}

void tmuxDeleteBuffer(Arena* arena, Str bufferName) {
  Str cmd = strJoin2(arena, S("tmux delete-buffer -b "), bufferName);
  cmdRun(cmd);
}

void tmuxSelector(Arena* arena, Str source, Str tmuxBuffer) {
  StrArray a = strArrayInit(arena, 9);

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

  TokenArray* array = createTokenArray(arena, 30);
  strTokens(array, raw, '\n', NULL);
  return strArrayWrap(array->strArray, array->size);
}

void tmuxSwitchWindow(Arena* arena, Str session, Str windowId) {
  cmdRun(strJoin4(arena, S("tmux select-window -t "), session, S(":"), windowId));
}

void editorGotoLine(Arena* arena, Str session, Str id, Str line, Str col) {
  StrArray a = strArrayInit(arena, 9);

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

void switchWindows(App* app, Str path, Str line, Str col) {
  Arena* arena = app->masterArena;

  Str listWindows = strJoin3(arena, S("tmux list-windows -t "), app->projectName, S(" -F '#{window_id} #{window_name}'"));
  StrArray openWindows = tmuxList(arena, listWindows);

  TokenArray* windowTokens = createTokenArray(arena, 2);
  bool windowExists = false;

  for (int i = 0; i < openWindows.size; i++) {
    windowTokens->size = 0;
    strTokens(windowTokens, openWindows.list[i], ' ', NULL);
    Str id = windowTokens->strArray[0];
    Str name = windowTokens->strArray[1];

    if (strEq(name, path)) {
      StrArray a = strArrayInit(arena, 4);

      tmuxSwitchWindow(arena, app->projectName, id);
      editorGotoLine(arena, app->projectName, id, line, col);

      windowExists = true;
      break;
    }
  }

  if (!windowExists) {
    Str helixCmd = openHelix(arena, path, line);
    Str newWindowCmd = tmuxNewWindow(arena, path, helixCmd);
    cmdRun(newWindowCmd);
  }
}

bool isFunctionDefinition(TokenArray* tokens) {
  size_t n = tokens->size - 1;
  Str* strings = tokens->strArray;

  if (strEq(strings[n - 1], S(")")) && strEq(strings[n], S("{"))) {
    logDebug("Function definition found");
    return true;
  }

  return false;
}

bool isMacroDefinition(TokenArray* tokens, Str token) {
  Str* s = tokens->strArray;

  if (strEq(s[0], S("#")) && strEq(s[1], S("define")) && strEq(s[2], token)) {
    logDebug("Macro definition found");
    return true;
  }

  return false;
}

void gotoDefinition(App* app, Str token) {
  Arena* arena = app->masterArena;

  // TODO: not sure if the dot (.) is what i want here, maybe check later
  Str cmd = strJoin3(arena, S("rg -w "), token, S(" --vimgrep ."));
  toStackStr(cmd, cStr);
  logStr(cmd, "Executing goto-definition pipe read: ");

  FILE* fp = popen(cStr, "r");

  if (!fp) {
    perror("popen");
    return;
  }

  // TODO: make sure line is no longer than 1024 otherwise it cannot be parsed
  int n = 1024;
  char buf[n];

  TokenArray* tokens = createTokenArray(arena, 4);
  TokenArray* splitSample = createTokenArray(arena, 400);

  while (fgets(buf, n, fp)) {
    // memset(buf, '')
    Str rawLine = wrapStrN(buf, n);
    strTokens(tokens, rawLine, ':', &VIM_PATH_SPLIT);

    Str path = tokens->strArray[0];
    Str row = tokens->strArray[1];
    Str col = tokens->strArray[2];
    Str sample = strTrim(tokens->strArray[3]);

    strTokens(splitSample, sample, ' ', &PUNCTUATION_SPLIT);
    if (isFunctionDefinition(splitSample) || isMacroDefinition(splitSample, token)) {
      logStr(path, "path: ");
      logStr(row, "row: ");
      logStr(col, "col: ");
      logStr(sample, "sample: ");
      switchWindows(app, path, row, col);
      return;
    }
  }

  pclose(fp);
  return;
}

void gotoFile(App* app) {
  Arena* arena = app->masterArena;
  Str tokenSource = S("find . -type f -printf '%P\n'");

  tmuxSelector(arena, tokenSource, BUFFER_NAME);
  Str path = tmuxReadBuffer(arena, BUFFER_NAME);

  Str windowName = strCopyBetween(arena, path, '\0', '\n');

  switchWindows(app, windowName, S("0"), S("0"));
  tmuxDeleteBuffer(arena, BUFFER_NAME);
}

void gotoSearch(App* app) {
  Arena* arena = app->masterArena;
  Str tokenSource = S("rg --line-number --color=always '' . ");

  tmuxSelector(arena, tokenSource, BUFFER_NAME);
  Str selection = tmuxReadBuffer(arena, BUFFER_NAME);

  TokenArray* selectionTokens = createTokenArray(arena, 10);
  strTokens(selectionTokens, selection, ':', NULL);
  Str path = selectionTokens->strArray[0];
  Str line = selectionTokens->strArray[1];

  Str windowName = strCopyBetween(arena, path, '/', '\0');
  switchWindows(app, windowName, line, S("0"));

  tmuxDeleteBuffer(arena, BUFFER_NAME);
}

void tmuxSwitchSessions(Arena* arena, Str sessionName) {
  Str cmd = strJoin2(arena, S("tmux switch-client -t"), sessionName);

  cmdRun(cmd);
}

void initEnv(Arena* arena, Str filePath) {
  StrArray a = strArrayInit(arena, 8);

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
  cmdRunAsync(S("find . -name '*.[ch]' | entr ctags -R --excmd=number . &"));
}

int main(int argc, char* argv[]) {
  App app = {0};
  initApp(&app);
  Params p = {0};
  parseParams(&p, argc, argv);

  Arena* arena = app.masterArena;
  char buf[256] = {0};

  switch (p.action) {
    case ACTION_INIT:
      initEnv(arena, p.p1);
      break;
    case ACTION_SWITCH_ENV:
      tmuxSwitchSessions(arena, p.p1);
      break;
    case ACTION_GOTO_DEFINITION: {
      fgets(buf, 256, stdin);
      Str token = wrapStr(buf);

      if (token.str == NULL) {
        logError("Error parsing stdin line");
        exit(0);
      }

      token = strTrim(token);
      gotoDefinition(&app, token);
    } break;
    case ACTION_GOTO_FILE:
      gotoFile(&app);
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
