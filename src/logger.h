#ifndef CYNE_LOGGER_H
#define CYNE_LOGGER_H

#include <stdio.h>
#include <time.h>

#define LOG(level, fmt, ...) do {                         \
    time_t _now = time(NULL);                             \
    struct tm *_tm = localtime(&_now);                    \
    char _timestamp[20];                                  \
    if (_tm != NULL)                                      \
        strftime(_timestamp, sizeof(_timestamp),          \
                 "%Y-%m-%d %H:%M:%S", _tm);               \
    else                                                  \
        snprintf(_timestamp, sizeof(_timestamp),          \
                 "unknown");                              \
    fprintf(stderr, "[%s][%s][%s:%d][%s] " fmt "\n",      \
            _timestamp, level, __FILE__, __LINE__,        \
            __func__, ##__VA_ARGS__);                     \
} while (0)

#define INFO(fmt, ...)    LOG("INFO", fmt, ##__VA_ARGS__)
#define WARNING(fmt, ...) LOG("WARNING", fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...)   LOG("ERROR", fmt, ##__VA_ARGS__)

#ifdef NODEBUG
    #define DEBUG(...) ((void)0)
#else
    #define DEBUG(fmt, ...) LOG("DEBUG", fmt, ##__VA_ARGS__)
#endif

#endif