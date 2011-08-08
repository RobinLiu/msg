/*
 * TimeoutHandler.h
 *
 *  Created on: 2011-6-23
 *      Author: Elric
 */

#ifndef TIMEOUTHANDLER_H_
#define TIMEOUTHANDLER_H_
#include <cstring>

#include "TimerManager.h"
namespace TimeLib {

class TimeoutHandler {
public:
    char name[32];
    TimeoutHandler() {
        std::strcpy(name, "TimeLib::TimeoutHandler");
    }
    virtual ~TimeoutHandler();
    virtual void handleTimeout(int tid) = 0;
};

}

#endif /* TIMEOUTHANDLER_H_ */
