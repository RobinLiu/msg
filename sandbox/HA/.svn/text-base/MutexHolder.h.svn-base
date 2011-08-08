/*
 * MutexHolder.h
 *
 *  Created on: 2011-6-22
 *      Author: Elric
 */

#ifndef MUTEXHOLDER_H_
#define MUTEXHOLDER_H_

#include "Mutex.h"
#include <cstdio>
namespace HAUtils {

class LogicError : public std::exception {
public:
    LogicError() {};
    virtual ~LogicError() throw() {};
    LogicError(const char* text) {
        pthread_t thisThread = pthread_self();
        snprintf(errText, sizeof(errText), "Thread %05i: %s", (int)thisThread, text);
    }
    const char* what() const throw() {
        return errText;
    }
private:
    char errText[256];
};

class MutexHolder {
public:
    MutexHolder(Mutex* theMutex, bool lockNow = true)
    : m_theMutex(theMutex), m_isLocked(false) {
        if (lockNow) {
            Acquire();
        }
    }
    MutexHolder(Mutex& theMutex, bool lockNow = true)
    : m_theMutex(&theMutex), m_isLocked(false) {
        if (lockNow) {
            Acquire();
        }
    }
    virtual ~MutexHolder();
    void Acquire();
    void Release();

private:

    HAUtils::Mutex* m_theMutex;
    bool            m_isLocked;

};

}
#endif /* MUTEXHOLDER_H_ */
