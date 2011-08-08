/*
 * TimerManager.cpp
 *
 *  Created on: 2011-6-22
 *      Author: Elric
 */

#include "TimerManager.h"
#include <iostream>
#include <syslog.h>
#include "log.h"
#include <cstdlib>
#include <poll.h>

namespace TimeLib {

TimerManager* ManagerInstance = 0;
const char* TimerManager::nullChar = "";

TimerManager* TimerManager::getInstance(unsigned int heapSize,
        unsigned int allocationThreshold, bool preallocate)
        throw (InitializationFailure, SystemException) {

    if (((int) allocationThreshold <= 1) || ((int) (heapSize
            - allocationThreshold) <= 1)) {
        throw InitializationFailure("invalid parameters", __FILE__, __LINE__);
    }

    if (ManagerInstance == NULL) {
        HAUtils::MutexHolder mh(managerMutex);
        if (ManagerInstance == NULL) {
            ManagerInstance = new TimerManager(heapSize, allocationThreshold,
                    preallocate);
            ManagerInstance->start();
        }

        ManagerInstance->waitUntilActive();
    }

    return ManagerInstance;
}
TimerManager::TimerManager(unsigned int heapSize,
        unsigned int allocationThreshold, bool preallocate) :
    HAUtils::HAThread(true), state(INACTIVE) {
    try {
        // Create a timer heap object.
        timerQueue = new TimerHeap(heapSize, allocationThreshold, preallocate);

        // Create a pair of read/write fds.
        this->pipe();
    } catch (SystemException&) {
        std::cout << "SystemException" << std::endl;
        delete timerQueue;
        throw;
    }
}

void TimerManager::waitUntilActive(void) {
    while (state == INACTIVE) {
        usleep(1000);
    }
    return;
}

void TimerManager::pipe(void) {
    if (::pipe(fd) < 0) {
        throw SystemException("error in creating pipe", __FILE__, __LINE__);
    }
    int flags = ::fcntl(fd[0], F_GETFL);
    flags = flags | O_NDELAY;
    if (::fcntl(fd[0], F_SETFL, (int) flags) < 0) {
        throw SystemException("O_NDELAY error", __FILE__, __LINE__);
    }
    fcntl(fd[0], F_SETFD, FD_CLOEXEC);
    fcntl(fd[1], F_SETFD, FD_CLOEXEC);
}

TimerManager::~TimerManager() {
    // Terminate timer thread if it is still active.
    if (state == ACTIVE) {
        state = TERMINATE;
        fdWrite();
    }

    // Delete the timer heap which was created in the constructor.
    delete timerQueue;

    // Close the fd pair.
    ::close(fd[0]);
    ::close(fd[1]);
}

int TimerManager::registerTimeout(int delay, TimeoutHandler* th,
        bool ignoreState) throw (RegistrationFailure, SystemException) {
    if (delay < 0) {
        LOG(ERROR) << "Negative delay " << delay;
        abort();
    }

    if (state == ACTIVE || ignoreState) {
        int tId = timerQueue->schedule(
                timerQueue->getTimeOfDay() + TimeValue(0, delay), th);

        if (state == ACTIVE) {
            fdWrite();
        }
        return tId;
    } else {
        throw RegistrationFailure("timer thread not active", __FILE__, __LINE__);
    }
}

int TimerManager::registerTimeout(struct timeval& tv, TimeoutHandler* th,
        bool ignoreState) throw (RegistrationFailure, SystemException) {
    if (tv.tv_sec < 0 || tv.tv_usec < 0) {
        LOG(ERROR) << "Negative delay value " << tv.tv_sec << "/" << tv.tv_usec;
        abort();
    }

    if (state == ACTIVE || ignoreState) {
        TimeValue now = timerQueue->getTimeOfDay();
        int t = ((int) now.get_sec() + (int) tv.tv_sec + 1000);
        TimeValue timeout;
        // Timeout too far in the future?
        if (t > 0x7FFFFF00) {
            // Force the timeout to be 1000 secs before max.
            timeout.set((0x7FFFFFFE - 1000), 0);
            LOG(INFO) << "timeout of " << tv.tv_sec << " secs changed to "
                    << timeout.get_sec() << " secs";
        } else {
            timeout = (now + TimeValue(tv));
        }
        int tId = timerQueue->schedule(timeout, th);

        if (state == ACTIVE) {
            fdWrite();
        }
        return tId;
    } else {
        throw RegistrationFailure("timer thread not active", __FILE__, __LINE__);
    }
}

void TimerManager::cancelTimeout(int timerId, TimeValue* timeValue)
        throw (CancellationFailure, TimeoutHandlerRunning) {

    timerQueue->cancel(timerId, timeValue);
}

void TimerManager::cancelTimeout(const TimeoutHandler * const th)
        throw (CancellationFailure, TimeoutHandlerRunning) {
    timerQueue->cancel(th);
}

TimeValue TimerManager::triggeringTime(int timerId) {
    return timerQueue->triggeringTime(timerId);
}

unsigned int TimerManager::pendingTimers(void) {
    return timerQueue->pending();
}

unsigned int TimerManager::resetTimers(void) {
    return timerQueue->reset();
}

void TimerManager::terminate(void) throw (TerminationFailure, SystemException) {
    if (state) {
        state = TERMINATE;
        fdWrite();
        pthread_join(getThreadId(), NULL);
    } else
        throw TerminationFailure("timer thread not running", __FILE__, __LINE__);
}

TimeValue TimerManager::getMonotonicTime() {
    TimeValue tv = timerQueue->getTimeOfDay();
    return tv;
}

void TimerManager::run(void) {
    if (state != ACTIVE) {
        state = ACTIVE;
        try {
            while (state == ACTIVE) {
                handleTimeout();
            }
            state = INACTIVE;
        } catch (...) {
            exit(-1);
        }
    } else {
        std::cerr << " WARNING::"
                << "an attempt has been made to start the timer thread twice"
                << std::endl;
    }

}

int TimerManager::handleTimeout(void) {
    waitForTimeoutEvents();
    return dispatchTimeoutEvents();
}

void TimerManager::waitForTimeoutEvents(void) {
    TimeValue* thisTimeout = timerQueue->calculateTimeout();
    struct pollfd pfd = { fd[0], POLLIN, 0 };
    switch (poll(
            &pfd,
            1,
            thisTimeout ? (thisTimeout->get_sec() * 1000
                    + thisTimeout->get_usec() / 1000) : -1)) {
    case 0:
        break;
    case -1:
        if (errno != EINTR)
            throw SystemException("poll failure", __FILE__, __LINE__);
        break;
    case 1:
        fdRead();
        break;
    }
}

int TimerManager::dispatchTimeoutEvents(void) {
    int numberDispatched = timerQueue->expire();
    return numberDispatched;
}

void TimerManager::select(int width, fd_set* rfds, fd_set* wfds, fd_set* efds,
        const TimeValue* timeout) {

    if (::select(width, (fd_set*) rfds, (fd_set*) wfds, (fd_set*) efds,
            timeout == 0 ? 0 : (timeval*) (const timeval*) timeout) < 0) {
        if (errno == EINTR) {
            return;
        }
        throw SystemException("select failure", __FILE__, __LINE__);
    }
}

void TimerManager::fdWrite(void) {
    if (::write(fd[1], nullChar, 1) < 0)
        throw SystemException("error in writing to fd", __FILE__, __LINE__);
}

void TimerManager::fdRead(void) {
    char flag[1];
    if (::read(fd[0], flag, sizeof(flag)) < 0)
        throw SystemException("error in reading from fd", __FILE__, __LINE__);
}

}
