#ifndef BASE_TIMER_H
#define BASE_TIMER_H
#ifdef __cplusplus
extern "C" {
#endif


#include "common/include/common.h"
#include "lock.h"
#include "thread.h"

//typedef struct {
//  uint32        timer_id;
//  uint32        timeout_time;
//  uint32        stored_timeout_time;
//  TIMEOUT_FUNC  timeout_func;
//  lock_t        lock;
//  thread_cond_t cond;
//  void*         data;
//  int8          state;
//  pthread_t     thread_id;
//  bool          renewed;
//  int           fd[2];
//} msg_timer_t;

typedef void (*TIMEOUT_FUNC)(void *);

typedef struct time_val {
  long sec;
  long msec; /*milliseconds*/
} time_val;

typedef struct timer_entry {
  void* user_data;
  TIMEOUT_FUNC cb;
  int timer_id;
  time_val timer_value;
  time_val delay;
} timer_entry;

typedef timer_entry msg_timer_t;

int init_timer_thread();

void init_timer(msg_timer_t* timer,
                 TIMEOUT_FUNC cb,
                 void* data,
                 unsigned int timeout_time);

void stop_timer(msg_timer_t* timer);

int start_timer(msg_timer_t* timer);

int renew_timer(msg_timer_t* timer, uint32 timeout_time);

bool is_timer_started(msg_timer_t* timer);

int timer_will_expire_in(msg_timer_t* timer);
//void setup_timer(msg_timer_t* timer, TIMEOUT_FUNC func, void* data, uint32 timeout_time);
//void start_timer(msg_timer_t* timer);
//void stop_timer(msg_timer_t* timer);

#ifdef __cplusplus
}
#endif
#endif
