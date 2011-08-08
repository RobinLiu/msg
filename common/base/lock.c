#include "common/include/build.h"
#include "lock.h"
#include "common/include/logging.h"

#ifdef USING_POSIX_THREAD

int32 init_lock(lock_t* lock) {
  CHECK(NULL != lock);
  return pthread_mutex_init(lock, NULL);
}

void lock(lock_t* lock) {
  CHECK(NULL != lock);
//  LOG(INFO,"begin lock");
  int32 ret = pthread_mutex_lock(lock);
//  LOG(INFO,"lock ok");
  CHECK(0 == ret);
}

void unlock(lock_t* lock) {
  CHECK(NULL != lock);
//  LOG(INFO,"begin unlock");
  int32 ret = pthread_mutex_unlock(lock);
//  LOG(INFO,"unlock ok");
  CHECK(0 == ret);
}

#endif
