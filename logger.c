#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// inspired from
// https://stackoverflow.com/questions/6508461/logging-library-for-c

LOGLEVEL current_log_level = LOG_WARN;

#define CLR_RED "\x1b[31m"
#define CLR_GREEN "\x1b[32m"
#define CLR_YELLOW "\x1b[33m"
#define CLR_RESET "\x1b[0m"

const char *level_to_str(LOGLEVEL level) {
  switch (level) {
  case LOG_INFO:
    return "INFO";
  case LOG_WARN:
    return "WARN";
  case LOG_ERROR:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

void logger(LOGLEVEL level, const char *tag, const char *fmt, ...) {
  // only print if lvl is equal or higher than setting
  if (level < current_log_level) {
    return;
  }

  char msg[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);

  time_t now;
  time(&now);
  char *date = ctime(&now);
  if (date)
    date[strlen(date) - 1] = '\0';

  const char *color;
  const char *level_str;
  switch (level) {
  case LOG_INFO:
    color = CLR_GREEN;
    level_str = "INFO";
    break;
  case LOG_WARN:
    color = CLR_YELLOW;
    level_str = "WARN";
    break;
  case LOG_ERROR:
    color = CLR_RED;
    level_str = "ERROR";
    break;
  default:
    color = CLR_RESET;
    level_str = "LOG";
    break;
  }

  fprintf(stderr, "%s %s [%s] [%s]%s: %s\n", color, date, level_str, tag,
          CLR_RESET, msg);
}

void logger_to_file(const char *tag, const char *message) {
  FILE *f = fopen("wpllm.log", "a"); // "a" for append
  if (f == NULL)
    return;

  time_t now = time(NULL);
  char *date = ctime(&now);
  date[strlen(date) - 1] = '\0';

  fprintf(f, "%s [%s]: %s\n", date, tag, message);
  fclose(f);
}
