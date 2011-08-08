/*
 * TimerManager.h
 *
 *  Created on: 2011-6-22
 *      Author: Elric
 */

#ifndef TIMERMANAGER_H_
#define TIMERMANAGER_H_
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include "TimerException.h"
#include "Mutex.h"
#include "HAThread.h"
#include "TimeValue.h"
#include "TimeoutHandler.h"
#include "TimerHeap.h"

namespace TimeLib {

static HAUtils::Mutex managerMutex;

class TimerManager: public HAUtils::HAThread {
public:
    TimerManager();
    virtual ~TimerManager();
    static TimerManager* getInstance(unsigned int heapSize = 500,
                                     unsigned int allocationThreshold = 2,
                                     bool preallocate = true) throw
                                     (InitializationFailure, SystemException);

    int registerTimeout(int delayinusec,
                        TimeoutHandler* th,
                        bool ignoreState = false)throw
                        (RegistrationFailure, SystemException);

    int registerTimeout(struct timeval& tv,
                        TimeoutHandler* th,
                        bool ignoreState = false)throw
                        (RegistrationFailure, SystemException);

    void cancelTimeout(int timerId, TimeValue* timevalue = 0)
        throw (CancellationFailure, TimeoutHandlerRunning);

    void cancelTimeout(const TimeoutHandler * const th)
        throw( CancellationFailure, TimeoutHandlerRunning );



    TimeValue triggeringTime(int timerId);

    unsigned int pendingTimers (void);

    unsigned int resetTimers (void);

    void terminate (void) throw (TerminationFailure, SystemException);

    TimeValue getMonotonicTime();


private:
    TimerManager(unsigned int heapSize,
                 unsigned int allocationThreshold,
                 bool preallocate);
    TimerManager(const TimerManager&);
    TimerManager& operator=(const TimerManager&);


    virtual void run(void);
    void waitUntilActive(void);
    int handleTimeout(void);
    void waitForTimeoutEvents(void);
    int dispatchTimeoutEvents(void);
    void select(int width, fd_set* rfds, fd_set* wfds, fd_set* efds,
            const TimeValue* timeout);
    void pipe (void);
    void fdWrite (void);
    void fdRead (void);

    static const char* nullChar;
    static TimerManager* instance;
    enum TimerState {
        TERMINATE = 0, ACTIVE, INACTIVE
    };
    TimerState state;
    TimerHeap* timerQueue;
    int fd[2];
};

}

#endif /* TIMERMANAGER_H_ */
