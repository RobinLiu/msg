/*
 * TimeValue.h
 *
 *  Created on: 2011-6-23
 *      Author: Elric
 */

#ifndef TIMEVALUE_H_
#define TIMEVALUE_H_

#include <sys/time.h>


namespace TimeLib {

class TimeValue {
public:
    TimeValue() {
        set(0, 0);
    }
    TimeValue(int sec, int usec = 0) {
        set(sec, usec);
        normalize();
    }
    TimeValue(const TimeValue& tv) : timeVal(tv.timeVal) {}
    TimeValue(const struct timeval& tv) {
        set(tv);
    }
    virtual ~TimeValue() {};
    void set(int sec, int usec) {
        timeVal.tv_sec = sec;
        timeVal.tv_usec = usec;
    }
    void set(const timeval& tv) {
        timeVal.tv_sec = tv.tv_sec;
        timeVal.tv_usec = tv.tv_usec;
        normalize();
    }
    void dump(void) const;
    void operator +=(const TimeValue& tv);
    void operator -=(const TimeValue& tv);
    friend TimeValue operator +(const TimeValue& tv1, const TimeValue& tv2);
    friend TimeValue operator -(const TimeValue& tv1, const TimeValue& tv2);
    friend int operator <(const TimeValue& tv1, const TimeValue& tv2);
    friend int operator >(const TimeValue& tv1, const TimeValue& tv2);
    static const int ONE_SECOND_IN_USECS;
    int get_sec(void) const {
        return (int) timeVal.tv_sec;
    }
    int get_usec(void) const {
        return (int) timeVal.tv_usec;
    }
    friend int operator <=(const TimeValue& tv1, const TimeValue& tv2);
    friend int operator >=(const TimeValue& tv1, const TimeValue& tv2);
    friend int operator ==(const TimeValue& tv1, const TimeValue& tv2);
    friend int operator !=(const TimeValue& tv1, const TimeValue& tv2);

private:
    struct timeval timeVal;
    void normalize(void);
};



inline void TimeValue::operator+=(const TimeValue& tv) {
    timeVal.tv_sec += tv.timeVal.tv_sec;
    timeVal.tv_usec += tv.timeVal.tv_usec;
    normalize();
}

inline void TimeValue::operator-=(const TimeValue& tv) {
    timeVal.tv_sec -= tv.timeVal.tv_sec;
    timeVal.tv_usec -= tv.timeVal.tv_usec;
    normalize();
}

inline TimeValue operator +(const TimeValue& tv1, const TimeValue& tv2) {
    TimeValue sum(tv1.timeVal.tv_sec + tv2.timeVal.tv_sec,
            tv1.timeVal.tv_usec + tv2.timeVal.tv_usec);
    sum.normalize();
    return sum;
}

inline TimeValue operator -(const TimeValue& tv1, const TimeValue& tv2) {
    TimeValue delta(tv1.timeVal.tv_sec - tv2.timeVal.tv_sec,
            tv1.timeVal.tv_usec - tv2.timeVal.tv_usec);
    delta.normalize();
    return delta;
}

inline int operator <(const TimeValue& tv1, const TimeValue& tv2) {

    return tv2 > tv1;
}

inline int operator >(const TimeValue& tv1, const TimeValue& tv2) {
    if (tv1.timeVal.tv_sec > tv2.timeVal.tv_sec)
        return 1;
    else if (tv1.timeVal.tv_sec == tv2.timeVal.tv_sec && tv1.timeVal.tv_usec
            > tv2.timeVal.tv_usec)
        return 1;
    else
        return 0;
}

inline int operator <=(const TimeValue& tv1, const TimeValue& tv2) {
    return tv2 >= tv1;
}

inline int operator >=(const TimeValue& tv1, const TimeValue& tv2) {
    if (tv1.timeVal.tv_sec > tv2.timeVal.tv_sec)
        return 1;
    else if (tv1.timeVal.tv_sec == tv2.timeVal.tv_sec && tv1.timeVal.tv_usec
            >= tv2.timeVal.tv_usec)
        return 1;
    else
        return 0;
}

inline int operator ==(const TimeValue& tv1, const TimeValue& tv2) {
    return tv1.timeVal.tv_sec == tv2.timeVal.tv_sec && tv1.timeVal.tv_usec
            == tv2.timeVal.tv_usec;
}

inline int operator !=(const TimeValue& tv1, const TimeValue& tv2) {
    return !(tv1 == tv2);
}
}

#endif /* TIMEVALUE_H_ */
