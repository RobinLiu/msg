/*
 * MutexException.h
 *
 *  Created on: 2011-6-21
 *      Author: Elric
 */

#ifndef MUTEXEXCEPTION_H_
#define MUTEXEXCEPTION_H_

#include <exception>
#include <string>

namespace HAUtils {

class MutexException : public std::exception {
public:
	MutexException(const char* errText) throw() :m_reason(errText) {};
	MutexException(const MutexException& e): std::exception(), m_reason(e.m_reason){}
	MutexException& operator=(MutexException& e){
		m_reason = e.m_reason;
		return *this;
	}
	virtual ~MutexException() throw() {};
	virtual const char* what() const throw() {
	    return m_reason.c_str();
	}
private:
	std::string m_reason;
};

class HAThreadException : public std::exception {
public:
    HAThreadException(const char* errText) throw() :m_reason(errText) {};
    HAThreadException(const HAThreadException& e): std::exception(), m_reason(e.m_reason){}
    HAThreadException& operator=(HAThreadException& e){
        m_reason = e.m_reason;
        return *this;
    }
    virtual ~HAThreadException() throw() {};
    virtual const char* what() const throw() {
        return m_reason.c_str();
    }
private:
    std::string m_reason;
};

class HAThreadTerminatedException : public HAThreadException {
public:
    HAThreadTerminatedException() throw()
    : HAThreadException( "HAThreadTerminatedException" )
    {
    };
    HAThreadTerminatedException( const HAThreadTerminatedException& c )
    : HAThreadException( c )
    {
    };
    virtual ~HAThreadTerminatedException() throw() {};
};

}

#endif /* MUTEXEXCEPTION_H_ */
