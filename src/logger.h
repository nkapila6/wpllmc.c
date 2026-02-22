// 22.02.2026 Nikhil Kapila
// C file for curl fetch utils

#ifndef LOGGER_H
#define LOGGER_H

typedef enum { LOG_INFO, LOG_WARN, LOG_ERROR } LOGLEVEL;

extern LOGLEVEL current_log_level;

void logger(LOGLEVEL level, const char *tag, const char *fmt, ...);

#endif
