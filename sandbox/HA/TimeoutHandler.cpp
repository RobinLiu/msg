/*
 * TimeoutHandler.cpp
 *
 *  Created on: 2011-6-23
 *      Author: Elric
 */

#include "TimeoutHandler.h"

namespace TimeLib {
TimeoutHandler::~TimeoutHandler() {
    try {
        TimerManager::getInstance()->cancelTimeout(this);
    } catch (...) {
    }
}

}
