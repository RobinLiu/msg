#ifndef BASE_LOCK_H
#define BASE_LOCK_H
#ifdef __cplusplus
extern "C" {
#endif
#include "common/include/build.h"
#include "common/include/basic_types.h"

#ifdef USING_POSIX_THREAD
#include <pthread.h>

typedef pthread_mutex_t lock_t;

//typedef struct {
//  int8              locked_times;
//  pthread_t         holder;
//  pthread_mutex_t   mutex;
//} lock_t;

int32 init_lock(lock_t* lock);

void lock(lock_t* lock);

void unlock(lock_t* lock);

#endif




#ifdef __cplusplus
}
#endif
#endif
