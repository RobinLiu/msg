/*
 * TimerHeap.cpp
 *
 *  Created on: 2011-6-23
 *      Author: Elric
 */

#include "TimerHeap.h"
#include "TimerNode.h"
#include "TimeoutHandler.h"
#include "MutexHolder.h"
#include <pthread.h>
#include <list>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <typeinfo>
#include <syslog.h>
#include <iostream>

//#define gnu_dev_major major
namespace TimeLib {

HAUtils::Mutex TimerHeap::heapMutex;

int TimerHeap::currentActiveTimer = 0;

TimerHeap::TimerHeap(unsigned int size, unsigned int allocThreshold,
        bool preallocate) :
    currentSize(0), mUseMonotonicClock(true), maxSize(size),
            allocationThreshold(allocThreshold), timerIdsFreelist(1),
            preallocatedNodes(0), preallocatedNodesFreelist(0), heap(0),
            timerIds(0) {

    try {

        // Create the heap array.
        heap = new (TimerNode *[size]);

        // Create the parallel timer ids array.
        timerIds = new int[size];

        // Initialize the "freelist," which uses negative values to distinguish
        // freelist elements from "pointers" into the <heap> array.
        for (unsigned int i = 0; i < size; i++)
            timerIds[i] = -((int) (i + 1));

        if (preallocate) {
            preallocatedNodes = new TimerNode[size];

            // Add to the set for later deletion.
            preallocatedNodesSet.push_front(preallocatedNodes);

            // Form the freelist by linking the next node pointers together.
            for (unsigned int j = 1; j < size; j++)
                preallocatedNodes[j - 1].setNextNode(&preallocatedNodes[j]);

            // NULL-terminate the freelist.
            preallocatedNodes[size - 1].setNextNode(0);

            // Assign the freelist pointer to the front of the list.
            preallocatedNodesFreelist = &preallocatedNodes[0];
        }

        iterator = new TimerHeapIterator(*this);
    } catch (...) // cssmanSysMemoryAllocationFailure&)
    {
        // Do the necessary clean-up and rethrow the exception.
        delete[] heap;
        delete[] timerIds;
        delete[] preallocatedNodes;
        throw;
    }
}

TimerHeap::~TimerHeap() {
    delete iterator;

    // Clean all the nodes still in the std::queue
    for (unsigned int i = 0; i < currentSize; i++)
        freeNode(heap[i]);

    // Clean up any preallocated timer nodes
    if (preallocatedNodes != 0) {
        std::list<TimerNode*>::iterator pn;
        for (pn = preallocatedNodesSet.begin(); pn
                != preallocatedNodesSet.end(); pn++)
            delete *pn;
    }

    delete[] heap;
    delete[] timerIds;
}

int TimerHeap::schedule(const TimeValue & futureTime, TimeoutHandler* th) {
    HAUtils::MutexHolder mh(heapMutex);

    if (currentSize < maxSize) {
        // Obtain the next unique sequence number.
        int tId = timerId();

        // Obtain the memory to the new node.
        TimerNode* temp = allocateNode();

        if (temp) {
            temp->set(th, futureTime, 0, tId);
            insert(temp);
            return tId;
        }
    }
    return 0;
}

void TimerHeap::cancel(int timerId, TimeValue* timeValue) {

    HAUtils::MutexHolder mh(heapMutex);

    // The timer that is being cancelled, might be active
    // right now. In that case this "cancel" will fail, but
    // we inform the caller of the condition.
    if (currentActiveTimer == timerId) {
        throw TimeoutHandlerRunning(
                "The TimeoutHandler of the timer is currently executing",
                __FILE__, __LINE__);
    }

    // Locate the timerNode that corresponds to the timerId.

    // Check to see if the timerId is out of range
    if (timerId < 0 || timerId >= (int) maxSize) {
        throw CancellationFailure("timer id out of range", __FILE__, __LINE__);
    }

    int timerNodeSlot = timerIds[timerId];

    // Check to see if timerId is still valid.
    if (timerNodeSlot < 0) {
        throw CancellationFailure("invalid timer id", __FILE__, __LINE__);
    }

    TimerNode* temp = remove(timerNodeSlot);

    if (timeValue && temp) {
        *timeValue = temp->getTimerValue();
    }
    freeNode(temp);
}

void TimerHeap::cancel(const TimeoutHandler * const th) {
    HAUtils::MutexHolder mh(heapMutex);

    for (int i = (currentSize - 1); i >= 0; --i)
        if (heap[i] && heap[i]->getTimeoutHandler() == th)
            cancel(heap[i]->getTimerId());
}

TimeValue TimerHeap::triggeringTime(int timerId) {

    TimeValue ret;
    HAUtils::MutexHolder mh(heapMutex);

    // Timer currently active?
    if (currentActiveTimer == timerId) {
        return ret;
    }

    // Locate the timerNode that corresponds to the timerId.

    // Check to see if the timerId is out of range
    if (timerId < 0 || timerId >= (int) maxSize) {
        return ret;
    }

    int timerNodeSlot = timerIds[timerId];

    // Check to see if timerId is still valid.
    if (timerNodeSlot < 0) {
        return ret;
    }

    TimerNode* temp = heap[timerNodeSlot];
    ret = temp->getTimerValue();
    return ret;
}

void TimerHeap::unlockedCancel(int timerId) {
    // Check to see if the timerId is out of range
    if (timerId < 0 || timerId >= (int) maxSize)
        throw CancellationFailure("timer id out of range", __FILE__, __LINE__);

    int timerNodeSlot = timerIds[timerId];

    // Check to see if timerId is still valid.
    if (timerNodeSlot < 0)
        throw CancellationFailure("invalid timer id", __FILE__, __LINE__);

    TimerNode* temp = remove(timerNodeSlot);
    freeNode(temp);

}

int TimerHeap::expire(void) {
    HAUtils::MutexHolder mh(heapMutex);

    int numberOfTimersExpired = 0;
    TimerNode* expired;
    TimeoutHandler* th;

    // Keep looping while there are timers remaining and the earliest
    // timer is <= the <curTime> passed in to the method.
    if (isEmpty())
        return 0;

    TimeValue curTime = getTimeOfDay();
    bool timeToQuit = false;
    if (earliestTime() > curTime)
        timeToQuit = true;
    while (!timeToQuit) {
        expired = heap[0];
        th = expired->getTimeoutHandler();
        int timId = expired->getTimerId();
        expired->passivate();
        currentActiveTimer = timId;
        mh.Release();
        try {
            th->handleTimeout(timId);
        } catch (const std::exception& e) {
            syslog(
                    LOG_INFO,
                    "Timlib::TimerHeap : CODE ERROR: A TimeoutHandler throw a std exception: %s",
                    e.what());
        } catch (...) {
            std::cout << "Exception from HandleTimeout" << std::endl;
            syslog(LOG_INFO,
                    "Timlib::TimerHeap : CODE ERROR: A TimeoutHandler throw an unknown exception");
            try {
                std::cout << "Name of failed Timeout Handler: '" << th->name
                        << "'" << std::endl;
            } catch (...) {
            }
        }
        mh.Acquire();
        currentActiveTimer = 0;
        removeFirst();
        freeNode(expired);

        // Call the factory method to free up the node. But check first
        // if the timer has been re-scheduled.
        // if (expired->getTimerValue() <= curTime)
        // {
        // freeNode(expired);
        // }

        numberOfTimersExpired++;

        if (isEmpty()) {
            // break;
            timeToQuit = true;
        } else {
            // Does it look like we've processed all timers?
            if (earliestTime() > curTime) {
                // Re-evaluate curTime: execution of timeout handler can have
                // lasted longer.
                curTime = getTimeOfDay();
                // Does it still look like everything is done?
                if (earliestTime() > curTime)
                    timeToQuit = true; // Yes: Quit the loop
            }
        }
    }

    return numberOfTimersExpired;
}

unsigned int TimerHeap::pending(void) {
    unsigned int remainingTimers = 0;

    HAUtils::MutexHolder mh(heapMutex);

    iterator->first();

    for (; !iterator->isDone(); iterator->next()) {
        // TimerNode* tn = iterator->item();
        remainingTimers++;
    }

    return remainingTimers;
}

unsigned int TimerHeap::reset(void) {
    unsigned int noOfTimersReset = 0;

    iterator->first();

    while (!iterator->isDone()) {
        TimerNode* tn = iterator->item();
        try {
            cancel(tn->getTimerId());
        } catch (...) {
        }
        iterator->first();
        noOfTimersReset++;
    }

    return noOfTimersReset;
}

TimeValue TimerHeap::getTimeOfDay(void) {

    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    if (mUseMonotonicClock) {
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
        if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
            mUseMonotonicClock = false;
            std::string
                    s(
                            "Current kernel does not support the POSIX MONOTONIC clock (");
            s += strerror(errno);
            s += ") System time changes will have severe impact on ";
            s += "process timers.";
            syslog(LOG_WARNING, "HAS Timer Library[%i]: %s", getpid(),
                    s.c_str());
        } else {
            tv.tv_sec = ts.tv_sec;
            tv.tv_usec = ts.tv_nsec / 1000;
        }
    }

