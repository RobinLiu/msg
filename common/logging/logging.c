#include "common/include/logging.h"
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

int8 g_min_log_level = 3;

pthread_mutex_t logging_lock;

bool is_lock_init = FALSE;

void get_time_str(int8* time_buf, int32 buf_len) {
  if(NULL == time_buf || buf_len <= 0) {
    return;
  }

  struct timeval tv;
  struct tm tm;
  time_t curtime = -1;

  int32 ret = gettimeofday(&tv, NULL);
  if (0 == ret) {
    curtime = tv.tv_sec;
  }

  memset(&tm, 0, sizeof(struct tm));
  localtime_r(&curtime, &tm);

  snprintf((void*)time_buf, buf_len, "%04d-%02d-%02d %02d:%02d:%02d:%06d",
           tm.tm_year + 1900,
           tm.tm_mon + 1,
           tm.tm_mday,
           tm.tm_hour,
           tm.tm_min,
           tm.tm_sec,
           (int)tv.tv_usec);

}

int8  GetMinLogLevel(void) {
  return g_min_log_level;
}

void SetMinLogLevel(int8 log_level) {
  g_min_log_level = log_level;
}
