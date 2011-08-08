#include "ConnectionHandler.h"

#include <list>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <iomanip>
#include <typeinfo>
#include <netdb.h>
#include <poll.h>
#include <sys/un.h>
#include <sys/times.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

typedef std::list<struct sockaddr_storage> SockaddrList;


bool sockaddr_smaller(const struct sockaddr_storage& a, const struct sockaddr_storage& b) {
  return memcmp(&a, &b, sizeof(struct sockaddr_storage)) < 0;
}

bool sockaddr_equal(const struct sockaddr_storage& a, const struct sockaddr_storage& b) {
  return memcmp(&a, &b, sizeof(struct sockaddr_storage)) == 0;
}

SockaddrList getHostByName_(const std::string& hostName, const struct addrinfo& hints) {
  SockaddrList result;
//  result.clear();
  struct addrinfo* res;
  int32 status = getaddrinfo(hostName.c_str(), NULL, &hints, &res);
  if (0 != status) {
	  LOG(ERROR)<<"Getaddrinfo failed, error code is: "<<status;
	  return result;
  }

  struct addrinfo* addrInfo = res;
  while (NULL != addrInfo) {
    struct sockaddr_storage addr;
	memset(&addr, 0, sizeof(addr));
	memcpy(&addr, addrInfo->ai_addr, addrInfo->ai_addrlen);
	try {
	  result.push_back(addr);
	} catch(...) {
		LOG(ERROR)<<"Put addr into result list failed";
		freeaddrinfo(res);
		return result;
	}
    addrInfo = addrInfo->ai_next;
  }
  freeaddrinfo(res);
  result.sort(sockaddr_smaller);
  result.unique(sockaddr_equal);
  return result;
}

SockaddrList getHostByName(const std::string& hostName, int af) {
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = (af == -1) ? AF_UNSPEC:af;
  return getHostByName_(hostName, hints);
}



ConnectionHandler::ConnectionHandler(const char* hostName,
		                             uint32 portNumber,
		                             bool isServer,
		                             uint32 timeout)
  : m_hostName(hostName),
    m_portNumber(portNumber),
    m_isServer(isServer),
    m_timeout(timeout),
    m_socket(-1),
    m_connectionType(0){
//  m_sendQueue.clear();
  memset(&m_remoteAddr, 0, sizeof(m_remoteAddr));
}

error_no_t ConnectionHandler::createTCPServer(const char* hostName, uint32 portNumber) {
  LOG(INFO)<<"Begin to createTCP server for host: "<<hostName
		   <<" on port :"<<portNumber;
  CHECK(portNumber > 0);
  m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (-1 == m_socket) {
	LOG(ERROR)<<"Create TCP server failed";
	return CREATE_SERVER_SOCKET_FAILED_EC;
  }

  struct in_addr inAddr;
  struct in_addr anyAddr = { htonl(INADDR_ANY) };

  if (NULL != hostName) {
    SockaddrList sockaddrList = getHostByName(hostName, AF_INET);
    if (sockaddrList.empty()) {
      inAddr = anyAddr;
    } else {
      struct sockaddr_storage* ss = &(*sockaddrList.begin());
      inAddr = ((struct sockaddr_in*)ss)->sin_addr;
    }
  } else {
    inAddr = anyAddr;
  }

  struct sockaddr_in myAddr = { AF_INET, htons(portNumber), inAddr };
  int val = fcntl(m_socket, F_GETFL, 0);
  fcntl(m_socket, F_SETFL, val | O_NONBLOCK);
  fcntl(m_socket, F_SETFD, fcntl(m_socket, F_GETFD) | FD_CLOEXEC);

  int value = 1;
  socklen_t val_len = sizeof(int);
  setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &value, val_len);

  if (-1 == bind(m_socket, reinterpret_cast<sockaddr*>(&myAddr), sizeof(myAddr))) {
	int error_no = errno;
	close(m_socket);
	m_socket = -1;
	LOG(ERROR)<<"Bind socket failed, eror code is "<<error_no;
  }

  m_connectionType = AF_INET;

  LOG(INFO)<<"Finished create TCP Server";
  return SUCCESS_EC;
}

