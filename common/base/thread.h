#ifndef BASE_THREAD_H
#define BASE_THREAD_H
#ifdef __cplusplus
extern "C" {
#endif
#include "common/include/common.h"
#include "lock.h"


#ifdef USING_POSIX_THREAD
#include <pthread.h>

typedef pthread_t thread_id_t;
typedef pthread_attr_t thread_attr_t;
typedef pthread_cond_t  thread_cond_t;

typedef void* (*ThreadFunc)(void*);

void create_thread(thread_id_t* thread_id,
                    thread_attr_t* thread_attr,
                    ThreadFunc func,
                    void* arg);

void thread_cond_init(pthread_cond_t* threshold);

void wait_to_be_notified(thread_cond_t* cond, lock_t* mutex);

void notify_thread(thread_cond_t* cond);

int32 cancel_thread(thread_id_t thread_id);

#endif

#ifdef __cplusplus
}
#endif
#endif