    if (mUseMonotonicClock == false) {
        ::gettimeofday(&tv, 0);
    }

    return TimeValue(tv);
}

TimeValue* TimerHeap::calculateTimeout() {

    if (isEmpty()) {
        // Nothing on the Timer Queue, so block indefinitely.
        return 0;
    } else {
        TimeValue curTime = getTimeOfDay();

        if (earliestTime() > curTime) {
            // The earliest item on the Timer Queue is still in the
            // future.  Therefore, use the delta time between now and
            // the earliest time on the Timer Queue.
            timeout = earliestTime() - curTime;
            return &timeout;
        } else {
            // The earliest item on the Timer Queue is now in the past.
            // Therefore, just dispatch timers.
            timeout = 0;
            return &timeout;
        }
    }
}

TimerHeapIterator& TimerHeap::iter(void) {

    iterator->first();
    return *iterator;
}

void TimerHeap::dump(void) const {

}

int TimerHeap::isEmpty(void) const {
    return currentSize == 0;
}

const TimeValue& TimerHeap::earliestTime(void) const {
    return heap[0]->getTimerValue();
}

TimerNode* TimerHeap::removeFirst(void) {

    if (currentSize == 0)
        return 0;

    return remove(0);
}

TimerNode* TimerHeap::allocateNode(void) {

    TimerNode* temp = 0;
    // Only allocate a node if we are *not* using the preallocated heap.
    if (preallocatedNodes == 0) {
        temp = new TimerNode;
    } else {
        // check to see if the heap needs to grow
        if (preallocatedNodesFreelist == 0)
            growHeap();

        temp = preallocatedNodesFreelist;

        // Remove the first element from the freelist.
        preallocatedNodesFreelist = preallocatedNodesFreelist->getNextNode();
    }

    return temp;
}

