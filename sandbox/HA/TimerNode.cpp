/*
 * TimerNode.cpp
 *
 *  Created on: 2011-6-23
 *      Author: Elric
 */

#include "TimerNode.h"
#include <iostream>
namespace TimeLib {

void TimerNode::dump(void) const {
    std::cout << "timerId = " << timerId << std::endl;
    timerValue.dump();
    std::cout << "nextNode = " << nextNode << std::endl;
    std::cout << "timeoutHandler = " << timeoutHandler << std::endl;
}

}
