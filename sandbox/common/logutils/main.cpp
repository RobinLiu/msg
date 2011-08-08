//============================================================================
// Name        : Log.cpp
// Author      : Robin
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C, Ansi-style
//============================================================================

#include "logging.h"
#include <stdio.h>
#include <stdlib.h>

void func1() {
  LOG(FATAL)<<"FATAL in func 1";
}

void func2() {
  LOG(INFO)<<"func 2";
  DCHECK(2==3);
  func1();
}

void func3() {
  LOG(INFO)<<"in func 3";
  func2();
}



int main(void) {
	logging::InitLogging("outputlog.txt",
                logging::LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG,
                logging::LOCK_LOG_FILE,
                logging::APPEND_TO_OLD_LOG_FILE,
                /*logging::ENABLE_DCHECK_FOR_NON_OFFICIAL_RELEASE_BUILDS*/
	            logging::DISABLE_DCHECK_FOR_NON_OFFICIAL_RELEASE_BUILDS);
	LOG(INFO)<<"test log";
	func3();
	LOG(ERROR)<<"test error log";
	LOG(FATAL)<<"test fatal log";
	CHECK(1==2);
}
