/*
 * TimerHeap.h
 *
 *  Created on: 2011-6-23
 *      Author: Elric
 */

#ifndef TIMERHEAP_H_
#define TIMERHEAP_H_
#include "Mutex.h"
#include "TimeValue.h"
#include <list>
namespace TimeLib {

class TimeoutHandler;
class TimerHeapIterator;
class TimerNode;

class TimerHeap {
public:
    TimerHeap(unsigned int size, unsigned int allocThreshold, bool preallocate);
    virtual ~TimerHeap();
    /* Schedule a new timer that will expire after <delay> amount of time.
     * If it expires then <dId> and <iId> are used to form the timeout event.
     * This method returns a timer id that can be used to cancel the timer
     * before it expires.
     * */
    int schedule(const TimeValue& delay, TimeoutHandler* th);

    /**
     * Cancel the single timer that matches the timer id value, which
     * was returned from the <schedule> method. If timeValue is specified
     * then it will be used to store the canceled timer value.
     * Throws: timlibCancellationFailure().
     * */
    void cancel(int tId, TimeValue* timeValue = 0);

    void cancel(const TimeoutHandler * const th);

    /**
     * Function that returns the (monotonic) system time when the timer
     * will trigger. Zero if the timer cannot be found.
     * @param timerId The timer id to be seached for.
     * @return The (monotonic) system time when the timer will trigger
     */
    TimeValue triggeringTime(int timerId);

    /**
     * Cancel the single timer with out taking any lock that matches the timer id value, which
     * was returned from the <schedule> method.
     * Throws: timlibCancellationFailure().
     */
    void unlockedCancel(int tId);

    // Form and deliver the timeout event for all timers whose values
    // are less than or equal to <gettimeofday>. Returns the number of
    // timers canceled.
    // Throws: timlibSystemException() with error
    // cause = not enough memory.
    int expire(void);

    // Returns the number of timers awaiting timeout.
    unsigned int pending(void);

    // Resets all the current timers and returns the
    // number of timers reset.
    unsigned int reset(void);

    // The wrapper function for the <gettimeofday> system call.
    //Returns the current time of day.
    TimeValue getTimeOfDay(void);

    // Determine the next event to timeout.  Returns <max> if there are
    // no pending timers or if all pending timers are longer than max.
    TimeValue* calculateTimeout();

    // Returns a pointer to this <timlibTimerHeap>'s iterator.
    TimerHeapIterator& iter(void);

    void dump(void) const;

    static HAUtils::Mutex heapMutex;
    static int currentActiveTimer;
    friend class TimerHeapIterator;

private:
    TimerHeap(const TimerHeap&);
    TimerHeap& operator=(const TimerHeap&);

    unsigned int currentSize;
    /**
     * Boolean flag that tells if we should use POSIX clock_gettime(
     * CLOCK_MONITONIC). The flag is initially set and cleared, if
     * the operating system does not seem to support the function.
     * Use of the monotonic clock makes TIMLIN immune to system time
     * changes, as the monotonic clock is not effected by system time
     * changes.
     */
    bool mUseMonotonicClock;
    unsigned int maxSize;

    // The size of heap is doubled when it is short of <allocationThreshold> slots.
    unsigned int allocationThreshold;

    // "Pointer" to the first element in the freelist contained within the
    // <timerIds> array, which is organized as a stack.
    int timerIdsFreelist;

    // If this is non-zero, then we preallocate <maxSize> number of
    // <timlibTimerNode> objects in order to reduce dynamic allocation costs.
    // In auto-growing implementation, this points to the last array of nodes
    // allocated.
    TimerNode* preallocatedNodes;

    TimerNode* preallocatedNodesFreelist;

    // Current contents of the Heap, which is organized as a "heap" of
    // <timlibTimerNode> *'s.  In this context, a heap is a "partially ordered,
    // almost complete" binary tree, which is stored in an array.
    TimerNode** heap;

    // An array of "pointers" that allows each <timlibTimerNode> in the
    // <heap> to be located in O(1) time.  Basically, <timerIds[i]> contains the
    // index in the <heap> array where a <timlibTimerNode>* with timer id <i>
    // resides.  Thus, the timer id passed back from <schedule> is really an
    // index into the <timerIds> array. The <timerIds> array serves two purposes:
    // negative values are treated as "pointers" for the <freelist>, whereas
    // positive values are treated as "pointers" into the <heap> array.
    int* timerIds;

    // Set of pointers to the arrays of preallocated timer nodes.
    // Used to delete the allocated memory when required.
    std::list<TimerNode*> preallocatedNodesSet;

    TimeValue timeout;
    TimerHeapIterator* iterator;

    int isEmpty(void) const;
    const TimeValue& earliestTime(void) const;
    TimerNode* removeFirst(void);
    TimerNode* allocateNode(void);
    void freeNode(TimerNode* tn);
    void insert(TimerNode* newNode);
    TimerNode* remove(unsigned int index);
    void growHeap(void);
    void reheapUp(TimerNode* newNode, unsigned int index, unsigned int parent);

    //////////////////////////////////////////////////////////////////////////////
    // Restore the heap property, starting at <index>.
    void reheapDown(TimerNode* movedNode, unsigned int index,
            unsigned int child);

    //////////////////////////////////////////////////////////////////////////////
    // Copy <movedNode> into the <index> slot of <heap> and move <index> into the
    // corresponding slot in the <timerId> array.
    void copy(int index, TimerNode* movedNode);
    inline unsigned int heapParent(unsigned int x) {
        return (x == 0 ? 0 : (((x) - 1) / 2));
    }

    inline unsigned int heapChild(unsigned int x) {
        return (x = ((x) + (x) + 1));
    }

    inline int timerId(void) {
        return popFreelist();
    }

    inline int popFreelist(void) {
        int newId = timerIdsFreelist;

        // The freelist values in the <timerIds> are negative, so we need
        // to negate them to get the next freelist "pointer."
        timerIdsFreelist = -timerIds[timerIdsFreelist];
        return newId;
    }

    inline void pushFreelist(int oldId) {
        // The freelist values in the <timerIds> are negative, so we need
        // to negate them to get the next freelist "pointer."
        timerIds[oldId] = -timerIdsFreelist;
        timerIdsFreelist = oldId;
    }
};

class TimerHeapIterator {
public:
    TimerHeapIterator(TimerHeap & heap) :
        timerHeap(heap) {
    }

    virtual ~TimerHeapIterator(void) {
    }

    void first(void) {
        position = 0;
    }

    void next(void) {
        if (position != timerHeap.currentSize)
            position++;
    }

    int isDone(void) {
        return position == timerHeap.currentSize;
    }

    TimerNode* item(void) {
        if (position != timerHeap.currentSize)
            return timerHeap.heap[position];
        return 0;
    }

private:
    TimerHeap& timerHeap;
    unsigned int position;
};

}

#endif /* TIMERHEAP_H_ */
