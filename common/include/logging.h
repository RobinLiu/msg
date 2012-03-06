#ifndef COMMON_LOGGING_H
#define COMMON_LOGGING_H
#include "build.h"
#include "basic_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FILTER_BUF_SIZE   256

extern int8             g_min_log_level;
extern pthread_mutex_t  logging_lock;
extern bool             is_lock_init;
extern bool             g_filter_log_flag ;
extern char             g_log_filter[FILTER_BUF_SIZE];

enum LOGSEVERITY { LOG_VERBOSE  = 0,
                   LOG_INFO     = 1,
                   LOG_WARNING  = 2,
                   LOG_ERROR    = 3,
                   LOG_IMPORTANT = 3,
                   LOG_FATAL    = 4};

int8 get_min_log_level(void);
void set_min_log_level(int8 log_level);
void set_log_filter(char* log_filter);
void clear_log_filter();

#define LOG_NOT_FILTERED(file_str) \
    (g_filter_log_flag ? (NULL != strstr((char*)file_str, (char*)g_log_filter)) : !g_filter_log_flag)


#define LOG_IS_ON(severity, file_str) \
  (((int8)(LOG_ ## severity) >= get_min_log_level())) && LOG_NOT_FILTERED(file_str)

#define ABORT abort()

#define PROCESS_FATAL(severity) \
  if((LOG_ ## severity) == LOG_FATAL) { \
    fflush(stderr); \
    fflush(stdout); \
    ABORT;\
  }

void get_time_str(int8* time_buf, int32 buf_len);


#define LOG(severity, fmt, arg...)  do { \
  if (LOG_IS_ON(severity, __FILE__)) { \
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
