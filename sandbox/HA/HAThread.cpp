/*
 * HAThread.cpp
 *
 *  Created on: 2011-6-22
 *      Author: Elric
 */

#include "HAThread.h"
#include "log.h"
#include <pthread.h>
namespace HAUtils {



void HAThread::start() throw(HAUtils::HAThreadException) {
    if (m_IsRunning) {
        throw HAThreadException("The thread is already running");
    }
    pWrapperfn thdEntryRoutine = (pWrapperfn)&runWrapper;
    LOG(INFO)<<"Start create thread";
    if (createThread(&m_ThreadId, thdEntryRoutine, (void*)this, m_StackSize)) {
        throw HAThreadException("Create Thread failed");
    }
    LOG(INFO)<<"Create thread ok.";
    if (!m_Joinable) {
        pthread_detach(m_ThreadId);
    }
}

void HAThread::stop() throw(HAUtils::HAThreadException) {
    if (pthread_self() != m_ThreadId) {
        throw HAThreadException("HAThread::stop(): Stop is not called from the thread that was started.");
    }
    m_IsRunning = false;
    pthread_exit(NULL);
}

void HAThread::runWrapper(void * objRef) {
    ((HAThread*)objRef)->setRunning(true);
    try {
        LOG(INFO)<<"Try to run thread";
        ((HAThread*)objRef)->run();
    }
    catch (const HAThreadTerminatedException& e){
        LOG(ERROR)<<"Run thread failed";
    }
    ((HAThread*)objRef)->setRunning(false);
}

int HAThread::createThread(pthread_t* pthread_id,
                        void*(*start_routine)(void*),
                        void* arg, int stackSize) {
    //TODO: check which header file include this macro
    if (stackSize > 0/*PTHREAD_STACK_MIN*/) {
        pthread_attr_t attr;
        int result = 0;
        if(0 != (result = pthread_attr_init(&attr))) {
            LOG(ERROR)<<"pthread_attr_init failed, error code: "<<result;
            return result;
        }
        if( 0 != (result = pthread_attr_setstacksize(&attr, stackSize))) {
            pthread_attr_destroy(&attr);
            LOG(ERROR)<<"pthread_attr_setstacksize failed, error code: "<<result;
            return result;
        }
        result = pthread_create(pthread_id, &attr, start_routine, arg);
        pthread_attr_destroy(&attr);
        return result;
    } else {
        LOG(INFO)<<"create thread";
        return pthread_create(pthread_id, NULL, start_routine, arg);
    }

}

HAThread::~HAThread() {
    // TODO Auto-generated destructor stub
}

}
