#include "mock_driver.h"
#include "msg_link.h"
#include "common/include/common.h"
#include "common/base/thread.h"
#include "message/mock_API/app_info.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

void dbg_print_msg_header(uint8* msg)
{
	uint8* buf = msg;
	uint32 i = 0;
	printf("Message:");
	printf("\n\tDest Mac:");
	for (i = 0; i < 6; ++i)
	{
		printf("%2x%s", buf[i], (i == 5 ? "" : ":"));
	}
	buf += 6;
	printf("\n\tSrc Mac:");
	for (i = 0; i < 6; ++i)
	{
		printf("%2x%s", buf[i], i == 5 ? "" : ":");
	}

	buf += 6;
	uint16 prot = *((uint16*) buf);
	printf("\n\tProtocol type: %x", prot);

	buf += 2;

	printf("\n\tlink header:");
	link_header_t* lh = (link_header_t*) buf;
	printf("\n\t\tfrag_type: %d", lh->pdu_type);
	printf("\n\t\tfrag_seq: %d", lh->frag_seq);
	printf("\n\t\tfrag_index: %d", lh->frag_index);
	printf("\n\t\tmsg_seq: %d", lh->msg_seq);
	printf("\n\t\tdst_node: %d", lh->dst_node);
	printf("\n\t\tsrc_node: %d", lh->src_node);
	printf("\n\t\tis_last_frag: %d\n", lh->is_last_frag);
}

void dbg_print_msg(uint8* msg)
{
	uint8* buf = msg;
	int32 i = 0;
	dbg_print_msg_header(msg);
	link_header_t* lh = (link_header_t*) (buf + 14);
	if (lh->pdu_type != PDU_DATA)
	{
		return;
	}

	buf += TOTAL_HEADER_LEN;
	if (lh->frag_index == 0)
	{
		msg_header_t* mh = (msg_header_t*) buf;

		printf("\n\tSender: group %d, app %d, range %d", mh->rcver.group_id,
				mh->rcver.app_id, mh->rcver.role);
		printf("\n\tReceiver: group %d, app %d, range %d", mh->snder.group_id,
				mh->snder.app_id, mh->snder.role);
		buf += sizeof(msg_header_t);
	}

	printf("\n\tMsg Content:");
	if (lh->is_last_frag == 1)
	{
		int32 msg_len = lh->frag_len - sizeof(msg_tail_t)
				- sizeof(msg_header_t);
		for (i = 0; i < msg_len; ++i)
		{
			printf("%c", buf[i]);
		}
		printf("\n");
		msg_tail_t* tail = (msg_tail_t*) (buf + msg_len);
		printf("\n\tTail:%x", tail->magic_num);
		printf("\n");
	}
	else
	{
		int32 msg_len = lh->frag_len - sizeof(link_header_t);
		for (i = 0; i < msg_len; ++i)
		{
			printf("%c", buf[i]);
		}
		printf("\n");
	}
}

void driver_xmit_pkt(int connfd, void* conndata, void* buff, uint32 buff_len)
{
//  LOG(INFO, "package len is %d, send package...", buff_len);
//  dbg_print_msg((uint8*)buff);
//  LOG(INFO, "Write content to dirver");
	int nsend = sendto(connfd, buff, buff_len, 0, (struct sockaddr *) conndata,
			sizeof(struct sockaddr_in));
	if (nsend < 0)
	{
		LOG(
				INFO,
				"send msg to UDP[%s:%u] failed",
				inet_ntoa(((struct sockaddr_in*)conndata)->sin_addr), ntohs(((struct sockaddr_in*)conndata)->sin_port));
		close(connfd);
		connfd = -1;
	}
	/*LOG(INFO, "send msg to UDP[%s:%u] done",inet_ntoa(((struct sockaddr_in*)conndata)->sin_addr),
	 ntohs(((struct sockaddr_in*)conndata)->sin_port));*/
//  size_t n = write(out_fd, buff, buff_len);
//  CHECK(n == buff_len);
}

void driver_rcv_pkt(void* msg_frag, uint32 msg_len)
{
	on_pkt_received(msg_frag, msg_len);
}

void* tst_receiver(void* arg)
{
	(void) arg;
	uint16 port = get_self_port();

	int sockfd;
	struct sockaddr_in servaddr, cliaddr;

	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	CHECK(sockfd != -1);

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	int ret = bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	CHECK(-1 != ret);

	int msg_len;
	socklen_t len;

	char msg_buf[8192];
	while (1)
	{
		len = sizeof(cliaddr);
		msg_len = recvfrom(sockfd, msg_buf, sizeof(msg_buf), 0,
				(struct sockaddr *) &cliaddr, &len);
		CHECK(-1 != msg_len);
		/*LOG(INFO, "Receive %d bytes from udp socket:[%s:%u]",
		 msg_len,
		 inet_ntoa(cliaddr.sin_addr),
		 ntohs(cliaddr.sin_port));*/
		uint8* rcvd_msg = (uint8*) malloc(msg_len);
		CHECK(NULL != rcvd_msg);
		memcpy(rcvd_msg, msg_buf, msg_len);
//    dbg_print_msg((uint8*)rcvd_msg);
		driver_rcv_pkt(rcvd_msg, (uint32) msg_len);
		free(rcvd_msg);
		rcvd_msg = NULL;
	}

	/*  struct pollfd pfd;
	 pfd.fd = in_fd;
	 pfd.events=POLLIN;
	 while(1) {
	 if(poll(&pfd, 1, -1) > 0) {
	 if(pfd.revents & POLLIN) {
	 LOG(INFO, "Package received");
	 pfd.revents = 0;
	 } else {
	 continue;
	 }
	 }

	 memset(msg, 0, sizeof(msg));
	 uint32 msg_len = read(in_fd, msg, sizeof(msg));
	 dbg_print_msg(msg);
	 driver_rcv_pkt(msg, msg_len);
	 }
	 */
	return (void*) (0);
}

void create_rcv_thread()
{
	thread_id_t thread_id;
	create_thread(&thread_id, NULL, &tst_receiver, NULL);
}
