/*
 * TimerNode.h
 *
 *  Created on: 2011-6-23
 *      Author: Elric
 */

#ifndef TIMERNODE_H_
#define TIMERNODE_H_

#include "TimeoutHandler.h"
#include "TimeValue.h"


namespace TimeLib {

class TimerNode {
public:
    TimerNode() :
        timeoutHandler(0), nextNode(0) {
    }

    virtual ~TimerNode() {
    }

    void set(TimeoutHandler* th, const TimeValue& tv, TimerNode* tn,
            int tId) {
        timeoutHandler = th;
        timerValue = tv;
        nextNode = tn;
        timerId = tId;
    }

    TimeoutHandler* getTimeoutHandler(void) const {
        return timeoutHandler;
    }

    void setTimeoutHandler(TimeoutHandler* th) {
        timeoutHandler = th;
    }

    TimeValue& getTimerValue(void) {
        return timerValue;
    }

    void setTimerValue(TimeValue tv) {
        timerValue = tv;
    }

    TimerNode* getNextNode(void) const {
        return nextNode;
    }
    void setNextNode(TimerNode* tn) {
        nextNode = tn;
    }

    int getTimerId(void) const {
        return timerId;
    }
    void setTimerId(int tId) {
        timerId = tId;
    }

    void dump(void) const;
    void passivate() {
        timerValue.set(0, 0);
    }

private:
    TimeoutHandler* timeoutHandler;
    TimeValue timerValue;
    TimerNode* nextNode;
    int timerId;
};

}

#endif /* TIMERNODE_H_ */
