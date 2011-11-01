//#include "common/base/msg_queue.h"
#include "common/base/msg_client.h"
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
