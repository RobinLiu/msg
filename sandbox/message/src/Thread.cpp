#include "Thread.h"
#include "common/include/common.h"
#include <pthread.h>
namespace HAUtils {



void Thread::start() throw(HAUtils::ThreadException) {
    if (m_IsRunning) {
        throw ThreadException("The thread is already running");
    }
    pWrapperfn thdEntryRoutine = (pWrapperfn)&runWrapper;
    LOG(INFO)<<"Start create thread";
    if (createThread(&m_ThreadId, thdEntryRoutine, (void*)this, m_StackSize)) {
        throw ThreadException("Create Thread failed");
    }
    LOG(INFO)<<"Create thread ok.";
    if (!m_Joinable) {
        pthread_detach(m_ThreadId);
    }
}

void Thread::stop() throw(HAUtils::ThreadException) {
    if (pthread_self() != m_ThreadId) {
        throw ThreadException("Thread::stop(): Stop is not called from the thread that was started.");
    }
    m_IsRunning = false;
    pthread_exit(NULL);
}

void Thread::runWrapper(void * objRef) {
    ((Thread*)objRef)->setRunning(true);
    try {
        LOG(INFO)<<"Try to run thread";
        ((Thread*)objRef)->run();
    }
    catch (const ThreadTerminatedException& e){
        LOG(ERROR)<<"Run thread failed";
    }
    ((Thread*)objRef)->setRunning(false);
}

int Thread::createThread(pthread_t* pthread_id,
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

Thread::~Thread() {
}

}
