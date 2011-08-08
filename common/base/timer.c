#include "timer.h"
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>

uint32 g_timer_index = 0;

#define TIMER_NOT_STARTED  0
#define TIMER_STOPPED      1
#define TIMER_STARTED      2
#define TIMER_EXPIRED      3
#define TIEMR_RENEWING     4

void set_timeout_time(msg_timer_t* timer, uint32 timeout_time) {
  CHECK(NULL != timer);
  lock(&timer->lock);
  timer->timeout_time = timeout_time;
  unlock(&timer->lock);
}


static void* timer_thread(void* arg) {
  msg_timer_t* timer = (msg_timer_t*)arg;

  struct pollfd pfd;
  pfd.fd = timer->fd[0];
  pfd.events = POLLIN;
  pfd.revents = 0;
//  = { timer->fd[0], POLLIN, 0 };
  char flag[1];
  while(1) {
//    LOG(INFO, "in while loop");
    switch (poll(&pfd, 1, (timer->timeout_time ? (int)timer->timeout_time : (int)-1))) {
      //timeout
      case 0:
        LOG(INFO, "Timeout, call timeout callback");
        (*timer->timeout_func)(timer->data);
        break;
      case -1:
        if (errno != EINTR) {
          LOG(ERROR, "Poll error");
        }
        break;
      case 1:
        if (read(timer->fd[0], flag, 1) < 0) {
          LOG(ERROR, "Read from pipe failed");
        } else {
//          LOG(INFO, "read one byte, time is %d", timer->timeout_time);
        }
        break;
    }
  }


//  lock(&timer->lock);
//  while(1) {
//    struct timespec tv;
//    clock_gettime(CLOCK_REALTIME, &tv);
//    tv.tv_sec = timer->timeout_time / 1000;
//    tv.tv_nsec = (timer->timeout_time % 1000)*1000000;
//    int32 ret = pthread_cond_timedwait(&timer->cond, &timer->lock, &tv);
//    if( ret == ETIMEDOUT) {
//      //if timeout, call the timeout handle;
//      if(timer->renewed) {
//        LOG(INFO, "Timer renewed in thread %x.", pthread_self());
//        timer->renewed = FALSE;
//        continue;
//      } else if (TIMER_STARTED == timer->state) {
//        LOG(INFO, "timeout, call the timout handle in thread %x", pthread_self());
//        (*timer->timeout_func)(timer->data);
//      } else {
//        break;
//      }
//    } else if (timer->renewed) {
//      LOG(INFO,"Timer renewed in thread %x", pthread_self());
//      timer->renewed = FALSE;
////      timer->state = TIMER_STARTED;
//      continue;
//    } else {
//      LOG(INFO, "Timer stop");
////      timer->state = TIMER_STOPPED;
//      break;
//    }
//    LOG(INFO, "timer exit");
//  }
//  timer->state = TIMER_NOT_STARTED;
//  unlock(&timer->lock);
  lock(&timer->lock);
  timer->state = TIMER_NOT_STARTED;
  unlock(&timer->lock);
  LOG(INFO, "Timer exit");
  pthread_exit(NULL);
}



void setup_timer(msg_timer_t* timer,
                 TIMEOUT_FUNC func,
                 void* data,
                 uint32 timeout_time) {
  CHECK(NULL != timer && NULL != func && NULL != data && timeout_time > 0);

  set_timeout_time(timer, 0);
  timer->stored_timeout_time = timeout_time;
  init_lock(&timer->lock);
  timer->timer_id = g_timer_index++;
  timer->timeout_func = func;
  timer->data = data;
  timer->state = TIMER_NOT_STARTED;
  timer->renewed = FALSE;
  CHECK(0 == pipe(timer->fd));

  LOG(INFO, "pipe[0]:%d, pipe[1]:%d", timer->fd[0], timer->fd[1]);
//  int flags = fcntl (timer->fd[0], F_GETFL );
//  flags = flags | O_NDELAY;
//  fcntl(timer->fd[0], F_SETFL, (int32)flags);
//  fcntl(timer->fd[0],F_SETFD,FD_CLOEXEC);
//  fcntl(timer->fd[1],F_SETFD,FD_CLOEXEC);

  int ret = pthread_create(&timer->thread_id, NULL, &timer_thread, (void*)timer);
//  created_times++;
//  LOG(ERROR, "Created times is %d", created_times);
  if (0 != ret) {
    perror("create thread failed");
  }
  CHECK(0 == ret, "Create timer failed");
}





