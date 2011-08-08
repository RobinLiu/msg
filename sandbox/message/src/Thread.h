#ifndef MESSAGE_THREAD_H
#define MESSAGE_THREAD_H

#include <pthread.h>
#include "Mutex.h"
#include "MutexHolder.h"


extern "C"
{
    typedef void * (*pWrapperfn) (void*);
}

namespace HAUtils {

class Thread {
public:
    Thread(bool joinable = false)
        : m_ThreadId(0), m_Joinable(joinable), m_IsRunning(false), m_StackSize(0) {};
    Thread(Thread const&) {};
    Thread& operator=(Thread const&) {return (*this); };

    virtual ~Thread();

    inline bool isRunning() { return m_IsRunning; }
    inline void setRunning(bool isRunning) { m_IsRunning = isRunning; }
    inline pthread_t getThreadId(){ return m_ThreadId;}
    inline void setIsJoinable(bool joinable) {m_Joinable = joinable; }

    void start() throw(HAUtils::ThreadException);
    void stop() throw(HAUtils::ThreadException);
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

#endif /* MESSAGE_THREAD_H */
