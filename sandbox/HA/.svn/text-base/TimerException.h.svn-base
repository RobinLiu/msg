/*
 * HAException.h
 *
 *  Created on: 2011-6-23
 *      Author: Elric
 */

#ifndef TIMEEXCEPTION_H_
#define TIMEEXCEPTION_H_

#include <string>
#include <exception>

namespace TimeLib {

class errorOnLineException: public std::exception {
public:
    std::string errorText;
    std::string fileName;
    int lineNumber;

    errorOnLineException(std::string theError, std::string theFile, int theLine) {
        errorText = theError;
        fileName = theFile;
        lineNumber = theLine;
    }

    ~errorOnLineException() throw () {
    }

};

class Exception: public errorOnLineException {
public:
    Exception(std::string theError, std::string theFile, int theLine) :
        errorOnLineException(theError, theFile, theLine) {
    }
    ~Exception() throw () {
    }
    void dump(std::ostream&) const {
    }
};

class InitializationFailure: public Exception {
public:
    InitializationFailure(std::string theError, std::string theFile,
            int theLine) :
        Exception(theError, theFile, theLine) {
    }
};

class RegistrationFailure: public Exception {
public:
    RegistrationFailure(std::string theError, std::string theFile, int theLine) :
        Exception(theError, theFile, theLine) {
    }
};

class CancellationFailure: public Exception {
public:
    CancellationFailure(std::string theError, std::string theFile, int theLine) :
        Exception(theError, theFile, theLine) {
    }
};

class TimeoutHandlerRunning: public Exception {
public:
    TimeoutHandlerRunning(std::string theError, std::string theFile,
            int theLine) :
        Exception(theError, theFile, theLine) {
    }
};

class TerminationFailure: public Exception {
public:
    TerminationFailure(std::string theError, std::string theFile, int theLine) :
        Exception(theError, theFile, theLine) {
    }
};

class SystemException: public Exception {
public:
    SystemException(std::string theError, std::string theFile, int theLine) :
        Exception(theError, theFile, theLine) {
    }
};

}

#endif /* HAEXCEPTION_H_ */
