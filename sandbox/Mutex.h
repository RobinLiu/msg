/*
 * Mutex.h
 *
 *  Created on: 2011-6-21
 *      Author: Elric
 */

#ifndef MUTEX_H_
#define MUTEX_H_

#include <pthread.h>

class Mutex {
public:
	Mutex();
	virtual ~Mutex();

	void lock();
	void unlock();
	bool trylock();
private:
	Mutex(const Mutex&);
	Mutex& operator=(const Mutex&);

	int               valid_;
	pthread_mutex_t   mutex_;
	pthread_t         thread_;
	int               nest_;
};


#endif /* MUTEX_H_ */
