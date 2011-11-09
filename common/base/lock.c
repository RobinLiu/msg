#include "common/include/build.h"
#include "lock.h"
#include "common/include/logging.h"

#ifdef USING_POSIX_THREAD

int32 init_lock(lock_t* lock) {
#if DEBUG_LOCK
  memset(lock, 0, sizeof(lock_t));
  pthread_mutexattr_t mta;
  int rv = pthread_mutexattr_init(&mta);
  CHECK(rv == 0);
  rv = pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_ERRORCHECK);
  CHECK(rv == 0);
  rv = pthread_mutex_init(&lock->locker, &mta);
  CHECK(rv == 0);
  rv = pthread_mutexattr_destroy(&mta);
  CHECK(rv == 0);
  return rv;
#else
  CHECK(NULL != lock);
  return pthread_mutex_init(lock, NULL);
#endif
}

#if DEBUG_LOCK
void _lock(lock_t* locker, char* file, int line) {
//  CHECK(1 != locker->locked)
  LOG(INFO, "Begin lock %p in thread %x at[%s:%d]", &locker->locker, (uint32)pthread_self(), file, line);
  int32 ret = pthread_mutex_lock(&locker->locker);
/*  locker->locked = TRUE;
  locker->locker_thread = pthread_self();
  sprintf(locker->locker_info, "[%s:%d]", file, line);
  CHECK(0 == ret);
  LOG(INFO, "End lock %p in thread %x at[%s:%d]", &locker->locker, (uint32)pthread_self(), file, line);*/

}
void _unlock(lock_t* locker, char* file, int line) {
  int32 ret = pthread_mutex_unlock(&locker->locker);
  CHECK(0 == ret);
  /*LOG(INFO, "Unlock %p in thread %x at[%s:%d]", &locker->locker, (uint32)pthread_self(), file, line);
  locker->locked = FALSE;
  locker->locker_thread = 0;
  memset(locker->locker_info, 0, 128);*/
}
#else
void lock(lock_t* locker) {
  CHECK(NULL != locker);
//  LOG(INFO,"begin lock");
  int32 ret = pthread_mutex_lock(locker);
//  LOG(INFO,"lock ok");
  CHECK(0 == ret);
}

void unlock(lock_t* locker) {
  CHECK(NULL != locker);
//  LOG(INFO,"begin unlock");
  int32 ret = pthread_mutex_unlock(locker);
//  LOG(INFO,"unlock ok");
  CHECK(0 == ret);
}
#endif
#endif