void TimerHeap::freeNode(TimerNode* node) {

    // Only free up a node if we are *not* using the preallocated heap.
    if (preallocatedNodes == 0)
        delete node;
    else {
        node->setNextNode(preallocatedNodesFreelist);
        preallocatedNodesFreelist = node;
    }
}

void TimerHeap::insert(TimerNode* newNode) {

    if (currentSize + allocationThreshold >= maxSize)
        growHeap();

    reheapUp(newNode, currentSize, heapParent(currentSize));

    currentSize++;
}

TimerNode* TimerHeap::remove(unsigned int index) {

    TimerNode* removedNode = heap[index];

    // Return this timer id to the freelist.
    pushFreelist(removedNode->getTimerId());

    // Decrement the size of the heap by one since we're removing the
    // "index"th node.
    currentSize--;

    // Only try to reheapify if we're not deleting the last entry.

    if (index < currentSize) {
        TimerNode* movedNode = heap[currentSize];

        // Move the end node to the location being removed and update
        // the corresponding slot in the parallel <timerIds> array.
        copy(index, movedNode);

        // If the <movedNode->timeValue> is great than or equal its
        // parent it needs be moved down the heap.
        unsigned int parent = heapParent(index);

        if (movedNode->getTimerValue() >= heap[parent]->getTimerValue())
            reheapDown(movedNode, index, heapChild(index));
        else
            reheapUp(movedNode, index, parent);
    }

    return removedNode;
}

