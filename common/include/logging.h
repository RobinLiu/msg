#ifndef COMMON_LOGGING_H
#define COMMON_LOGGING_H
#include "build.h"
#include "basic_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#ifdef __cplusplus
extern "C" {
#endif


extern int8 g_min_log_level;
extern pthread_mutex_t logging_lock;
extern bool is_lock_init;

enum LOGSEVERITY { LOG_VERBOSE = 0,
                   LOG_INFO,
                   LOG_WARNING,
                   LOG_ERROR,
                   LOG_FATAL};

int8  GetMinLogLevel(void);
void SetMinLogLevel(int8 log_level);

#define LOG_IS_ON(severity) \
  ((int8)(LOG_ ## severity) >= GetMinLogLevel())

#define ABORT abort()
//#define ABORT exit(-1)

#define PROCESS_FATAL(severity) \
  if((LOG_ ## severity) == LOG_FATAL) { \
    fflush(stderr); \
    fflush(stdout); \
    ABORT;\
  }

void get_time_str(int8* time_buf, int32 buf_len);

//TODO: Lock may needed in multithread environment.
#define LOG(severity, fmt, arg...)  do { \
  if (LOG_IS_ON(severity)) { \
    if(!is_lock_init) { \
      pthread_mutex_init(&logging_lock, NULL); \
      is_lock_init = TRUE; \
    } \
    pthread_mutex_lock(&logging_lock); \
    int8 time_buf[32] = {0}; \
    get_time_str(time_buf, sizeof(time_buf)); \
	  printf("%s:[%s][%s:%d]:"fmt"\n", #severity, time_buf, __FILE__, __LINE__, ## arg);\
    PROCESS_FATAL(severity); \
    pthread_mutex_unlock(&logging_lock); \
    } \
  } while (0)


#define CHECK(condition, err_msg...) do {\
    if (!(condition)) { \
      LOG(FATAL, "Check failed: ("#condition"). "err_msg); \
    } \
  } while (0)


#ifdef __cplusplus
}
#endif

#endif
