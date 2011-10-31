#include "common/base/msg_queue.h"
#include "common/base/message.h"
#include "common/include/common.h"
#include "message/mock_API/app_info.h"
#include <unistd.h>
#include <string.h>
#include <pthread.h>


extern uint16 dst_group_id;
uint32 recvd_msg_num = 0;
uint32 send_msg_num = 0;
uint32 droped_msg_num = 0;
uint8 group_id;
uint16 app_id;


void on_msg_received(void* msg) {
  CHECK(NULL != msg);
  ++recvd_msg_num;
  if(0 == (recvd_msg_num % 10000)) {
    LOG(IMPORTANT, "Received %d messages", recvd_msg_num);
  }
//  print_msg((message_t*)msg);
}


void * sender_thread(void* arg) {
  (void)arg;
  message_t* msg = NULL;
  LOG(INFO, "Sender started");
  while(1) {
    uint32 msglen = 2000;
    msg = allocate_msg_buff(msglen);
    fill_msg_header(group_id, app_id, RANGE_ACTIVE, 4, 0, msg);
    uint8* buf = msg->body;
    uint32 i = 0;
    for(i = 0; i < msglen; ++i) {
      *buf++ = (uint8)('a' + (i%26));
    }
    if (0 != send_msg(msg)) {
      droped_msg_num++;
    } else {
      send_msg_num++;
    }
    free_msg_buff(&msg);
//    if (1 == send_msg_num) {
//      return;
//    }
    if (0 == (send_msg_num % 5000)) {
      LOG(INFO, "send %d messages", send_msg_num);
    }
  }

  return NULL;
}


int main(int argc, char** argv) {
  if(argc != 3) {
    printf("Usage: %s dest_group_id  dest_app_id  package_num\n", argv[0]);
    exit(0);
  }
  msg_client_t msg_client;
  group_id = (uint8)atoi(argv[1]);
  app_id = (uint16)atoi(argv[2]);
  init_sys_info();
  init_msg_system(&msg_client);




  message_t* msg = NULL;
  uint32 msglen = 2000;
  msg = allocate_msg_buff(msglen);
  fill_msg_header(group_id, app_id, RANGE_ACTIVE, 4, 1, msg);
  int ret = send_sync_msg(msg, 10000, &msg_client);
  LOG(INFO,"sync msg result is %d", ret);
  return 0;
}
