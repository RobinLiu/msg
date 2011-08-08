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

class ThreadException : public std::exception {
public:
    ThreadException(const char* errText) throw() :m_reason(errText) {};
    ThreadException(const ThreadException& e): std::exception(), m_reason(e.m_reason){}
    ThreadException& operator=(ThreadException& e){
        m_reason = e.m_reason;
        return *this;
    }
    virtual ~ThreadException() throw() {};
    virtual const char* what() const throw() {
        return m_reason.c_str();
    }
private:
    std::string m_reason;
};

class ThreadTerminatedException : public ThreadException {
public:
    ThreadTerminatedException() throw()
    : ThreadException( "ThreadTerminatedException" )
    {
    };
    ThreadTerminatedException( const ThreadTerminatedException& c )
    : ThreadException( c )
    {
    };
    virtual ~ThreadTerminatedException() throw() {};
};

}

#endif /* MUTEXEXCEPTION_H_ */
