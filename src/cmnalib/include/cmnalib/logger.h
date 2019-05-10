#pragma once

#include <stdio.h>

#include <sys/time.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define POSITION __FILE__ ":" TOSTRING(__LINE__)

#define LOGGER_VERBOSE_DEBUG 2
#define LOGGER_VERBOSE_INFO  1
#define LOGGER_VERBOSE_NONE  0

#define DO_LOG_ENABLE_TIMESTAMP 1

#define LOGGER_ENABLED 1
#define LOGGER_LEVEL LOGGER_VERBOSE_DEBUG
//#define LOGGER_LEVEL LOGGER_VERBOSE_INFO
//#define LOGGER_LEVEL LOGGER_VERBOSE_NONE



#define LOGGER_VERBOSE_ISINFO() (srslte_verbose>=SRSLTE_VERBOSE_INFO)
#define LOGGER_VERBOSE_ISDEBUG() (srslte_verbose>=SRSLTE_VERBOSE_DEBUG)
#define LOGGER_VERBOSE_ISNONE() (srslte_verbose==SRSLTE_VERBOSE_NONE)

#define PRINT_DEBUG srslte_verbose=SRSLTE_VERBOSE_DEBUG
#define PRINT_INFO srslte_verbose=SRSLTE_VERBOSE_INFO
#define PRINT_NONE srslte_verbose=SRSLTE_VERBOSE_NONE

#define DEBUG_STREAM stdout
#define INFO_STREAM stdout
#define WARNING_STREAM stdout
#define ERROR_STREAM stdout

#define FDEBUG(FILESTREAM, _fmt, ...) if (LOGGER_ENABLED && LOGGER_LEVEL >= LOGGER_VERBOSE_DEBUG) \
  do_log(FILESTREAM, "[D]: " _fmt, ##__VA_ARGS__)

#define FINFO(FILESTREAM, _fmt, ...) if (LOGGER_ENABLED && LOGGER_LEVEL >= LOGGER_VERBOSE_INFO) \
  do_log(FILESTREAM, "[I]: " _fmt, ##__VA_ARGS__)

#define FWARNING(FILESTREAM, _fmt, ...) if (LOGGER_ENABLED && LOGGER_LEVEL >= LOGGER_VERBOSE_INFO) \
  do_log(FILESTREAM, "[W]: " _fmt, ##__VA_ARGS__)

#define FERROR(FILESTREAM, _fmt, ...) if (LOGGER_ENABLED && LOGGER_LEVEL >= LOGGER_VERBOSE_INFO) \
  do_log(FILESTREAM, "[E]: " _fmt, ##__VA_ARGS__)

#define DEBUG(_fmt, ...) FDEBUG(DEBUG_STREAM, _fmt, ##__VA_ARGS__)
#define INFO(_fmt, ...) FINFO(INFO_STREAM, _fmt, ##__VA_ARGS__)
#define WARNING(_fmt, ...) FWARNING(WARNING_STREAM, _fmt, ##__VA_ARGS__)
#define ERROR(_fmt, ...) FERROR(ERROR_STREAM, _fmt, ##__VA_ARGS__)

#define TEE_STREAM(CMDA, STREAMA, CMDB, STREAMB, _fmt, ...) CMDA(STREAMA, _fmt, ##__VA_ARGS__);\
                                                            CMDB(STREAMB, _fmt, ##__VA_ARGS__)

extern int enable_logger;

void do_log(FILE* stream, const char * format, ...);
int timeval_subtract (struct timeval *result,
                      struct timeval *x,
                      struct timeval *y);

#ifdef __cplusplus
}
#endif