uint32 created_times = 0;

void start_timer(msg_timer_t* timer) {

  lock(&timer->lock);
  if (timer->state != TIMER_NOT_STARTED) {
    LOG(WARNING, "Timer has already been started.");
    unlock(&timer->lock);
    return;
  }
  LOG(INFO, "start new timer");
  timer->state = TIMER_STARTED;
  unlock(&timer->lock);
  set_timeout_time(timer, timer->stored_timeout_time);
  char msg[] = "1";
//  CHECK(1 == write(timer->fd[0], (void*)msg, 1));
  LOG(INFO, "fd[0] is %d", timer->fd[1]);
  int ret = write(timer->fd[1], (void*)msg, 1);
  if(ret != 1) {
    perror("write error");
    exit(-1);
  }


}

void stop_timer(msg_timer_t* timer) {
  set_timeout_time(timer, 0);
  CHECK(1 == write(timer->fd[1], "0", 1));
  LOG(INFO, "stop timer....");

//  lock(&timer->lock);
//  if (TIMER_NOT_STARTED != timer->state) {
//    timer->state = TIMER_STOPPED;
//    pthread_cond_signal(&timer->cond);
////    int ret = pthread_cancel(timer->thread_id);
//    LOG(INFO,"stop timer....");
////    LOG(INFO,"stop timer...., result is %d", ret);
//  } else {
//    LOG(INFO,"Timer already stopped.");
//  }
//  unlock(&timer->lock);
}

void renew_timer(msg_timer_t* timer, uint32 timeout_time) {
  LOG(INFO, "renew timer....");
  CHECK(0 != timeout_time);
  lock(&timer->lock);
  uint8 timer_state = timer->state;
  unlock(&timer->lock);
  if (TIMER_NOT_STARTED == timer_state) {
    start_timer(timer);
  } else {
    set_timeout_time(timer, timeout_time);
    CHECK(1 == write(timer->fd[1], "1", 1));
  }

//  if (pthread_equal(timer->thread_id, pthread_self())) {
////    set_timeout_time(timer, timeout_time);
//    LOG(INFO, "Renew in timeout function");
//    CHECK(1 == write(timer->fd[1], "1", 1));
//    return;
//  }
//  lock(&timer->lock);
//  if (TIMER_NOT_STARTED == timer->state) {
//    unlock(&timer->lock);
//    start_timer(timer);
//    return;
//  }
//  unlock(&timer->lock);

//  lock(&timer->lock)
//  //Renew timer in the timeout function.
//  if (pthread_equal(timer->thread_id, pthread_self())) {
//    LOG(INFO, "Renew timer in the timeout function");
////    if (TIEMR_RENEWING == timer->state ||
////        TIMER_STARTED == timer->state) {
////    timer->state = TIEMR_RENEWING;
//    timer->renewed = TRUE;
////    } else {
////          LOG(INFO, "Timer is not started, start it first");
////    }
//    return;
//  }
//
//  //renew timer in other thread;
//  lock(&timer->lock);
//  if (TIMER_NOT_STARTED != timer->state) {
//    timer->renewed = TRUE;
//    pthread_cond_signal(&timer->cond);
//    LOG(INFO, "Renew timer...");
//    unlock(&timer->lock);
//  } else {
//    unlock(&timer->lock);
//    start_timer(timer);
//    LOG(INFO, "Timer is not started, start it first");
//  }
}
