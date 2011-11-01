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
//  init_msg_system(&on_msg_received);
  init_msg_system(&msg_client);
//  pthread_t sender_id;
//  pthread_create(&sender_id, NULL, &sender_thread, NULL);
  message_t* msg = NULL;
  int msg_id = 1;
  char rsp_buf[4];
  int ret = 123;
  while(1) {
    if(0 == receive_msg(&msg_client, &msg)) {
      continue;
    }
    LOG(INFO, "Received a msg in app id is %d, response with ack",
        msg->header->msg_id);
    print_msg_header(msg);
    if(msg->header->msg_id == 1) {
      memcpy(rsp_buf, &ret, sizeof(int));
      LOG(INFO, "Begin to send rsp msg...");
      send_rsp_msg(msg, msg_id, rsp_buf, sizeof(rsp_buf));
    }
  }
//  char key;
//  while (EOF != (key = getchar())) {
//    switch(key) {
//      case 'q':
//        break;
//      case 's':
//        pthread_cancel(sender_id);
//        pthread_join(sender_id, NULL);
//        break;
//    }
//    LOG(IMPORTANT, "Number of msg send: %d, received %d", send_msg_num, recvd_msg_num);
//  }

  LOG(IMPORTANT, "Number of msg send: %d, received %d", send_msg_num, recvd_msg_num);
//  destroy_msg_queue();
  return 0;
}
