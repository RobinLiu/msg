/*
 * TimeValue.cpp
 *
 *  Created on: 2011-6-23
 *      Author: Elric
 */

#include "TimeValue.h"
#include <iostream>

namespace TimeLib {

const int TimeValue::ONE_SECOND_IN_USECS = 1000000;

void TimeValue::normalize(void) {

    if (timeVal.tv_usec >= ONE_SECOND_IN_USECS) {
        do {
            timeVal.tv_sec++;
            timeVal.tv_usec -= ONE_SECOND_IN_USECS;
        } while (timeVal.tv_usec >= ONE_SECOND_IN_USECS);
    } else if (timeVal.tv_usec <= -ONE_SECOND_IN_USECS) {
        do {
            timeVal.tv_sec--;
            timeVal.tv_usec += ONE_SECOND_IN_USECS;
        } while (timeVal.tv_usec <= -ONE_SECOND_IN_USECS);
    }

    if (timeVal.tv_sec >= 1 && timeVal.tv_usec < 0) {
        timeVal.tv_sec--;
        timeVal.tv_usec += ONE_SECOND_IN_USECS;
    } else if (timeVal.tv_sec < 0 && timeVal.tv_usec > 0) {
        timeVal.tv_sec++;
        timeVal.tv_usec -= ONE_SECOND_IN_USECS;
    }
}

void TimeValue::dump(void) const {

    std::cerr << "tv_sec = " << timeVal.tv_sec << std::endl;
    std::cerr << "tv_usec = " << timeVal.tv_usec << std::endl;
}

}
