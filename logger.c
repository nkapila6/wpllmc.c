#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// inspired from
// https://stackoverflow.com/questions/6508461/logging-library-for-c

LOGLEVEL current_log_level = LOG_WARN;

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

void logger(LOGLEVEL level, const char *tag, const char *msg) {
  // only print if lvl is equal or higher than setting
  if (level < current_log_level) {
    return;
  }

  time_t now;
  time(&now);

  char *date = ctime(&now);
  if (date)
    date[strlen(date) - 1] = '\0';

  fprintf(stderr, "%s [%s]: %s\n", date, tag, msg);
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