void TimerHeap::growHeap(void) {

    TimerNode** newHeap = 0;
    int* newTimerIds = 0;

    try {
        // All the containers will double in size from maxSize
        unsigned int newSize = maxSize * 2;

        // First grow the heap itself.

        newHeap = new TimerNode*[newSize];

        ::memcpy(newHeap, heap, maxSize * (sizeof(*newHeap)));

        delete[] heap;
        heap = newHeap;

        // Grow the array of timer ids.

        newTimerIds = new int[newSize];

        ::memcpy(newTimerIds, timerIds, maxSize * (sizeof(int)));

        delete[] timerIds;
        timerIds = newTimerIds;

        // and add the new elements to the end of the "freelist"
        for (unsigned int i = maxSize; i < newSize; i++)
            timerIds[i] = -((int) (i + 1));

        // Grow the preallocation array (if using preallocation)
        if (preallocatedNodes != 0) {
            // Create a new array with maxSize elements to link in
            // to existing list.
            preallocatedNodes = 0;
            preallocatedNodes = new TimerNode[maxSize];

            // Add to the set for later deletion.
            preallocatedNodesSet.push_front(preallocatedNodes);

            // link new nodes together (as for original list).
            for (unsigned int k = 1; k < maxSize; k++)
                preallocatedNodes[k - 1].setNextNode(&preallocatedNodes[k]);

            // NULL-terminate the new list.
            preallocatedNodes[maxSize - 1].setNextNode(0);

            // link new array to the end of the existling std::list
            if (preallocatedNodesFreelist == 0)
                preallocatedNodesFreelist = &preallocatedNodes[0];
            else {
                TimerNode* previous = preallocatedNodesFreelist;

                for (TimerNode* current =
                        preallocatedNodesFreelist->getNextNode(); current != 0; current
                        = current->getNextNode())
                    previous = current;

                previous->setNextNode(&preallocatedNodes[0]);
            }
        }

        maxSize = newSize;
    } catch (...) // cssmanSysMemoryAllocationFailure&)
    {
        // Do the necessary clean-up and rethrow the exception.
        delete[] newHeap;
        delete[] newTimerIds;
        throw;
    }
}

void TimerHeap::reheapUp(TimerNode* movedNode, unsigned int index,
        unsigned int parent) {

    // Restore the heap property after an insertion.

    while (index > 0) {
        // If the parent node is greater than the <movedNode> we need
        // to copy it down.
        if (movedNode->getTimerValue() < heap[parent]->getTimerValue()) {
            copy(index, heap[parent]);
            index = parent;
            parent = heapParent(index);
        } else
            break;
    }

    // Insert the new node into its proper resting place in the heap and
    // update the corresponding slot in the parallel <timerIds> array.
    copy(index, movedNode);
}

void TimerHeap::reheapDown(TimerNode* movedNode, unsigned int index,
        unsigned int child) {
    // Restore the heap property after a deletion.
    while (child < currentSize) {
        // Choose the smaller of the two children.
        if ((child + 1 < currentSize) && (heap[child + 1]->getTimerValue()
                < heap[child]->getTimerValue()))
            child++;

        // Perform a <copy> if the child has a larger timeout value than
        // the <movedNode>.
        if (heap[child]->getTimerValue() < movedNode->getTimerValue()) {
            copy(index, heap[child]);
            index = child;
            child = heapChild(child);
        } else
            // We've found our location in the heap.
            break;
    }

    copy(index, movedNode);
}

void TimerHeap::copy(int index, TimerNode* movedNode) {
    // Insert <movedNode> into its new location in the heap.
    heap[index] = movedNode;

    // Update the corresponding slot in the parallel <timerIds> array.
    timerIds[movedNode->getTimerId()] = index;
}

}
