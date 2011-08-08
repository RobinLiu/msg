#ifndef MESSAGE_SENDERTHREAD_H
#define MESSAGE_SENDERTHREAD_H
#include "Thread.h"

class ConnectionMgr;
class SenderThread : public HAUtils::Thread {
public:
  void run(void);

private:
  ConnectionMgr* cnnMgr;
};

#endif
