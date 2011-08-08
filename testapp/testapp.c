#include "common/base/msg_queue.h"
#include "common/base/message.h"
#include "common/include/common.h"
#include "message/mock_API/app_info.h"
#include <unistd.h>

extern uint16 dst_group_id;
uint32 recvd_msg_num = 0;
void on_msg_received(void* msg) {
  CHECK(NULL != msg);
  LOG(ERROR, "Received %d messages", ++recvd_msg_num);
//  print_msg((message_t*)msg);
}

int main(int argc, char** argv) {
  if(argc != 4) {
    printf("Usage: %s dest_group_id  dest_app_id  package_num\n", argv[0]);
    exit(0);
  }
  uint8 group_id = (uint8)atoi(argv[1]);
  uint16 app_id = (uint16)atoi(argv[2]);
  uint32 num_of_msg = (uint32)atoi(argv[3]);
  uint32 num_of_send_msg = 0;
  uint32 num_of_droped_msg = 0;
  init_msg_center();
  init_msg_queue(&on_msg_received);
  message_t* msg = NULL;
  while(num_of_msg--) {
//  while(1) {
    uint32 msglen = 2000;
    msg = allocate_msg_buff(msglen);
    fill_msg_header(group_id, app_id, RANGE_ACTIVE, 4, msg);
//    msg->header->msg_len = msglen;
//    msg->header->msg_seq = msg_seq++;
    uint8* buf = msg->body;
    uint32 i = 0;
    for(i = 0; i < msglen; ++i) {
      *buf++ = (uint8)('a' + (i%26));
    }
    if (0 != send_msg(msg)) {
      num_of_droped_msg++;
    } else {
      num_of_send_msg++;
    }
    free_msg_buff(&msg);
//    print_msg(msg);
    LOG(INFO, "Number of msg send: %d, droped %d", num_of_send_msg, num_of_droped_msg);
//    if(num_of_msg == 65534) {
//      return 0;
//    }
//    usleep(100);
//    sleep(30);
//    getchar();
  }

  while ('q' != getchar()) {
    continue;
  }
//  destroy_msg_queue();
  return 0;
}
