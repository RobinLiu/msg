#ifndef BASE_LOCK_H
#define BASE_LOCK_H
#ifdef __cplusplus
extern "C" {
#endif
#include "common/include/build.h"
#include "common/include/basic_types.h"

#if USING_POSIX_THREAD
#include <pthread.h>

#if DEBUG_LOCK
typedef struct {
  pthread_t           locker_thread;
  pthread_mutex_t     locker;
  char                locker_info[128];
  bool                locked;
} lock_t;

void _lock(lock_t* locker, char* file, int line);
void _unlock(lock_t* locker, char* file, int line);

#define lock(locker)    _lock(locker, __FILE__, __LINE__)
#define unlock(locker)  _unlock(locker, __FILE__, __LINE__)

#else
typedef pthread_mutex_t lock_t;
void lock(lock_t* locker);

void unlock(lock_t* locker);
#endif


int32 init_lock(lock_t* lock);


#endif


#ifdef __cplusplus
}
#endif
#endif
