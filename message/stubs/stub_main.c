#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "msg_link.h"


#define PIPE_OUT "/tmp/out"    /*message from the app*/
#define PIPE_IN "/tmp/in"      /*send message from the stub*/


int main() {
	int fd_out, fd_in;
	printf("0.\n");
	mkfifo(PIPE_IN, 0777);
	printf("1.\n");
	mkfifo(PIPE_OUT, 0777);
	printf("2.\n");
	fd_out  = open(PIPE_OUT, O_RDONLY);
	printf("3.\n");
	fd_in = open(PIPE_IN, O_WRONLY);
	printf("3.\n");

	struct pollfd pfd;
	pfd.fd = fd_out;
	pfd.events=POLLIN;
	unsigned short frag_seq = 0;
	unsigned char msg[2048] = {0};
	printf("FIFO are opened, now polling.\n");
	while(1) {
	if(poll(&pfd, 1, -1) > 0) {
		if(pfd.revents & POLLIN) {
			printf("Package received");
			pfd.revents=0;
		} else {
			continue;
		}
	}
    memset((void*)msg, 0, sizeof(msg));
    read(fd_out, msg, sizeof(msg));
	link_header_t* lh = (link_header_t*)(msg + 14);
	printf("PDU is %d\n", lh->pdu_type);
	if(lh->dst_node != 2) {
		printf("lh->dst_node != 2\n");
		continue;
	}
	unsigned short node = lh->src_node;
	lh->src_node = lh->dst_node;
	lh->dst_node = node;
	if(lh->pdu_type == 1 ) {
		//if(frag_seq % 2 == 0 ) {
			lh->pdu_type = 2;
			printf("send ack message\n");
			write(fd_in, msg, sizeof(msg));
		//}
		lh->pdu_type = 1;
		lh->frag_seq = frag_seq++;
		lh->is_last_frag = 1;
		write(fd_in, msg, sizeof(msg));
	}
	

	//}
	}
}
