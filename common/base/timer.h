#ifndef BASE_TIMER_H
#define BASE_TIMER_H
#ifdef __cplusplus
extern "C" {
#endif


#include "common/include/common.h"
#include "lock.h"
#include "thread.h"
typedef void (*TIMEOUT_FUNC)(void *);

typedef struct {
  uint32        timer_id;
  uint32        timeout_time;
  uint32        stored_timeout_time;
  TIMEOUT_FUNC  timeout_func;
  lock_t        lock;
  thread_cond_t cond;
  void*         data;
  int8          state;
  pthread_t     thread_id;
  bool          renewed;
  int           fd[2];
} msg_timer_t;


void setup_timer(msg_timer_t* timer, TIMEOUT_FUNC func, void* data, uint32 timeout_time);
void start_timer(msg_timer_t* timer);
void stop_timer(msg_timer_t* timer);
void renew_timer(msg_timer_t* timer, uint32 timeout_time);

#ifdef __cplusplus
}
#endif
#endif
