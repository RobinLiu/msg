#ifndef MESSAGE_SENDERTHREAD_H
#define MESSAGE_SENDERTHREAD_H

#include "Thread.h"
#include "ConnectionMgr.h"

class ConnectionMgr;

class ReceiverThread : public HAUtils::Thread {
public:
  ReceiverThread() {
//    Thread(true);
	  m_cnnMgr = ConnectionMgr::getInstance();
  }
  void run(void);
private:
  ConnectionMgr* m_cnnMgr;
};

#endif
