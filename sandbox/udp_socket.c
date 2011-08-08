#include "udp_socket.h"
#include "common/include/common.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

void create_udp_server(/*const char* hostname, */uint32 port) {
  int sockfd;
  struct sockaddr_in servaddr, cliaddr;

  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  CHECK(sockfd != -1);

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  int ret = bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
  CHECK(-1 != ret);

  int n;
  socklen_t len;
  char msg[8192];
  while(1) {
    len = sizeof(cliaddr);
    n = recvfrom(sockfd, msg, 8192, 0, (struct sockaddr *)&cliaddr, &len);
    CHECK(-1 != n);
    LOG(INFO, "Receive %d bytes from udp socket:[%s:%u]",
        n,
        inet_ntoa(cliaddr.sin_addr),
        ntohs(cliaddr.sin_port));
    //TODO:do the job;
  }
}


int create_udp_client(const char* serv_ip_addr, uint32 serv_port) {
  int sockfd;
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(serv_port);
  inet_pton(AF_INET, serv_ip_addr, &servaddr.sin_addr);
  //inet_aton

  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  CHECK(-1 != sockfd);
  return sockfd;


  /*uint8 msg[8192];

  int nsend = sendto(sockfd, msg, sizeof(msg), 0, (struct sockaddr *)&servaddr,
                     sizeof(servaddr));
  if (nsend < 0) {
    LOG(INFO, "send msg to UDP[%s:%u] failed",inet_ntoa(servaddr.sin_addr),
        ntohs(servaddr.sin_port));
    close(sockfd);
    sockfd = -1;
  }*/
}