error_no_t ConnectionHandler::createTCPClient(const char* hostName,
		                   uint32 portNumber,
		                   int32 timeout) {
  LOG(INFO)<<"Start to create TCP client to host: "<<hostName<<" on port "<<portNumber;
  CHECK(NULL != hostName);
  CHECK(portNumber > 0);

  error_no_t ret = SUCCESS_EC;
  m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (-1 == m_socket) {
	ret = errno;
	LOG(ERROR)<<"Cannot create TCP socket client: "<<strerror(ret);
	return ret;
  }

  struct in_addr inAddr;
  struct hostent hostent;
  struct hostent* hostPtr = NULL;

  char buf[512];
  size_t buflen = 512;
  int h_errnop = 0;

  struct sockaddr_in myAddr;
  memset(&myAddr, 0, sizeof(myAddr));
  if (0 == memcmp(&m_remoteAddr, &myAddr, sizeof(myAddr))) {
	//TODO: getaddrinfo(&myAddr);
  } else {
	myAddr = m_remoteAddr;
  }

  int val = fcntl(m_socket, F_GETFL, 0);
  fcntl(m_socket, F_SETFL, val | O_NONBLOCK);
  fcntl(m_socket, F_SETFL, fcntl(m_socket, F_GETFD) | FD_CLOEXEC);
  int value = 1;
  socklen_t val_len = sizeof(int);
  setsockopt(m_socket, SOL_SOCKET, SO_KEEPALIVE, &value, val_len);

  int flag = 1;
  if( 0 != setsockopt(m_socket,
		              IPPROTO_TCP,
		              TCP_NODELAY,
		              (char*)flag,
		              sizeof(int))) {
    LOG(ERROR)<<"Set TCP_NODELAY option failed for socket: "<<m_socket;
  }
  if (-1 == connect(m_socket, reinterpret_cast<sockaddr*>(&myAddr), sizeof(myAddr))) {
	if (EINPROGRESS != errno) {
      error_no_t error = errno;
      close(m_socket);
      m_socket = -1;
      LOG(ERROR)<<"Could not connect to the host"<<strerror(error);
      return error;
	}
  }

  struct pollfd wset;
  wset.fd = m_socket;
  wset.events = POLLOUT;

  ret = poll(&wset, 1, timeout ? timeout*1000 : -1);
  if (-1 == ret) {
	error_no_t error = errno;
	close(m_socket);
	m_socket = -1;
	LOG(ERROR)<<"Could not connect to the host: "<<strerror(error);
	return error;
  } else if (0 == ret) {
	close(m_socket);
	m_socket = -1;
	LOG(ERROR)<<"Could not connect to the host: connection timeout";
	return TIMEOUT_EC;
  } else if (wset.revents & POLLERR) {
	close(m_socket);
	m_socket = -1;
	LOG(ERROR)<<"POLLERR Addr: "<<hostName<<": "<<portNumber;
	return POLLERR;
  } else if (wset.revents & POLLOUT) {
	val = 0;
	getsockopt(m_socket, SOL_SOCKET, SO_ERROR, &val, &val_len);
	if ( 0 != val) {
      error_no_t error = val;
      close(m_socket);
      m_socket = -1;
      return error;
	}

	char addrBuf[INET_ADDRSTRLEN];
	m_peerAddress = inet_ntop(AF_INET, &inAddr, addrBuf, INET_ADDRSTRLEN);
  } else {
	close(m_socket);
	m_socket = -1;
	LOG(ERROR)<<"Could not connect to host";
	return SOCKET_CONNECT_ERR_EC;
  }

  m_remoteAddr = myAddr;
  m_connectionType = AF_INET;

  LOG(INFO)<<"Finished create TCP client";
  return SUCCESS_EC;
}
