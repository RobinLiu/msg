#include "ConnectionMgr.h"
#include "MutexHolder.h"

HAUtils::Mutex ConnectionMgr::m_cnnMgrMutex;

ConnectionMgr* ConnectionMgr::m_cnnMgr = NULL;

ConnectionMgr::ConnectionMgr() {

}

ConnectionMgr::ConnectionMgr() {
  if (pipe(m_receiverPipe) < 0) {
    LOG(FATAL)<<"Create Receiver pipe failed";
  }
  fcntl(m_receiverPipe[0], F_SETFD, fcntl(m_receiverPipe[0], F_GETFD) | FD_CLOEXEC);
  fcntl(m_receiverPipe[1], F_SETFD, fcntl(m_receiverPipe[1], F_GETFD) | FD_CLOEXEC);

  if (pipe(m_senderPipe) < 0) {
    LOG(FATAL)<<"Create Receiver pipe failed";
  }
  fcntl(m_senderPipe[0], F_SETFD, fcntl(m_senderPipe[0], F_GETFD) | FD_CLOEXEC);
  fcntl(m_senderPipe[1], F_SETFD, fcntl(m_senderPipe[1], F_GETFD) | FD_CLOEXEC);

  m_descHolder = new DescHolder();
  CHECK(NULL != m_descHolder);

  {
    HAUtils::MutexHolder mh(m_descHolder->m_mutex);
    m_descHolder->addFdToSet(m_receiverPipe[0], POLLIN);
  }

  m_receiverThread = new ReceiverThread();
  CHECK(m_receiverThread != NULL);
  m_senderThread = new SenderThread();
  CHECK(m_senderThread != NULL);

  try {
    m_receiverThread->start();
    m_senderThread->start();
  } catch (...) {
    LOG(FATAL)<<"Start listener and sender thread failed";
  }
}

ConnectionMgr::~Connection() {
  if (m_listenerThread && m_listenerThread->isRunning()) {
    //TODO: inform thread to stop;
    //
  }

  delete m_receiverThread;
  delete m_senderThread;
  delete m_descHolder;

  close(m_senderPipe[0]);
  close(m_senderPipe[1]);
  close(m_receiverPipe[0]);
  close(m_receiverPipe[1]);
}

ConnectionMgr* ConnectionMgr::getInstance() {
  if (NULL == m_cnnMgr) {
    HAUtils::MutexHolder mh(m_cnnMgrMutex);
    if (NULL == m_cnnMgr) {
	  m_cnnMgr = new ConnectionMgr();
    }
  }
  return m_cnnMgr;
}
