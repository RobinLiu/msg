#ifndef MUTEX_H_
#define MUTEX_H_

#include "MutexException.h"
#include <pthread.h>

namespace HAUtils {

class Mutex {
public:
	Mutex();
	virtual ~Mutex();

	void lock()throw(MutexException);
	void unlock() throw(MutexException) ;
	bool trylock() throw(MutexException);
private:
	Mutex(const Mutex&);
	Mutex& operator=(const Mutex&);

	int               valid_;
	pthread_mutex_t   mutex_;
	pthread_t         thread_;
	int               nest_;
};

}

#endif /* MUTEX_H_ */
