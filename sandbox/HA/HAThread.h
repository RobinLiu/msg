/*
 * HAThread.h
 *
 *  Created on: 2011-6-22
 *      Author: Elric
 */

#ifndef HATHREAD_H_
#define HATHREAD_H_

#include <pthread.h>
#include "Mutex.h"
#include "MutexHolder.h"


extern "C"
{
    typedef void * (*pWrapperfn) (void*);
}

namespace HAUtils {

class HAThread {
public:
    HAThread(bool joinable = false)
        : m_ThreadId(0), m_Joinable(joinable), m_IsRunning(false), m_StackSize(0) {};
    HAThread(HAThread const&) {};
    HAThread& operator=(HAThread const&) {return (*this); };

    virtual ~HAThread();

    inline bool isRunning() { return m_IsRunning; }
    inline void setRunning(bool isRunning) { m_IsRunning = isRunning; }
    inline pthread_t getThreadId(){ return m_ThreadId;}
    inline void setIsJoinable(bool joinable) {m_Joinable = joinable; }

    void start() throw(HAUtils::HAThreadException);
    void stop() throw(HAUtils::HAThreadException);
    static void runWrapper(void *);
    virtual void run() = 0;
    static int createThread(pthread_t* thread_id,
                            void*(*start_routine)(void*),
                            void* arg, int stackSize);


private:
    pthread_t m_ThreadId;
    bool m_Joinable;
    bool m_IsRunning;
    int m_StackSize;
};

}

#endif /* HATHREAD_H_ */
