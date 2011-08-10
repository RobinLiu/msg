#include "thread.h"
#include <sys/time.h>
#include <time.h>

#ifdef USING_POSIX_THREAD
#define THREAD_BLOCK_TIME  60 /*60s*/

void create_thread(thread_id_t* thread_id,
                    thread_attr_t* thread_attr,
                    ThreadFunc func,
                    void* arg) {

  CHECK(NULL != thread_id && NULL != func);
  int32 ret = 0;
  ret =  pthread_create(thread_id, thread_attr, func, arg);
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
  pthread_cond_timedwait(cond, mutex, &tv);
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
