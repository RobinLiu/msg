#include "common/base/msg_queue.h"
#include "common/base/message.h"
#include "common/include/common.h"
#include "message/mock_API/app_info.h"
#include <unistd.h>
#include <string.h>

extern uint16 dst_group_id;
uint32 recvd_msg_num = 0;
void on_msg_received(void* msg) {
  CHECK(NULL != msg);
  ++recvd_msg_num;
  if(0 == (recvd_msg_num % 10000)) {
    LOG(ERROR, "Received %d messages", recvd_msg_num);
  }
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
  init_sys_info();
  init_msg_queue(&on_msg_received);
  message_t* msg = NULL;
  /*char key = 0;
  node_status_t node_status;
  uint32 msglen = 2000;

  while(-1 != (key = getchar())) {
    switch(key) {
    case 'q':
      return -1;
    case 'd':
      msg = allocate_msg_buff(sizeof(node_status_t));
      fill_msg_header(group_id, app_id, RANGE_ACTIVE, 4, msg);
      msg->header->msg_id = MSG_ID_NODE_STATUS;
      node_status.node_id = num_of_msg;
      node_status.node_status = 1;
      memcpy((void*)msg->body, (void*)&node_status, sizeof(node_status));
      if (0 != send_msg(msg)) {
        num_of_droped_msg++;
      } else {
        num_of_send_msg++;
      }
      free_msg_buff(&msg);
      LOG(INFO, "one message is send");
      break;
    case 'u':
      msg = allocate_msg_buff(sizeof(node_status_t));
      fill_msg_header(group_id, app_id, RANGE_ACTIVE, 4, msg);
//      node_status_t node_status;
      msg->header->msg_id = MSG_ID_NODE_STATUS;
      node_status.node_id = num_of_msg;
      node_status.node_status = 2;
      memcpy((void*)msg->body, (void*)&node_status, sizeof(node_status));
      if (0 != send_msg(msg)) {
        num_of_droped_msg++;
      } else {
        num_of_send_msg++;
      }
      free_msg_buff(&msg);
      break;
    case 'o':
      msg = allocate_msg_buff(msglen);
      fill_msg_header(group_id, app_id, RANGE_ACTIVE, 4, msg);
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
      break;
    default:
      break;
    }
  }
*/


  LOG(ERROR, "Begin Number of msg send: %d, received %d", num_of_send_msg, recvd_msg_num);
  while(num_of_msg--) {
//  while(1) {
    uint32 msglen = 2000;
    msg = allocate_msg_buff(msglen);
//    msg->header->msg_len = msglen;
    fill_msg_header(group_id, app_id, RANGE_ACTIVE, 4, msg);
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
//    if(0 == (num_of_msg % 500)) {
//      sleep(2);
//      return 0;
//    }
//    usleep(100);
//    sleep(30);
//    getchar();
  }

  LOG(ERROR, "Number of msg send: %d, received %d", num_of_send_msg, recvd_msg_num);
  while ('q' != getchar()) {
    LOG(ERROR, "Number of msg send: %d, received %d", num_of_send_msg, recvd_msg_num);
    continue;
  }
//  destroy_msg_queue();
  return 0;
}
