#ifndef MESSAGE_CONNECTIONMGR_H
#define MESSAGE_CONNECTIONMGR_H

#include "common/include/common.h"
#include "Mutex.h"
#include "DescHolder.h"

class ConnectionMgr {
public:
  static ConnectionMgr* getInstance();

  static HAUtils::Mutex m_cnnMgrMutex;

private:
  ConnectionMgr();

  static ConnectionMgr* m_cnnMgr;
  int m_senderPipe[2];
  int m_receiverPipe[2];
  ReceiverThread* m_receiverThread;
  SenderThread*  m_senderThread;
  HAUtils::Mutex  m_mgrMutex;
  DescHolder* m_descHolder;

  DISALLOW_COPY_AND_ASSIGN(ConnectionMgr);
};

#endif
