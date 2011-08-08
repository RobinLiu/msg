#include "MutexHolder.h"

namespace HAUtils {

void MutexHolder::Acquire() {
    if (m_isLocked) {
        throw LogicError("MutexHolder::Acquire: logic errro. Attempt to acquire a lock that is already locked");
    }
    m_theMutex->lock();
    m_isLocked = true;
}

void MutexHolder::Release() {
    if (!m_isLocked) {
        throw LogicError("MutexHolder::Release: logic errro. Attempt to release an unlock mutex.");
    }
    m_theMutex->unlock();
    m_isLocked = false;
}

MutexHolder::~MutexHolder() {
    if (m_isLocked) {
        m_theMutex->unlock();
    }
}

}
