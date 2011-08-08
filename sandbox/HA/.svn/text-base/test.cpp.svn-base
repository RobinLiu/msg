//============================================================================
// Name        : test.cpp
// Author      :
// Version     :
// Copyright   : Your copyright notice
// Description :
//============================================================================

#include "MutexHolder.h"
#include <iostream>
#include <unistd.h>
#include "glog/logging.h"
#include "HAThread.h"
#include "TimerManager.h"
#include "TimeoutHandler.h"
#include <cstdlib>

class testThread : public HAUtils::HAThread {
public:
    void run(){
        while(1) {
            LOG(INFO)<<"I'm thread"<<pthread_self();
            sleep(1);
        }
    }
};

class tmh : public TimeLib::TimeoutHandler {
    void handleTimeout(int tid) {
        LOG(INFO)<<"handleTimeout in children class";
    }
};

HAUtils::Mutex theMutex;


long simpleWakeups = 0;
long complexWakeups = 0;
class MyTimeoutHandler: public TimeLib::TimeoutHandler {
public:
    long m_wakeups;
    MyTimeoutHandler() {
        m_wakeups = 0;
    }
    ;
    void handleTimeout(int32_t tid) {
        std::cout << "MyTimeoutHandler: TimerId " << tid << " expired"
                << std::endl;
        simpleWakeups++;
        delete this;
    }
};
struct ComplexInfo {
    char action[32];
    long count;

    ComplexInfo() :
        count(0) {
        strcpy(action, "ABCDEH");
    }
};

class DeleteHandler: public TimeLib::TimeoutHandler {
public:
    DeleteHandler(size_t i, unsigned long delay) {
        std::cout << "Creating " << i << ". timeouthandler with " << (delay
                / 1000000) << " s timeout" << std::endl;
        TimeLib::TimerManager::getInstance()->registerTimeout(delay, this);
    }
    virtual ~DeleteHandler() {
        std::cout << "Timeout handler deleted, timeouts not cancelled"
                << std::endl;
    }
    virtual void handleTimeout(int32_t id) {
        std::cout << "This should not have been called!" << std::endl;
        exit(1);
    }
};

class ComplexTimeoutHandler: public TimeLib::TimeoutHandler {
    ComplexInfo i;
    long tid;
    TimeLib::TimerManager* tm;
    long wakeups;
public:
    ComplexTimeoutHandler(unsigned long delay, ComplexInfo ci) {
        wakeups = 0;
        tid = -1;
        strcpy(name, "ComplexTimeoutHandler");
        strcpy(i.action, ci.action);
        i.count = ci.count;
        tm = NULL;
        tm = TimeLib::TimerManager::getInstance();
        tid = tm->registerTimeout(delay, this);
    }
    ~ComplexTimeoutHandler() {
        std::cout << "~ComplexTimeout() " << i.action << "#" << i.count
                << std::endl;
        if (tid >= 0) {
            std::cout << "Cancelling TimerId#" << tid << std::endl;
            tm->cancelTimeout(tid);
        }
    }
    void handleTimeout(int32_t timerId) {
        wakeups++;
        complexWakeups++;
        std::cout << "ComplexTimeoutHandler::HandleTimeout() TID#" << timerId
                << "-" << tid << std::endl;
        std::cout << "Action: " << i.action << "\tCount: " << i.count
                << std::endl;
        if (tid != timerId) {
            std::cout << "ERROR: Expecting TID " << tid
                    << " but wokeup with TID " << timerId << std::endl;
        }
        tid = -1;
        if (wakeups >= 10) {
            delete this;
            return;
        }
        try {
            tid = tm->registerTimeout(2000000, this);
            std::cout << "OldTimerId: " << timerId << " changed to " << tid
                    << std::endl;
        } catch (...) {
            std::cout << "Exception from registerTimeout()" << std::endl;
        }
    }
};


using namespace std;
int main(int argc, char* argv[]) {
    google::InitGoogleLogging(argv[0]);
    TimeLib::TimerManager* timerMgr = TimeLib::TimerManager::getInstance();
    tmh tm;
    tmh tm2;
    int tId = timerMgr->registerTimeout(5000000, &tm);
    sleep(2);
    timerMgr->cancelTimeout(&tm);
    {HAUtils::MutexHolder mh(theMutex);}
    testThread* a = new testThread();
    testThread* b = new testThread();
//    a->start();
//    b->start();
	
	while(1) {
	sleep(1);
	}
    return 0;
}
