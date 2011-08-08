#ifndef MESSAGE_CONNECTIONHANDLER_H
#define MESSAGE_CONNECTIONHANDLER_H

#include "common/include/common.h"
#include <queue>
#include "netinet/in.h"

class Message;
class ConnectionHandler {
public:
  ConnectionHandler(const char* target, uint32 portNumber,
		            bool isServer = false, uint32 timeout = 0);

  error_no_t createTCPServer(const char* hostName, uint32 portNumber);
  error_no_t createTCPClient(const char* hostName, uint32 portNumber, int32 timeout);


private:
  std::queue<Message*> m_sendQueue;
  std::string m_hostName;
  std::string m_peerAddress;
  uint32 m_portNumber;
  bool m_isServer;
  uint32 m_timeout;
  int m_socket;
  int m_connectionType;
  struct ::sockaddr_in m_remoteAddr;
};

#endif
