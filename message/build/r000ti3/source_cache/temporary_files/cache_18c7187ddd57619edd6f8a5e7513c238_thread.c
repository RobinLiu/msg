#include "thread.h"
#include <sys/time.h>
#include <time.h>

#ifdef USING_POSIX_THREAD
#define THREAD_BLOCK_TIME  60 /*60s*/

void create_thread(thread_id_t* thread_id,
                    thread_attr_t* thread_attr,
                    ThreadFunc func,
                    void* arg) {

  (void)thread_attr;
  CHECK(NULL != thread_id && NULL != func);
  pthread_attr_t attr;
  size_t stack_size = 0;
  pthread_attr_init(&attr);
  pthread_attr_getstacksize(&attr, &stack_size);
  LOG(ERROR, "Default stack size is %zd bytes", stack_size);
  stack_size = 16*1024*1024;
  pthread_attr_setstacksize(&attr, stack_size);

  int32 ret = 0;
  ret =  pthread_create(thread_id, /*thread_attr*/&attr, func, arg);
  pthread_attr_getstacksize(&attr, &stack_size);
  LOG(ERROR, "after set stack size is %zd bytes", stack_size);
  pthread_attr_destroy(&attr);
  CHECK(0 == ret);
}


void thread_cond_init(pthread_cond_t* threshold) {
  CHECK(NULL != threshold);
  CHECK(0 == pthread_cond_init(threshold, NULL));
}


void wait_to_be_notified(thread_cond_t* cond, lock_t* mutex) {
  struct timespec tv;
  clock_gettime(CLOCK_REALTIME, &tv);
  tv.tv_sec += THREAD_BLOCK_TIME;
#if DEBUG_LOCK
  int ret = pthread_cond_timedwait(cond, &mutex->locker, &tv);
#else
  int ret = pthread_cond_timedwait(cond, mutex, &tv);
#endif
  if (ETIMEDOUT == ret) {
    LOG(ERROR, "Timeout...");
  }
}

void notify_thread(thread_cond_t* cond) {
  LOG(INFO, "Notify thread...");
  pthread_cond_signal(cond);
}

int32 cancel_thread(thread_id_t thread_id) {
  int ret = pthread_cancel(thread_id);
  if (ret != 0) {
    return ret;
  }
  return  pthread_join(thread_id, NULL);
}
#endif
