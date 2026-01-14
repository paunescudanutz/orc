#pragma once

#define LOG_TO_FILE
#define LOG_ENABLED

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef LOG_TO_FILE
extern FILE* logFile;
#define LOG_OUT logFile
#else
#define LOG_OUT stdout
#endif

#ifdef LOG_ENABLED
#define cs_log(level, fmt, ...)                           \
  do {                                                    \
    fprintf(LOG_OUT,                                      \
            "[" level                                     \
            "] : "                                        \
            "%s: %s(): %d: " fmt "\n",                    \
            __FILE__, __func__, __LINE__, ##__VA_ARGS__); \
    fflush(LOG_OUT);                                      \
  } while (0)

#else
#define cs_log(level, fmt, ...) \
  do {                          \
  } while (0)
#endif

#define logInt(variable) cs_log("DEBUG", #variable ": %d", variable)
#define logChar(variable) cs_log("DEBUG", #variable ": %c", variable)
#define logString(variable) cs_log("DEBUG", #variable ": %s", variable)

#define logInfo(fmt, ...) cs_log("INFO", fmt, ##__VA_ARGS__)
#define logWarn(fmt, ...) cs_log("WARN", fmt, ##__VA_ARGS__)
#define logDebug(fmt, ...) cs_log("DEBUG", fmt, ##__VA_ARGS__)
#define logError(fmt, ...) cs_log("ERROR", fmt, ##__VA_ARGS__)
#define logAssert(fmt, ...) cs_log("ASSERT FAILED", fmt, ##__VA_ARGS__)
#define logSeparator(param) cs_log("DIV", "=================[ %s ]===================", param)

// TODO: refactor this to use this trick: https://www.youtube.com/watch?v=oJH8yX1WxFI
#define logStr(str, prefix)        \
  {                                \
    toStackStr(str, cStr);         \
    logInfo("%s%s", prefix, cStr); \
  }

#ifdef LOG_ENABLED
#define initLogger()                  \
  {                                   \
    logFile = fopen("log.txt", "w+"); \
  }

#define destroyLogger() \
  {                     \
    fclose(logFile);    \
  }
#else
#define initLogger() {};
#define destroyLogger() {};
#endif
