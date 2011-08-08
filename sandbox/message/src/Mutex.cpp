#include "Mutex.h"
#include "common/include/common.h"
#include <cstdlib>
#include <cerrno>
#include <iostream>

namespace HAUtils {

Mutex::Mutex()
    : valid_(1), thread_(0), nest_(0) {
    pthread_mutex_init(&mutex_, NULL);
}

Mutex::~Mutex() {
    valid_ = 0;
    pthread_t theThread = pthread_self();
    if (pthread_equal(theThread, thread_) != 0) {
        if (pthread_mutex_unlock(&mutex_) != 0) {
            //TODO: error unlock
        }
    }
    if (pthread_mutex_destroy(&mutex_) != 0) {
        //TODO: error destroy mutex
    }
}

void Mutex::lock() throw(MutexException) {
    if (valid_ == 0) {
        return;
    }
    pthread_t theThread = pthread_self();

    if (pthread_equal(theThread, thread_) != 0) {
        ++nest_;
        LOG(INFO) <<"thread " << theThread <<" lock mutex " << (void*)&mutex_<<" ok\n";
    } else {
        if (pthread_mutex_lock(&mutex_) == 0) {
            thread_ = theThread;
            nest_ = 0;
			LOG(INFO) <<"thread " << theThread <<" lock mutex " << (void*)&mutex_<<" ok\n";
        } else {
            //			abort();
            throw MutexException("pthread_mutex_lock failed");
        }
    }

}
void Mutex::unlock() throw (MutexException) {
    if (valid_ == 0) {
        return;
    }

    pthread_t theThread = pthread_self();
    if (pthread_equal(theThread, thread_) == 0) {
        //thread does not own the mutex and try to unlock it.
        abort();
    }
    if (nest_ > 0) {
        LOG(INFO) <<"thread " << theThread <<" unlock mutex " << (void*)&mutex_<<" ok";
        --nest_;
    } else {
        thread_ = 0;
        if (pthread_mutex_unlock(&mutex_) != 0) {
            throw MutexException("pthread_mutex_unlock failed");
        }
        LOG(INFO) <<"thread " << theThread <<" unlock mutex " << (void*)&mutex_<<" ok\n";
    }
}

bool Mutex::trylock() throw (MutexException) {
    if (valid_ == 0) {
        return false;
    }
    pthread_t theThread = pthread_self();

    if (pthread_equal(theThread, thread_) != 0) {
        ++nest_;
        return true;
    } else {
        int code = pthread_mutex_trylock(&mutex_);
        if (code == 0) {
            thread_ = theThread;
            nest_ = 0;
            return true;
        } else if (code == EBUSY) {
            return false;
        } else {
            throw MutexException("pthread_mutex_trylock failed");
        }
    }

    return false;
}

}
